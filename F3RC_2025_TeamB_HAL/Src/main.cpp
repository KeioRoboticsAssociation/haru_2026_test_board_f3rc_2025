#include "Encoder.hpp"
#include "UartLink.hpp"
#include "main.hpp"
#include "stm32f4xx_hal.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim5;
extern TIM_HandleTypeDef htim8;
extern UART_HandleTypeDef huart2;

// ---- 対ROS通信 ---- (不要な場合はUartLink周りを削除)
UartLink uart_link(&huart2, 0);

// UART受信割り込み
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        uart_link.interrupt();
    }
}

// GPIO割り込み
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    
}

// 宣言
Encoder* encoder1;
Encoder* encoder3;
Encoder* encoder4;

const int16_t ENCODER_RESOLUTION = 8192;


void setup() {
  // ここに初期化処理を書く
    
    // エンコーダー初期化（TIM1, TIM2, TIM3のエンコーダーモード）
    encoder1 = new Encoder(&htim2, ENCODER_RESOLUTION);
    encoder3 = new Encoder(&htim3, ENCODER_RESOLUTION);
    encoder4 = new Encoder(&htim8, ENCODER_RESOLUTION);

    encoder1->start();
    encoder3->start();
    encoder4->start();

    uart_link.start(); // ros2との通信を開始
}

void loop() {
  // ここに繰り返し処理を書く
  UartLinkPublisher<int32_t,int32_t,int32_t> pub(uart_link, 1);
  pub.publish(-encoder1->getRawCount(),-encoder3->getRawCount(),-encoder4->getRawCount());
}