# リミットスイッチのRogiLinkFlex送信設計

## 1. 送信データの設計

### データ構造

5つのリミットスイッチ（LSW_1〜LSW_5）の現在の**ON/OFF状態**を **`uint8_t` 1バイトにビットパック**して送信します。

| bit | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|
| 内容 | LSW_5 | LSW_4 | LSW_3 | LSW_2 | LSW_1 |

- `0` = 未押下 (HIGH)、`1` = 押下中 (LOW)
- 例：LSW_1とLSW_3が押されている → `0b00000101` = `5`

> [!TIP]
> `fell()`（エッジ検出）ではなく **現在の状態**を送ります。エッジ検出はROS側で前回値と比較して行う方が、通信の取りこぼしに強く信頼性が高いためです。

### メッセージID

既存の `encoder_counts` が **id: 1** を使用しているため、リミットスイッチは **id: 2** を使用します。

### 送信タイミング

エンコーダーと同じ **10ms周期** で送信します。既存の `if(now - last_send_time >= 10)` ブロック内に追加するだけです。

---

## 2. config.json への加筆

`reception_messages` にリミットスイッチ用のエントリを追加します：

```json
{
  "devices": [
    {
      "device_id": 0,
      "reception_messages": [
        {
          "id": 1,
          "name": "encoder_counts",
          "type": "(c_int64, c_int64, c_int64)"
        },
        {
          "id": 2,
          "name": "limit_switches",
          "type": "c_uint8",
          "args": "packed_lsw"
        }
      ],
      "transmission_messages": []
    }
  ]
}
```

ROS側のnode.pyでは以下のように受信できます：

```python
from rogilink_flex_lib import Subscriber
from ctypes import c_uint8

self.sub_lsw = Subscriber(self, 'limit_switches', c_uint8, self.lsw_callback)

def lsw_callback(self, msg):
    lsw1 = bool(msg & 0x01)
    lsw2 = bool(msg & 0x02)
    lsw3 = bool(msg & 0x04)
    lsw4 = bool(msg & 0x08)
    lsw5 = bool(msg & 0x10)
```

---

## 3. HAL側 main.cpp への実装方法

### 変更箇所まとめ

#### (A) Publisher宣言を追加（16行目付近）

```cpp
UartLinkPublisher<int32_t, int32_t, int32_t> encoder_pub(uart_link, 1);
UartLinkPublisher<uint8_t> lsw_pub(uart_link, 2);  // ← 追加
```

#### (B) 現在の状態を読む関数を追加（57行目付近、setup()の前）

`LimitSwitch::fell()` はエッジ検出なので、状態送信には直接 `HAL_GPIO_ReadPin` を使います。

```cpp
// リミットスイッチの現在値をビットパックして返す
uint8_t readLswPacked() {
  uint8_t packed = 0;
  // GPIO_PIN_RESET(LOW) = 押下中 → bitを1にする
  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET) packed |= (1 << 0); // LSW_1
  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11) == GPIO_PIN_RESET) packed |= (1 << 1); // LSW_2
  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET) packed |= (1 << 2); // LSW_3
  if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4)  == GPIO_PIN_RESET) packed |= (1 << 3); // LSW_4
  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6)  == GPIO_PIN_RESET) packed |= (1 << 4); // LSW_5
  return packed;
}
```

#### (C) loop()内の送信ブロックに追記（85行目付近）

```cpp
if (now - last_send_time >= 10) {
    last_send_time = now;
    encoder_pub.publish(-encoder1->getRawCount(), -encoder2->getRawCount(),
                        encoder3->getRawCount());
    lsw_pub.publish(readLswPacked());  // ← 追加
}
```

### 変更後の main.cpp（差分のみ）

```diff
 UartLinkPublisher<int32_t, int32_t, int32_t> encoder_pub(uart_link, 1);
+UartLinkPublisher<uint8_t> lsw_pub(uart_link, 2);

+// リミットスイッチの現在値をビットパックして返す
+uint8_t readLswPacked() {
+  uint8_t packed = 0;
+  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET) packed |= (1 << 0);
+  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11) == GPIO_PIN_RESET) packed |= (1 << 1);
+  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET) packed |= (1 << 2);
+  if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4)  == GPIO_PIN_RESET) packed |= (1 << 3);
+  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6)  == GPIO_PIN_RESET) packed |= (1 << 4);
+  return packed;
+}

   if (now - last_send_time >= 10) {
     last_send_time = now;
     encoder_pub.publish(-encoder1->getRawCount(), -encoder2->getRawCount(),
                         encoder3->getRawCount());
+    lsw_pub.publish(readLswPacked());
   }
```

> [!NOTE]
> `LimitSwitch` クラスの `fell()` はエッジ検出（押した瞬間だけtrue）なので、ROS側への状態送信には不向きです。代わりに `HAL_GPIO_ReadPin` で直接読み取り、ビットパックして送信しています。`fell()` はマイコン内でのローカルな制御（モーター停止など）に使えます。
