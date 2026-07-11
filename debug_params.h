// LD（レベルデザイン）変数管理
// デバッグ中に ImGui で値を変更できる。
// 使い方: D_PARAMS.noteSpeed など
#pragma once

struct DebugParams
{
    // ノーツ
    float noteSpeed         = 15.0f;  // ノーツのZ軸移動速度 (units/sec)
    float hitDistance       = 2.0f;   // 判定が発生するZ距離 (units)

    // プレイヤー
    float laneWidth         = 2.0f;   // レーン間の距離 (units)
    float gravityTransTime  = 0.3f;   // 重力移動にかかる時間 (sec)
    float damageFlashColor[4] = { 1.0f, 0.1f, 0.1f, 1.0f };
    float damageFlashDuration = 0.7f; // ダメージ点滅の合計時間 (sec)
    float damageFlashInterval = 0.08f;// 通常色/点滅色の切替間隔 (sec)

    // スコア
    int   baseScore         = 100;    // 1ヒットあたりの基本スコア
    float comboMultiplier   = 0.1f;   // コンボ数 × この値が倍率に加算（1.0 + combo * multiplier）

    // オーブ
    int   orbHealAmount     = 30;     // オーブ取得時のHP回復量
    float orbJudgeWindow    = 0.5f;   // オーブの早期HIT判定を開始するZ距離（PASSIVE_ZONE_Zより手前側のみ。Miss確定ラインはPASSIVE_ZONE_Z固定）

    static DebugParams& Get()
    {
        static DebugParams s;
        return s;
    }
private:
    DebugParams() = default;
};

#define D_PARAMS (DebugParams::Get())
