# gamepad.h / gamepad.cpp 使い方メモ

`framework/gamepad.cpp/.h` の使い方まとめ。

---

## 対応バックエンド

XInput / RawInput / DirectInput の3系統を自動切り替えする。
XInput が検出された場合は優先的にそちらを使い、それ以外はどちらか片方をアクティビティで選択する。

現在のバックエンドは `Gamepad_GetActiveBackend()` で確認できる。

| 値 | 意味 |
|---|---|
| `GAMEPAD_INPUT_BACKEND_NONE` | コントローラー未接続 |
| `GAMEPAD_INPUT_BACKEND_XINPUT` | XInput（Xbox コントローラー等）|
| `GAMEPAD_INPUT_BACKEND_RAWINPUT` | Raw Input（Switch Proコン等）|
| `GAMEPAD_INPUT_BACKEND_DIRECTINPUT` | DirectInput（旧ジョイスティック等）|

---

## ライフサイクル

```cpp
// 初期化（game の Init で呼ぶ）
Gamepad_Initialize();

// 毎フレーム更新（game の Update 先頭で呼ぶ）
Gamepad_Update();

// WndProc 内でウィンドウメッセージを渡す（RawInput に必要）
Gamepad_ProcessMessage(message, wParam, lParam);

// 終了（game の Finalize で呼ぶ）
Gamepad_Finalize();
```

---

## playerIndex について

XInput は最大4コントローラー（0〜3）。
RawInput / DirectInput のフォールバック時は常に `playerIndex = 0` に統合される。

```cpp
// 接続中の最初のプレイヤーを探す（-1 なら未接続）
int player = Gamepad_FindConnectedPlayer();

// 特定のインデックスが接続中か確認する
bool connected = Gamepad_IsConnected(0);
```

---

## ボタン入力

```cpp
// 押し続けている間 true
Gamepad_IsButtonDown(0, GPB_A);

// 押した瞬間だけ true（トリガー判定）
Gamepad_IsButtonTrigger(0, GPB_A);
```

### ボタン一覧（`Gamepad_Button` 列挙体）

| 定数 | ボタン |
|---|---|
| `GPB_A` | A ボタン |
| `GPB_B` | B ボタン |
| `GPB_X` | X ボタン |
| `GPB_Y` | Y ボタン |
| `GPB_DPAD_UP` | 十字キー上 |
| `GPB_DPAD_DOWN` | 十字キー下 |
| `GPB_DPAD_LEFT` | 十字キー左 |
| `GPB_DPAD_RIGHT` | 十字キー右 |
| `GPB_LEFT_SHOULDER` | LB / L |
| `GPB_RIGHT_SHOULDER` | RB / R |
| `GPB_START` | Start / + |
| `GPB_BACK` | Back / - |
| `GPB_LEFT_STICK` | 左スティック押し込み |
| `GPB_RIGHT_STICK` | 右スティック押し込み |

---

## スティック・トリガー入力

```cpp
// 左スティック（x: 左-1〜右+1、y: 下-1〜上+1）
Gamepad_ThumbStick ls = Gamepad_GetLeftStick(0);
ls.x;  // 左右
ls.y;  // 上下

// 右スティック（同上）
Gamepad_ThumbStick rs = Gamepad_GetRightStick(0);

// トリガー（0.0〜1.0、デッドゾーン処理済み）
float lt = Gamepad_GetLeftTrigger(0);
float rt = Gamepad_GetRightTrigger(0);
```

スティックにはデッドゾーン処理が済んでいるので、`0.0f` が返ってきたら「倒していない」と判断してよい。

---

## 振動

XInput バックエンド時のみ機能する。RawInput / DirectInput では無視される。

```cpp
// leftMotor: 低周波（0.0〜1.0）、rightMotor: 高周波（0.0〜1.0）
Gamepad_SetVibration(0, 0.5f, 0.5f);

// 振動停止
Gamepad_SetVibration(0, 0.0f, 0.0f);
```

---

## ボタンレイアウト

Switch と Xbox でボタンの物理配置が異なるため、レイアウトを指定する。
デフォルトは `GAMEPAD_LAYOUT_SWITCH_ABXY`（Switch レイアウト）になっている。

```cpp
// Switch Proコン向け（デフォルト）
Gamepad_SetLayout(GAMEPAD_LAYOUT_SWITCH_ABXY);

// Xbox コントローラー向け
Gamepad_SetLayout(GAMEPAD_LAYOUT_XBOX);
```

Switch レイアウト時は `GPB_A` / `GPB_B` の物理マッピングが入れ替わる（B=確定、A=キャンセル の習慣に合わせる）。

---

## 実装例（Player::Update 内）

```cpp
int player = Gamepad_FindConnectedPlayer();
if (player >= 0)
{
    Gamepad_ThumbStick ls = Gamepad_GetLeftStick(player);

    // 左右移動（床・天井面）
    if (ls.x < -0.5f) { /* 左レーンへ */ }
    if (ls.x >  0.5f) { /* 右レーンへ */ }

    // 上下移動（壁面）
    if (ls.y >  0.5f) { /* 左壁へ */ }
    if (ls.y < -0.5f) { /* 右壁へ */ }

    // 攻撃（押した瞬間）
    if (Gamepad_IsButtonTrigger(player, GPB_A)) { /* 攻撃 */ }

    // ホールド判定（押し続け）
    float lt = Gamepad_GetLeftTrigger(player);
    float rt = Gamepad_GetRightTrigger(player);
    bool holding = (lt > 0.5f || rt > 0.5f);

    // 重力移動（右スティック）
    Gamepad_ThumbStick rs = Gamepad_GetRightStick(player);
    if (rs.y >  0.5f) { /* 天井へ */ }
    if (rs.y < -0.5f) { /* 床へ */ }
    if (rs.x < -0.5f) { /* 左壁へ */ }
    if (rs.x >  0.5f) { /* 右壁へ */ }
}
```
