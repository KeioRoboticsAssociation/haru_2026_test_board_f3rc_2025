#include "LimitSwitch.hpp"
#include "stdio.h"

// ボタン入力の初期化（前回状態も保持）
LimitSwitch::LimitSwitch(GPIO_TypeDef* port, uint16_t pin)
  : port_(port), pin_(pin), prev_(GPIO_PIN_SET) {}

// 立下り（押下）エッジ検出（押された瞬間にtrue）
bool LimitSwitch::fell() {
  GPIO_PinState now = HAL_GPIO_ReadPin(port_, pin_);
  bool edge = (prev_ == GPIO_PIN_SET && now == GPIO_PIN_RESET);
  prev_ = now;
  if(edge) {
    // ボタンが押されたときの処理
    printf("LimitSwitch pressed\n"); // デバッグ用出力
  }
  return edge;
}
