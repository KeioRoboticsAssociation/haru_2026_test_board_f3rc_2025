#include "Servo.hpp"

// サーボPWMの初期化（範囲/反転/開閉角を設定）
Servo::Servo(TIM_HandleTypeDef* htim,
             uint32_t channel,
             uint16_t pulseMinUs,
             uint16_t pulseMaxUs,
             bool invert,
             float gear_ratio,
             uint8_t device_id)
  : htim_(htim),
    ch_(channel),
  pulseMin_(pulseMinUs),
  pulseMax_(pulseMaxUs),
  invert_(invert),
  angleMin_(-90.0f),
  angleMax_( 90.0f),
  gear_ratio_(gear_ratio),
  handOpen_(10.0f),    // true
  handClose_(72.0f)    // false
{
  device_id_ = device_id;
  pulse_span_ = static_cast<float>(pulseMax_) - static_cast<float>(pulseMin_);
  const float span = (angleMax_ - angleMin_);
  span_inv_ = (span != 0.0f) ? (1.0f / span) : 0.0f;
}

// PWM出力開始と初期角度（open）適用
void Servo::begin() {
  HAL_TIM_PWM_Start(htim_, ch_);
  // 初期は open=true → 10°
  setAngle(0.0f);
}

// 入力角度レンジ[deg]を設定
void Servo::setAngleRange(float minDeg, float maxDeg) {
  if (minDeg > maxDeg) { float t = minDeg; minDeg = maxDeg; maxDeg = t; }
  angleMin_ = minDeg;
  angleMax_ = maxDeg;
  const float span = (angleMax_ - angleMin_);
  span_inv_ = (span != 0.0f) ? (1.0f / span) : 0.0f;
}

// 減速比を設定（<=0 は 1.0 とみなす）
void Servo::setGearRatio(float ratio) {
  if (ratio <= 0.0f) ratio = 1.0f;
  gear_ratio_ = ratio;
}

// 角度[deg]を指定して出力（clamp+CCR更新）
void Servo::setAngle(float deg) {
  // 事前クランプは行わず、angleToPulse 内の飽和に一任する
  // （外部角度→サーボ角度の変換後に正しく端で飽和させるため）
  __HAL_TIM_SET_COMPARE(htim_, ch_, angleToPulse(deg));
}

// パルス幅[us]を直接指定
void Servo::setPulse(uint32_t us) {
  if (us < pulseMin_) us = pulseMin_;
  if (us > pulseMax_) us = pulseMax_;
  __HAL_TIM_SET_COMPARE(htim_, ch_, us);
}

// ハンドの開閉角度を設定
void Servo::setHandAngles(float openDeg, float closeDeg) {
  handOpen_  = openDeg;
  handClose_ = closeDeg;
}

// ハンド開閉（true=open, false=close）
void Servo::setHand(bool open) {
  const float target = open ? handOpen_ : handClose_;
  setAngle(target);
}

// ショートカット：開/閉
void Servo::openHand()  { setHand(true);  }
void Servo::closeHand() { setHand(false); }

// 角度[deg]→パルス幅[us]へ線形変換（反転対応）
uint32_t Servo::angleToPulse(float deg) const {
  // 外部角度[deg] → サーボ角度[deg]
  // - 外部角度の中心を基準に、(deg-center)*gear_ratio_ でサーボ角度を拡大/縮小
  // - マッピング自体は [angleMin_, angleMax_] をサーボ角度レンジとみなして線形変換
  if (span_inv_ == 0.0f) return pulseMin_;
  const float center = 0.5f * (angleMin_ + angleMax_);
  const float servo_deg = center + (deg - center) * gear_ratio_;
  // 除算削減: 事前計算した1/spanで乗算
  float t = (servo_deg - angleMin_) * span_inv_;
  if (t < 0.f) t = 0.f; else if (t > 1.f) t = 1.f;
  if (invert_) t = 1.f - t;

  // 除算は無く、掛け算と加算のみ
  float us = static_cast<float>(pulseMin_) + t * pulse_span_;
  if (us < pulseMin_) us = static_cast<float>(pulseMin_);
  if (us > pulseMax_) us = static_cast<float>(pulseMax_);
  return static_cast<uint32_t>(us + 0.5f);
}

// 値を[min,max]に収めるヘルパ
float Servo::clamp(float v, float lo, float hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

// 現在のCCR（パルス幅[us]）を返す
uint32_t Servo::getPulse() const {
  return __HAL_TIM_GET_COMPARE(htim_, ch_);
}