/*==============================================================================

   ポリゴン描画 [game.h]
--------------------------------------------------------------------------------

==============================================================================*/
#pragma once

#include <d3d11.h>
#include "define.h"

//ゲームシーンにおける定数定義
#define dt	(1.0f/FPS)//
#define LANE_WIDTH	(2.5f)//レーンの間隔

//リザルトに渡す構造体
struct RESULT {
	int score;				//スコア
	std::string rank;		//ランク
	float accurary;		//達成率
	int maxCombo;		//最大コンボ数

	int success;				//成功数
	int miss;				//失敗数
};

void Game_Initialize(void);
void Game_Finalize(void);
void Game_Update(void);
void Game_Draw(void);