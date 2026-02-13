# UART通信不安定問題の分析レポート

> **対象**: F3RC_2025_TeamB_HAL プロジェクト（STM32F446RE NUCLEO）
> **症状**: ThinkPadでは安定通信、ミニPCでは不安定
> **日付**: 2026-02-13

---

## 結論: HALに問題はあるか？

**はい、HAL側のコードに問題があります。** ただし、HALライブラリ自体のバグではなく、**HALの使い方（RogiLinkFlexライブラリの実装）に複数の問題**があります。ThinkPadで安定しているのは、ThinkPad側のUSBコントローラやOS側のタイミングが「たまたま」許容範囲に収まっているだけで、本質的には同じ問題を抱えています。

---

## 発見された問題点

### 🔴 問題1【致命的】: 送信がブロッキング（最大50msのCPU停止）

**ファイル**: `Lib/RogiLinkFlex/Src/UartLink.cpp` 18行目

```cpp
void UartLink::send_raw(uint8_t* data, uint8_t size)
{
    HAL_UART_Transmit(huart_ptr, data, size, 50);  // ← 最大50msブロック!
}
```

**何が問題か:**
- `HAL_UART_Transmit`はポーリング送信。送信完了まで**CPUが完全に停止**する
- 送信中にRX割り込みが来た場合、割り込みの処理は可能だが、次の`HAL_UART_Receive_IT`が内部でロックと競合する可能性がある
- 115200bpsでint32_t×3 = 12バイト + ヘッダ + COBS = 約20バイトの送信にかかる時間は約1.7ms。これ自体は短いが、**loopが全速力で回っている**ため（問題3参照）、送信が連続し、受信処理が追いつかなくなる

**影響**: ミニPCがThinkPadより高速にデータを送信してきた場合、ブロッキング送信中にRXバッファが溢れて受信データが失われる

---

### 🔴 問題2【致命的】: UARTエラー時の復帰処理がない

**ファイル**: `Lib/RogiLinkFlex/Src/UartLink.cpp`, `Src/main.cpp`

```cpp
void UartLink::start()
{
    HAL_UART_Receive_IT(huart_ptr, &receive_buffer[0], 1);
    // エラーが発生したら？ → 何もしない → 通信が永久に停止
}
```

**何が問題か:**
- UART通信では、ノイズやタイミングの問題で以下のエラーが頻繁に発生する:
  - **Overrun Error (ORE)**: 受信データがDRレジスタから読まれる前に次のデータが来た
  - **Framing Error (FE)**: ストップビットが検出されなかった
  - **Noise Error (NE)**: ノイズが検出された
- HALはエラー発生時、UARTを`HAL_UART_STATE_READY`にリセットし、**以降の`HAL_UART_Receive_IT`を自動的に再開しない**
- `HAL_UART_ErrorCallback`が未実装のため、**一度エラーが起きると、受信が永久に停止する**
- ミニPCのUSBシリアルコントローラはThinkPadと異なるチップを使っている可能性が高く、微妙なタイミング差でOverrunErrorが発生しやすい

> [!CAUTION]
> **これが最も可能性の高い直接原因です。** ミニPCのUSB-シリアル変換チップのタイミング特性の違いにより、ORE（Overrun Error）が発生し、受信が永久停止していると考えられます。

---

### 🟡 問題3【重要】: loop()に送信レート制限がない

**ファイル**: `Src/main.cpp` 52-56行目

```cpp
void loop() {
    UartLinkPublisher<int32_t,int32_t,int32_t> pub(uart_link, 1);
    pub.publish(-encoder1->getRawCount(),-encoder3->getRawCount(),-encoder4->getRawCount());
    // ← 何のwaitもなく、全速力で送信し続ける
}
```

**何が問題か:**
- CPU 84MHzで`loop()`が回ると、**1秒間に数千回〜数万回**送信を試みる
- 115200bpsでは物理的に1秒間に約5760バイトしか送れないため、`HAL_UART_Transmit`のブロッキング待ちが頻発する
- ROS側が処理しきれないほどのデータが流れ込み、USB-シリアルバッファが溢れる
- さらに、`UartLinkPublisher`をloop()のたびにスタック上に毎回生成しているのは無駄（致命的ではないが非効率）

---

### 🟡 問題4【重要】: NVIC優先度グループの設定

**ファイル**: `F3RC_TEAMB_HAL.ioc` 50行目

```
NVIC.PriorityGroup=NVIC_PRIORITYGROUP_0
```

**何が問題か:**
- `NVIC_PRIORITYGROUP_0`はプリエンプション優先度ビット数が0 = **割り込みの横取り（ネスト）が一切不可能**
- SysTick割り込みとUSART2割り込みが同じプリエンプション優先度になる
- `HAL_UART_Transmit`内部で`HAL_GetTick()`を使ってタイムアウトを管理するが、SysTickがUSART2割り込みにプリエンプトされた場合に正確に動作しなくなる

---

### 🟡 問題5【中程度】: 割り込みコンテキストでの重い処理

**ファイル**: `Lib/RogiLinkFlex/Src/UartLink.cpp` 22-41行目, `Lib/RogiLinkFlex/Src/CommunicationBase.cpp` 43-52行目

```cpp
void UartLink::interrupt() {
    // ... 
    if (c == 0x00) {
        // ISR内でmemcpy → COBSデコード → ヘッダ解析 → std::mapルックアップ → コールバック呼び出し
        memcpy(copied, receive_buffer, receive_buffer_index);
        on_receive_raw(copied, receive_buffer_index);  // ← ISR内で全処理を実行!
    }
}
```

**何が問題か:**
- `on_receive_raw` → `decode_cobs` → `remove_header` → `on_receive` → `callback_map[frame_id]` → コールバック呼び出し、これら全てが**UART受信割り込み(ISR)コンテキスト内で実行**される
- `std::map`のルックアップは`O(log n)`で比較的重い（動的メモリアロケーションの可能性もある）
- ISRが長いと、次のUARTバイトの受信開始（`HAL_UART_Receive_IT`）が遅れ、Overrun Errorが発生する

---

## DMAやTIMを用いた非ブロッキング処理は必要か？

### 短期的には不要

上記の問題のうち、**問題1〜3を修正するだけで通信は大幅に安定化する**と考えられます。DMAは最終的な最適解ですが、RogiLinkFlexライブラリの構造変更が必要になるため、まずは以下の最小限の修正を推奨します。

### 長期的にはDMA化を推奨

将来的にデータ量が増える場合や、より高いボーレート（例: 921600bps）を使う場合は、DMA受信への移行を強く推奨します。

---

## 推奨修正（優先度順）

### 修正A【最優先】: UARTエラーコールバックの追加

`Src/main.cpp`に以下を追加するだけで、最大の問題が解決する可能性が高い:

```cpp
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        // エラーフラグをクリア（HALが自動でやるが念のため）
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        
        // 受信を再開（バッファインデックスもリセットされる）
        uart_link.start();
    }
}
```

**効果**: ORE等のエラーが発生しても、受信が自動的に再開される

---

### 修正B【重要】: 送信レート制限の追加

`Src/main.cpp`の`loop()`を修正:

```cpp
// Publisherをグローバルスコープに移動
UartLinkPublisher<int32_t,int32_t,int32_t> pub(uart_link, 1);

void loop() {
    static uint32_t last_send_time = 0;
    uint32_t now = HAL_GetTick();
    
    if (now - last_send_time >= 10) {  // 10ms間隔 = 100Hz
        last_send_time = now;
        pub.publish(-encoder1->getRawCount(), -encoder3->getRawCount(), -encoder4->getRawCount());
    }
}
```

**効果**: 
- 送信頻度が100Hzに制限され、ROS側が安定して処理できる
- CPUがブロッキング送信に費やす時間が激減する
- エンコーダの値を100Hzで送信するのはロボット制御として十分（必要に応じて調整可能）

---

### 修正C【推奨】: 送信の非ブロッキング化

`Lib/RogiLinkFlex/Src/UartLink.cpp`の`send_raw`を修正:

```cpp
void UartLink::send_raw(uint8_t* data, uint8_t size)
{
    // ブロッキング版（現行）
    // HAL_UART_Transmit(huart_ptr, data, size, 50);
    
    // 非ブロッキング版（割り込み送信）
    // 注意: 送信完了前に次のsendが呼ばれないよう、送信レート制限(修正B)が前提
    while (huart_ptr->gState != HAL_UART_STATE_READY) {
        // 前回の送信が完了するまで待機（短時間）
    }
    HAL_UART_Transmit_IT(huart_ptr, data, size);
}
```

> [!WARNING]
> 修正Cは**ライブラリファイルの変更**が必要です。修正AとBだけで改善されるか、まず試してから検討するのが良いでしょう。

---

### 修正D【参考】: NVIC優先度グループの変更

CubeMXで `NVIC_PRIORITYGROUP_4` に変更することで、割り込みのプリエンプションが有効になります。ただし、他のペリフェラルに影響がないか確認が必要です。

---

## まとめ

| 問題 | 深刻度 | 修正難易度 | 修正箇所 |
|------|--------|-----------|---------|
| UARTエラー復帰なし | 🔴 致命的 | ⭐ 簡単 | `main.cpp`に5行追加 |
| 送信レート制限なし | 🟡 重要 | ⭐ 簡単 | `main.cpp`のloop修正 |
| ブロッキング送信 | 🔴 致命的 | ⭐⭐ 中程度 | ライブラリ変更が必要 |
| ISR内での重い処理 | 🟡 中程度 | ⭐⭐⭐ 難しい | ライブラリの設計変更が必要 |
| NVIC優先度グループ | 🟡 中程度 | ⭐ 簡単 | CubeMXで変更 |

### 推奨される対応手順

1. **まず修正AとBだけ実施**（`main.cpp`のみの変更で済む）
2. ミニPCで通信テスト
3. 改善されればそれで完了。改善されなければ修正C以降を検討
4. 長期的にはDMA受信化を検討

> [!IMPORTANT]
> **修正AとBは`main.cpp`のみの変更で、ライブラリには手を加えません。** ROS担当者に「これで試してみてほしい」と伝えやすい最小構成です。
