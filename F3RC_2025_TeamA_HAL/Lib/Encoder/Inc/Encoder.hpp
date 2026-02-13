#pragma once

#include "main.h"
#include <stdint.h>

class Encoder {
public:
  Encoder(TIM_HandleTypeDef *htim, int16_t resolution);
  void start();

  // カウント取得
  int32_t getRawCount();
  float getRotations();
  float getDegrees();
  float getRadians();

  // リセット
  void setRawCount(int32_t count);
  void setRotations(float rotations);
  void setDegrees(float degrees);
  void setRadians(float radians);

  void setResolution(int16_t resolution) { this->resolution = resolution; }

private:
  TIM_HandleTypeDef *htim;
  int16_t resolution;
  int32_t count;

  int32_t prev_count;
};