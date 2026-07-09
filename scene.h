#pragma once

#include <string>

//リザルトに渡す構造体
struct SnedResult {
	int maxCombo;			//最大コンボ数
	int hits;			//成功数
	int misses;				//失敗数
	int orbgets;
	int orblosses;
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
void SetResult(const SnedResult& r);
const SnedResult* GetResult();
