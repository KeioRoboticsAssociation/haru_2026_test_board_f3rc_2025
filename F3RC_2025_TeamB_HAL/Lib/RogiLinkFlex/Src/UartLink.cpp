#include "UartLink.hpp"
#include <string.h>

// コンストラクタ
UartLink::UartLink(UART_HandleTypeDef* huart, uint8_t device_id): CobsEncodedCommunicationBase(device_id)
{
    huart_ptr = huart;
}

void UartLink::start()
{
    HAL_UART_Receive_IT(huart_ptr, &receive_buffer[0], 1);
}

// 送信処理
void UartLink::send_raw(uint8_t* data, uint8_t size)
{
    HAL_UART_Transmit(huart_ptr, data, size, 50);
}

// 受信割り込み
void UartLink::interrupt()
{
    uint8_t c = receive_buffer[receive_buffer_index];
    receive_buffer_index++;

    if (receive_buffer_index >= BUFFER_SIZE - 6) {
        // バッファがオーバーフローしたらリセット
        receive_buffer_index = 0;
        return;
    }

    if (c == 0x00) {
        HAL_UART_Receive_IT(huart_ptr, &receive_buffer[0], 1); // 次の受信を開始
        uint8_t copied[BUFFER_SIZE]; // 受信バッファのコピー
        memcpy(copied, receive_buffer, receive_buffer_index); // 受信バッファをコピー
        on_receive_raw(copied, receive_buffer_index); // 受信処理
        receive_buffer_index = 0; // 受信バッファをリセット
    }else{
        HAL_UART_Receive_IT(huart_ptr, &receive_buffer[receive_buffer_index], 1); // 次の受信を開始
    } 
}

