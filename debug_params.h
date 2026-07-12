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
    float orbJudgeWindow    = -1.0f;   // オーブの早期HIT判定を開始する追加Z距離（HIT_ZONE_Zに加算。値を大きくするほどプレイヤーから遠い位置で取得判定になり、見た目の取得位置が胴側に上がる）

    static DebugParams& Get()
    {
        static DebugParams s;
        return s;
    }
private:
    DebugParams() = default;
};

#define D_PARAMS (DebugParams::Get())
