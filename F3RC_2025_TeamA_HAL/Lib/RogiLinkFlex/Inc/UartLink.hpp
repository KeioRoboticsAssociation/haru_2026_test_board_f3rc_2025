#pragma once

#include "CommunicationBase.hpp"
#include "main.h"

// UartLinkクラス。CobsEncodedCommunicationBaseを継承
class UartLink : public CobsEncodedCommunicationBase {
    public:
        UartLink(UART_HandleTypeDef* huart, uint8_t device_id=0);

        void start();

        // 送信処理
        void send_raw(uint8_t* data, uint8_t size) override;

        // 受信割り込み時に呼ぶ
        void interrupt();

    private:
        

        uint8_t receive_buffer[BUFFER_SIZE];
        uint8_t receive_buffer_index = 0;

        UART_HandleTypeDef* huart_ptr;
};

// Publisher
template<typename... Args>
using UartLinkPublisher = Publisher<UartLink, Args...>;

// Subscriber
template<typename... Args>
using UartLinkSubscriber = Subscriber<UartLink, Args...>;

