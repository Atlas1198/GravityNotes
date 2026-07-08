#pragma once

#include <string>

//リザルトに渡す構造体
struct RESULT {
	int score;				//スコア
	std::string rank;		//ランク
	float accurary;			//達成率
	int maxCombo;			//最大コンボ数

	int success;			//成功数
	int miss;				//失敗数
};

enum SCENE {
	SCENE_TITLE = 0,
	SCENE_STAGESELECT,
	SCENE_GAME,
	SCENE_RESULT,
	SCENE_DEBUG,
	SCENE_MAX,
	SCENE_NONE,
};

void Init(void);
void Update(void);
void Draw(void);
void Finalize(void);

void SetScene(SCENE id);
SCENE GetScene(void);
void SetPlayJson(const std::string& jsonName);
const std::string& GetPlayJson(void);
void SetResult(const RESULT& r);
const RESULT* GetResult();
