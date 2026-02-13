#pragma once
#include <cstdint>
#include "main.h"

class Servo {
public:
  // pulseMin/pulseMax は省略可能（デフォルト 600 / 2100）
  Servo(TIM_HandleTypeDef* htim,
        uint32_t channel,
        uint16_t pulseMinUs = 600,
        uint16_t pulseMaxUs = 2100,
        bool invert = false,
        float gear_ratio = 1.0f,
        uint8_t device_id = 0);

  void begin();                             // PWM開始＋初期位置(10°)
  void setAngleRange(float minDeg, float maxDeg); // 有効角度範囲
  // 減速比を設定（>0）。既定値は 1.0（等倍）。
  void setGearRatio(float ratio = 1.0f);
  void setAngle(float deg);                 // 任意角度
  void setPulse(uint32_t us);               // 任意パルス

  // ハンド開閉用
  void setHandAngles(float openDeg, float closeDeg); // 角度設定
  void setHand(bool open);                  // true=10°, false=72°
  void openHand();                          // 10°
  void closeHand();                         // 72°

  // 現在出力中のパルス幅[us]（CCR値）を取得
  uint32_t getPulse() const;

private:
  uint32_t angleToPulse(float deg) const;
  static float clamp(float v, float lo, float hi);

  TIM_HandleTypeDef* htim_;
  uint32_t ch_;
  uint16_t pulseMin_, pulseMax_;
  bool invert_;
  float angleMin_, angleMax_;
  float gear_ratio_ = 1.0f; // 外部角度→サーボ角度の倍率（減速比）。1.0で等倍。
  // 事前計算で除算を削減
  float span_inv_ = 0.0f;     // 1/(angleMax-angleMin)
  float pulse_span_ = 0.0f;   // (pulseMax - pulseMin)

  float handOpen_;   // 10°
  float handClose_;  // 72°
  uint8_t device_id_ = 0;
public:
  uint8_t deviceID() const { return device_id_; }
};