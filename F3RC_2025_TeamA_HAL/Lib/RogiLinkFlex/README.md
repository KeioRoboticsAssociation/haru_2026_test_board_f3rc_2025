# RogiLinkFlex

RogiLinkFlexのHAL実装です。UART通信、RS485通信に対応しています。
送受信するデータ型を柔軟に設定できるようになっており、データ変換も自動で行います。マイコン同士の通信、マイコンとros2の通信の両方に対応しています。

## Other Environments

- [rogilinkFlex-ros2](https://github.com/KeioRoboticsAssociation/rogilinkFlex-ros2)
- [rogilinkFlex-esp32](https://github.com/KeioRoboticsAssociation/rogilinkFlex-esp32) (動作未確認)

## 使い方(UART通信)

### 初期化
```cpp
#include "UartLink.hpp"

UartLink uart(&huart, デバイスID); // UART通信の設定
```


### 受信割り込みの設定
```cpp
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == UARTx) {
        uart.receive();
    }
}
```

### 通信開始


```cpp
uart.start(); // UART通信の開始
```


### データの送信
```cpp
UartLinkPublisher<データ型1, データ型2, ...> pub(uart, フレームID); // publisherの設定

pub.send(データ1, データ2, ...);
```

### データの受信
```cpp
UartLinkSubscriber<データ型1, データ型2, ...> sub(uart, フレームID); // subscriberの設定

void callback(データ型1, データ型2, ...) {
    // コールバック関数
}

sub.set_callback(callback); // コールバック関数の設定
```


## 使い方(RS485通信)

当然、UartLinkをRS485として使うこともできますが、1対nでRS485通信用する場合の専用クラスも用意しています。中身はUartLinkをラップしているだけです。

### 初期化
```cpp
#include "RS485Link.hpp"

RS485Link rs485(&huart, デバイスID); // RS485通信の設定
```


### 受信割り込みの設定
```cpp
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == UARTx) {
        rs485.receive();
    }
}
```

### 通信開始

```cpp
rs485.start(); // RS485通信の開始
```


### データの送信
```cpp
RS485LinkPublisher<データ型1, データ型2, ...> pub(rs485, フレームID); // publisherの設定

pub.send(送信先のID, データ1, データ2, ...);
```

### データの受信
```cpp
RS485LinkSubscriber<データ型1, データ型2, ...> sub(rs485, フレームID); // subscriberの設定

void callback(データ型1, データ型2, ...) {
    // コールバック関数
}

sub.set_callback(callback); // コールバック関数の設定
```


## 高度な使い方

`serialize`、`deserialize`関数を実装すれば、自作の構造体やクラスを送受信することができます。

### シリアライザ、デシリアライザの設定

デフォルトでは、送受信するデータ型として使えるのは、
- `int`, `float`, `char`,...などの基本型
- `char*`型(文字列)

のみです。ただし、構造体やクラスなどの複合型を送受信する場合は、シリアライザ(送信時)、デシリアライザ(受信時)を設定することで可能になります。

```cpp
struct Data {
    int a;
    float b;
    char c;

    // シリアライザ
    void serialize(uint8_t* data) {
        memcpy(data, &a, sizeof(int));
        memcpy(data + sizeof(int), &b, sizeof(float));
        memcpy(data + sizeof(int) + sizeof(float), &c, sizeof(char));
    }

    // デシリアライザ
    void deserialize(const uint8_t* data, size_t* size) {
        memcpy(&a, data, sizeof(int));
        memcpy(&b, data + sizeof(int), sizeof(float));
        memcpy(&c, data + sizeof(int) + sizeof(float), sizeof(char));
        *size = sizeof(int) + sizeof(float) + sizeof(char);
    };
};

UartLinkSubscriber<Data> sub(uart, 1); // Subscriber(受信側)の設定
```
シリアライザ・デシリアライザを設定しないとコンパイルエラーになります。
