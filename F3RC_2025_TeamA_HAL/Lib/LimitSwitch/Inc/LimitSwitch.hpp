#pragma once
#include "stm32f4xx_hal.h"

class LimitSwitch {
public:
  LimitSwitch(GPIO_TypeDef* port, uint16_t pin);
  // 立下りエッジ（押した瞬間）で true を1回返す
  bool fell();

private:
  GPIO_TypeDef* port_;
  uint16_t pin_;
  GPIO_PinState prev_;
};