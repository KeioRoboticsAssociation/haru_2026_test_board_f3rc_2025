#pragma once
#include "main.h"
#include <cstdint>

class DCMotor {
public:
  DCMotor(TIM_HandleTypeDef *htim, uint16_t channel, GPIO_TypeDef *GPIO_Port,
          uint16_t GPIO_Pin, bool direction = 1, float max_duty = 1.0f);
  void setDuty(float duty);
  void start();
  float getDuty() const { return current_duty; }
  void setMaxDuty(float max_duty) { this->max_duty = max_duty; }

private:
  void setDirection(bool direction);
  TIM_HandleTypeDef *htim;
  uint16_t channel;
  bool direction;
  float current_duty;
  float max_duty;
  uint16_t pwm_resolution;

  GPIO_TypeDef *GPIO_Port;
  uint16_t GPIO_Pin;
};