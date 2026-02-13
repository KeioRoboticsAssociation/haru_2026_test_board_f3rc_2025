#include "Encoder.hpp"
#include "main.h"
#include <cmath>

Encoder::Encoder(TIM_HandleTypeDef *htim, int16_t resolution) {
  this->htim = htim;
  this->resolution = resolution;
  this->count = 0;
  this->prev_count = 0;
}

void Encoder::start() {
  HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
  if (htim->Init.Period != 0xFFFF && htim->Init.Period != 0xFFFFFFFF) {
    // 16bit or 32bit ではなければエラーを起こす
    Error_Handler();
  }
}

__RAM_FUNC int32_t Encoder::getRawCount() {
  uint32_t current_count_raw = __HAL_TIM_GET_COUNTER(htim);
  int32_t diff;

  // タイマーの分解能で判定
  if (htim->Init.Period == 0xFFFFFFFF) {
    // 32bitタイマー
    diff = (int32_t)(current_count_raw - (uint32_t)prev_count);
  } else {
    // 16bitタイマー
    diff = (int32_t)((int16_t)((uint16_t)current_count_raw - (uint16_t)prev_count));
  }

  prev_count = current_count_raw;
  count += diff;
  return count;
}

float Encoder::getRotations() {
  return (float)getRawCount() / (float)resolution;
}

float Encoder::getDegrees() { return getRotations() * 360.0f; }

float Encoder::getRadians() { return getRotations() * 2.0f * M_PI; }

void Encoder::setRawCount(int32_t count) {
  this->count = count;
  __HAL_TIM_SET_COUNTER(htim, 0); // bufferをリセット
}

void Encoder::setRotations(float rotations) {
  setRawCount((int32_t)(rotations * resolution));
}

void Encoder::setDegrees(float degrees) { setRotations(degrees / 360.0f); }

void Encoder::setRadians(float radians) {
  setRotations(radians / (2.0f * M_PI));
}