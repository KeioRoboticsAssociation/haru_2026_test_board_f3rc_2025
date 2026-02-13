#include "DCMotor.hpp"

DCMotor::DCMotor(TIM_HandleTypeDef *htim, uint16_t channel,
                 GPIO_TypeDef *GPIO_Port, uint16_t GPIO_Pin, bool direction,
                 float max_duty) {
  this->htim = htim;
  this->channel = channel;
  this->direction = direction;
  this->GPIO_Port = GPIO_Port;
  this->GPIO_Pin = GPIO_Pin;
  this->max_duty = max_duty < 1.0f ? max_duty : 1.0f;
  this->pwm_resolution = 0;
  this->current_duty = 0.0f;
}

void DCMotor::start() {
  if (htim == nullptr) {
    return;
  }
  HAL_TIM_PWM_Start(htim, channel);
  pwm_resolution = htim->Init.Period;
}

__RAM_FUNC void DCMotor::setDuty(float duty) {
  if (htim == nullptr || pwm_resolution == 0) {
    return;
  }

  // まず上下限を確認
  if (duty > max_duty) {
    duty = max_duty;
  } else if (duty < -max_duty) {
    duty = -max_duty;
  }

  current_duty = duty; // オーバーしない値を保存

  // 方向とPWM値を設定
  const bool dir = (duty >= 0.0f);
  const float abs_duty = dir ? duty : -duty;

  setDirection(dir);

  // 通常のPWM
  uint32_t value = static_cast<uint32_t>(pwm_resolution * (abs_duty));
  __HAL_TIM_SET_COMPARE(htim, channel, value);
}

void DCMotor::setDirection(bool dir) {
  if (GPIO_Port == nullptr) {
    return;
  }
  // directionフラグでXORして反転可能に
  bool pin_state = (dir == this->direction);
  HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin,
                    pin_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}