#include "uart.hpp"

#include <fcntl.h>
#include <linux/usbdevice_fs.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>

#include <filesystem>

UartLink::UartLink(std::string device, BaudRate baud) : serial(device, baud) {
    serial.SetTimeout(0);  // ブロッキングしない
    serial.Open();
    this->device = device;
}

UartLink::~UartLink() {
    serial.Close();  // デストラクタでシリアルポートを閉じる
}

void UartLink::set_error_callback(
    std::function<void(int16_t, std::string)> callback
) {
    error_callback = callback;  // エラーコールバックをセット
}

void UartLink::set_callback(
    uint8_t id, std::function<void(std::vector<uint8_t>)> callback
) {
    callback_map[id] = callback;  // idごとの受信コールバックをセット
}

void UartLink::set_general_callback(
    std::function<void(uint8_t, std::vector<uint8_t>)> callback
) {
    callback_no_id = callback;  // すべてのidに対する受信コールバックをセット
}

void UartLink::set_disconnect_callback(std::function<void()> callback) {
    disconnect_callback = callback;  // 切断コールバックをセット
}

// データを送信
std::vector<uint8_t> UartLink::send(std::vector<uint8_t> data, uint8_t id) {
    std::vector<uint8_t> headered = add_header(data, id);
    std::vector<uint8_t> encoded = encode_cobs(headered);
    if (serial.GetState() != State::OPEN) {
        disconnect_callback();  // シリアルポートが開いていない場合は切断コールバックを呼ぶ
        return encoded;
    } else {
        serial.WriteBinary(encoded);
        return encoded;
    }
}

// COBSエンコード
std::vector<uint8_t> UartLink::encode_cobs(std::vector<uint8_t> src) {
    int last_zero_index = 0;  // 最後に0x00が出現したインデックス
    std::vector<uint8_t> dst;
    dst.push_back(0x00);  // 先頭に0x00を追加
    for (int i = 0; i < (int)src.size(); i++) {
        if (src[i] == 0x00) {
            // 0x00が出現したら、直前までの0x00からのバイト数を書き込む
            dst.at(last_zero_index) = i - last_zero_index + 1;
            dst.push_back(0x00);
            last_zero_index = i + 1;
        } else {
            // 0x00以外が出現したら、そのまま書き込む
            dst.push_back(src[i]);
        }
    }
    dst.at(last_zero_index) =
        src.size() - last_zero_index + 1;  // 最後の0x00からのバイト数を書き込む
    dst.push_back(0x00);                   // 終端に0x00を追加
    return dst;
}

// COBSデコード
std::vector<uint8_t> UartLink::decode_cobs(std::vector<uint8_t> src) {
    std::vector<uint8_t> dst;
    int next_zero_index = src[0];  // 次に0x00が出現するインデックス
    for (int i = 1; i < (int)src.size(); i++) {
        if (i == next_zero_index) {
            // 0x00が出現するインデックスになったら、0x00を追加して次に0x00が出現するインデックスを更新
            dst.push_back(0x00);
            next_zero_index += src[i];
        } else {
            // そうでなければそのまま追加
            dst.push_back(src[i]);
        }
    }
    return dst;
}

// ヘッダを付加
std::vector<uint8_t> UartLink::add_header(
    std::vector<uint8_t> src, uint8_t id
) {
    std::vector<uint8_t> dst;
    dst.push_back(id);                              // 先頭にidを追加
    dst.push_back((uint8_t)src.size());             // 次にデータ長を追加
    dst.push_back(calc_checksum(src));              // 次にチェックサムを追加
    dst.insert(dst.end(), src.begin(), src.end());  // 次にデータを追加
    return dst;
}

// ヘッダを削除
std::vector<uint8_t> UartLink::remove_header(
    std::vector<uint8_t> src, uint8_t* id, bool* error
) {
    *id = src.at(0);           // 先頭のidを取得
    int length = src.at(1);    // 次のデータ長を取得
    int checksum = src.at(2);  // 次のチェックサムを取得
    std::vector<uint8_t> dst;  // フレームを削除したデータの格納先
    dst = std::vector<uint8_t>(
        src.begin() + 3, src.begin() + 3 + length
    );  // フレームを削除
    *error =
        (calc_checksum(dst) != checksum) ||
        ((int)src.size() !=
         length +
             4);  // チェックサムが一致しないか、データ長が一致しない場合はerrorをtrueにする
    return dst;
}

// チェックサムを計算
uint8_t UartLink::calc_checksum(std::vector<uint8_t> src) {
    uint8_t sum = 0;
    for (uint8_t i : src) {
        sum += i;
    }
    return sum;
}

// 受信ループ
void UartLink::loop() {
    std::vector<uint8_t> data;
    if (serial.GetState() != State::OPEN) {
        disconnect_callback();  // シリアルポートが開いていない場合は切断コールバックを呼ぶ
        return;
    } else {
        serial.ReadBinary(data);
    }

    for (uint8_t i : data) {
        receive_buffer.push_back(i);  // 受信バッファに1バイトずつ追加
        if (i == 0x00) {
            // 0x00が出現したら、受信バッファをデコードして受信処理を行う
            receive(receive_buffer);
            receive_buffer.clear();
        }
    }
}

// 受信処理
void UartLink::receive(std::vector<uint8_t> data) {
    uint8_t id;  // id
    bool error;  // エラーが発生したかどうか

    // データが4バイト未満の場合は受信処理を行わない
    if (data.size() < 4) {
        if (error_callback) {
            error_callback(-1, "Received data is too short");
        }
        return;
    }

    // データをデコードしてヘッダを削除
    std::vector<uint8_t> decoded = decode_cobs(data);
    std::vector<uint8_t> removed = remove_header(decoded, &id, &error);

    // エラーが発生した場合は受信処理を行わない
    if (error) {
        // TODO: 受信エラーが発生した場合にユーザーに通知する処理
        if (error_callback) {
            error_callback(id, "Checksum does not match");
        }
        return;
    }

    // idごとのコールバックを呼び出す
    if (callback_map.find(id) != callback_map.end()) {
        callback_map[id](removed);
    }

    // 全idに対するコールバックを呼び出す
    if (callback_no_id) {
        callback_no_id(id, removed);
    }
}

// デバイスリストを取得
std::vector<std::string> UartLink::get_device_list() {
    std::vector<std::string> device_list;

    for (const auto& entry : std::filesystem::directory_iterator("/dev")) {
        std::string path = entry.path().string();
        if (path.find("/dev/ttyACM") != std::string::npos ||
            path.find("/dev/ttyUSB") != std::string::npos) {
            device_list.push_back(entry.path());
        }
    }
    return device_list;
}

// リセット信号を送信
int UartLink::reset_device() {
    int fd = open(device.c_str(), O_WRONLY);
    if (fd < 0) {
        return -1;
    }

    int rc = ioctl(fd, USBDEVFS_RESET, 0);
    close(fd);

    return rc;
}
