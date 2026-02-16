#pragma once

#include <vector>
#include <map>
#include <functional>
#include <CppLinuxSerial/SerialPort.hpp>

#include <sys/ioctl.h>
#include <unistd.h> // usleep関数のため


typedef unsigned char uint8_t;
using namespace mn::CppLinuxSerial;

class UartLink {
    public:
        UartLink(std::string device="/dev/ttyACM0", BaudRate baud = BaudRate::B_115200); // コンストラクタ
        ~UartLink();

        std::vector<uint8_t> send(std::vector<uint8_t> data, uint8_t id); // 送信
        void receive(std::vector<uint8_t> data); // 受信

        static std::vector<uint8_t> encode_cobs(std::vector<uint8_t> src); // COBSエンコード
        static std::vector<uint8_t> decode_cobs(std::vector<uint8_t> src); // COBSデコード

        static std::vector<uint8_t> add_header(std::vector<uint8_t> src, uint8_t id); // ヘッダを付加
        static std::vector<uint8_t> remove_header(std::vector<uint8_t> src, uint8_t* id, bool* error); // ヘッダを削除

        static uint8_t calc_checksum(std::vector<uint8_t> src); // チェックサムを計算

        void loop(); // 受信ループ

        void set_callback(uint8_t id, std::function<void(std::vector<uint8_t>)> callback); // idごとの受信コールバックをセット
        void set_general_callback(std::function<void(uint8_t, std::vector<uint8_t>)> callback); // すべてのidに対する受信コールバックをセット
        void set_error_callback(std::function<void(int16_t, std::string)> callback); // エラーコールバックをセット
        void set_disconnect_callback(std::function<void()> callback); // 切断コールバックをセット

        int reset_device(); // デバイスをリセット

        static std::vector<std::string> get_device_list(); // デバイスリストを取得

    private:
        SerialPort serial; // シリアルポート

        std::vector<uint8_t> receive_buffer; // 受信バッファ
        std::map<uint8_t, std::function<void(std::vector<uint8_t>)>> callback_map; // コールバックマップ
        std::function <void(uint8_t, std::vector<uint8_t>)> callback_no_id; // コールバック (idなし)
        std::function <void(int16_t, std::string)> error_callback; // エラーコールバック (id, message) id=-1の場合はIDも不明
        std::function<void()> loopFunc; // ループ関数(ブロッキングする処理を入れるので)
        std::function<void()> disconnect_callback; // 切断コールバック

        std::string device; // デバイス名
};
