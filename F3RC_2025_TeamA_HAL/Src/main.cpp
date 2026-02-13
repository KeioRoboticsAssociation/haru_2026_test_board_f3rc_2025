#include "main.hpp"
#include "Encoder.hpp"
#include "LimitSwitch.hpp"
#include "UartLink.hpp"
#include "stm32f4xx_hal.h"
#include <cstdint>

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern UART_HandleTypeDef huart2;

// ---- 対ROS通信 ---- (不要な場合はUartLink周りを削除)
UartLink uart_link(&huart2, 0);
// EncoderのPub
UartLinkPublisher<int32_t, int32_t, int32_t> encoder_pub(uart_link, 1);
// LSWのPub
UartLinkPublisher<uint8_t> lsw_pub(uart_link, 2);

// UART受信割り込み
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART2) {
    uart_link.interrupt();
  }
}

// エラーコールバックの処理
static uint32_t uart_error_count = 0;
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART2) {
    // エラーフラグをクリア（HALが自動でやるが念のため）
    uart_error_count++;
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    __HAL_UART_CLEAR_PEFLAG(huart);

    // 受信を再開（バッファインデックスもリセットされる）
    uart_link.start();
  }
}

// GPIO割り込み
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {}

// Encoder宣言
Encoder *encoder1;
Encoder *encoder2;
Encoder *encoder3;

const int16_t ENCODER_RESOLUTION = 8192;

// LSW 宣言
LimitSwitch *lsw1;
LimitSwitch *lsw2;
LimitSwitch *lsw3;
LimitSwitch *lsw4;
LimitSwitch *lsw5;

// リミットスイッチの現在値をビットパックして返す
uint8_t readLswPacked() {
  uint8_t packed = 0;
  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET)
    packed |= (1 << 0);
  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11) == GPIO_PIN_RESET)
    packed |= (1 << 1);
  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET)
    packed |= (1 << 2);
  if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4) == GPIO_PIN_RESET)
    packed |= (1 << 3);
  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_RESET)
    packed |= (1 << 4);
  return packed;
}

void setup() {
  // ここに初期化処理を書く

  // エンコーダー初期化（TIM1, TIM2, TIM3のエンコーダーモード）
  encoder1 = new Encoder(&htim3, ENCODER_RESOLUTION);
  encoder2 = new Encoder(&htim4, ENCODER_RESOLUTION);
  encoder3 = new Encoder(&htim1, ENCODER_RESOLUTION);

  encoder1->start();
  encoder2->start();
  encoder3->start();

  // リミットスイッチ初期化
  lsw1 = new LimitSwitch(GPIOA, GPIO_PIN_15); // LSW_1: PA15
  lsw2 = new LimitSwitch(GPIOA, GPIO_PIN_11); // LSW_2: PA11
  lsw3 = new LimitSwitch(GPIOB, GPIO_PIN_12); // LSW_3: PB12
  lsw4 = new LimitSwitch(GPIOC, GPIO_PIN_4);  // LSW_4: PC4
  lsw5 = new LimitSwitch(GPIOA, GPIO_PIN_6);  // LSW_5: PA6

  uart_link.start(); // ros2との通信を開始
}

void loop() {
  // ここに繰り返し処理を書く
  static uint32_t last_send_time = 0;
  uint32_t now = HAL_GetTick();

  if (now - last_send_time >= 10) {
    // 10ms間隔で送信処理
    last_send_time = now;
    // ★★基板の配線上、TeamAボードのEncoder3のみ値を反転して送信中★★
    encoder_pub.publish(-encoder1->getRawCount(), -encoder2->getRawCount(),
                        encoder3->getRawCount());
    lsw_pub.publish(readLswPacked());
  }
}