#include "result.h"
#include "sprite2d.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "define.h"
#include "font.h"
#include "mouse.h"
#include "sound.h"
#include "ClickFont.h"
#include "scoresummaryloader.h"
#include "scene.h"
#include "MultiLineFontRenderer.h"
#include <sstream>
#include <iomanip>
#include "imgui/imgui.h"
#include <string>

using namespace DirectX;

// ①インスタンス、ポインタ用意
static Sprite2D* g_pResultBG = nullptr;
static Sprite2D* g_pResultBackUI = nullptr;
static ClickFont* g_pChangeSceneText = nullptr;
static ScoreSummary g_ResultScoreSummary;
static RESULT g_Result;

static MultiLineFontRenderer* g_pMusicText = nullptr;
static Sprite2D* g_pRankTextre = nullptr;


// アニメーション用の構造体
struct ResultRowData {
	std::string label;		 // 左側の文字 (例: "SCORE :")
	std::string valueStr;	 // 右側の数字 (例: "12345" ※演出で文字化けする)
	int targetValue{};       // 最終的に表示する数値
	float currentX{};        // 現在のX座標
	float y{};               // Y座標
};

static const int MAX_ROWS = 5;
static ResultRowData g_ResultRows[MAX_ROWS];
static FontRenderer* g_pLabelFont = nullptr;
static FontRenderer* g_pValueFont = nullptr;
	

static float g_CountUpTimer = 0.0f;
static const float COUNT_UP_MAX_TIME = 90.0f; // 90フレーム(1.5秒)でカウントアップ
static float g_ResultSceneTimer = 0.0f;
static const float RANK_ANIM_START_TIME = 120.0f; // 120フレーム(2秒)
static const float RANK_ANIM_DURATION = 30.0f; // 30フレーム(0.5秒)

static const float ROW_DELAY = 30.0f;               // 1行ごとの遅延フレーム
static const float VALUE_START_DELAY = 30.0f;       // ラベルがすべて表示された後に数値を開始するまでの待機フレーム

void Result_Initialize(void)
{
	// ②各種初期化
	
	//プレイした楽曲の概要を取得
	g_ResultScoreSummary = LoadSingleScoreSummary(GetPlayJson());
	//リザルトデータを実体化させてコピー
	g_Result = *GetResult();

	//デバッグ出力（構造体の中身をいい感じに表示すればOK）
	hal::dout << "[result.cpp]" << g_ResultScoreSummary.musicname << std::endl;
	hal::dout << "[result.cpp]" << g_Result.maxCombo << std::endl;

	g_Result.score += 100000; //デバッグ用にスコアを加算
	g_Result.success += 100000;
	g_Result.maxCombo += 100000;
	g_Result.miss += 100000;
	g_Result.rank = "A";

	g_pResultBG = new Sprite2D(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },					//位置
		{ SCREEN_WIDTH, SCREEN_HEIGHT },							//サイズ
		0.0f,														//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },									//RGBA
		BLENDSTATE_ALFA,											//BlendState
		L"asset\\texture\\Result_BG_kari.png"						//テクスチャパス
	);

	g_pResultBackUI = new Sprite2D(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },					//位置
		{ SCREEN_WIDTH, SCREEN_HEIGHT },							//サイズ
		0.0f,														//回転（度）
		{ 1.0f, 1.0f, 1.0f, 0.8f },									//RGBA
		BLENDSTATE_ALFA,											//BlendState
		L"asset\\texture\\Result_Back_UI.png"						//テクスチャパス
	);

	g_pChangeSceneText = new ClickFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT * 5 / 6 },			//位置
		50.0f,														//文字サイズ
		0.0f,														//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },									//通常色
		{ 1.0f, 0.8f, 0.2f, 1.0f },									//ホバー色
		"曲選択へ"									//テキスト
	);


	// フォントの生成（空文字で初期化し、Draw時にセットする）
	g_pLabelFont = new FontRenderer({ 0, 0 }, 43.0f, 0.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, "", TA_START);
	g_pValueFont = new FontRenderer({ 0, 0 }, 43.0f, 0.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, "", TA_START);

	// 各行の初期状態と目標値をセット
	float startX = 100.0f;    
	float targetX = 180.0f;    // 所定のX座標へ
	float startY = 294.0f;
	float lineSpacing = 60.0f; // 行間隔

	g_ResultRows[0] = { "SCORE :",    "", g_Result.score,                   startX,  startY };
	g_ResultRows[1] = { "HIT数 :",    "", g_Result.success , startX ,  startY + lineSpacing };
	g_ResultRows[2] = { "COMBO :", "", g_Result.maxCombo,                startX ,	 startY + lineSpacing * 2 };
	g_ResultRows[3] = { "MISS :",  "", g_Result.miss,                 startX ,  startY + lineSpacing * 3 };


	// 難易度の小数点以下1桁まで表示するためのstringstreamを使用
	std::stringstream ss;
	ss << std::fixed << std::setprecision(1) << g_ResultScoreSummary.difficulty;

	g_pMusicText = new MultiLineFontRenderer(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 12 },
		30.0f,
		0.0f,
		{ 1.0f, 1.0f, 0.0f, 1.0f },
		g_ResultScoreSummary.musicname + "\n" + g_ResultScoreSummary.musicauthor + "\n" + g_ResultScoreSummary.scoreauthor + "\n難易度 : " + ss.str(),
		1.5f,												// 行間倍率
		TA_MIDDLE
	);

	// ランク文字列(std::string)をワイド文字列(std::wstring)に変換
	std::wstring wRank(g_Result.rank.begin(), g_Result.rank.end());
	
	// テクスチャのファイルパスを動的に生成
	std::wstring rankTexturePath = L"asset\\texture\\Result_Rank_" + wRank + L"_UI.png";

	g_pRankTextre = new Sprite2D(
		{ SCREEN_WIDTH / 2 + 350.0f , SCREEN_HEIGHT / 3 + 100.0f },			//位置
		{ 500, 500 },												//サイズ
		0.0f,														//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },									//RGBA
		BLENDSTATE_ALFA,											//BlendState
		rankTexturePath.c_str()										//テクスチャパス
	);

	g_CountUpTimer = 0.0f;
	g_ResultSceneTimer = 0.0f;

	UnLockMouse();//マウスアンロック
}

void Result_Update(void)
{
	//③処理
	g_pChangeSceneText->Update();

	// デバッグ用: Rキーでアニメーションをリスタート
	if (Keyboard_IsKeyDownTrigger(KK_R))
	{
		// 既存のタイマーリセット（g_CountUpTimerは廃止してもOK）
		g_ResultSceneTimer = 0.0f;

		// 初期座標リセット
		for (int i = 0; i < MAX_ROWS; ++i) {
			g_ResultRows[i].currentX = 100.0f ; // startX
			g_ResultRows[i].valueStr = ""; // 表示リセット
		}
	}    
	

	g_ResultSceneTimer += 1.0f;

	for (int i = 0; i < MAX_ROWS; ++i)
	{
		// 数値のアニメーション（ただのカウントアップ）
		float valueStartTime = (MAX_ROWS * ROW_DELAY) + VALUE_START_DELAY + (i * ROW_DELAY);
		if (g_ResultSceneTimer >= valueStartTime)
		{
			// 数値アニメーションの進行度 (0.0 ~ 1.0)
			float valueProgress = (g_ResultSceneTimer - valueStartTime) / COUNT_UP_MAX_TIME;
			if (valueProgress >= 1.0f)
			{
				valueProgress = 1.0f;
				g_ResultRows[i].valueStr = std::to_string(g_ResultRows[i].targetValue);
			}
			else
			{
				// イージング (Ease-Out Quad)
				float easeProgress = 1.0f - (1.0f - valueProgress) * (1.0f - valueProgress);
				int curVal = static_cast<int>(g_ResultRows[i].targetValue * easeProgress);
				g_ResultRows[i].valueStr = std::to_string(curVal);
			}
		}
		else
		{
			// まだ開始時間になっていない場合は空にしておく
			g_ResultRows[i].valueStr = "";
		}
	}

	//ClickFontがクリックされた
	if (g_pChangeSceneText->IsClick())
	{
		SetPlayJson("");//resultを抜けるときに初期化
		SetSceneFade(SCENE_STAGESELECT);
	}
}

void Result_Draw(void)
{
	//④描画
	g_pResultBG->Draw();
	g_pResultBackUI->Draw();
	g_pChangeSceneText->Draw();
	g_pMusicText->Draw();
	g_pRankTextre->Draw();


	if (g_pLabelFont && g_pValueFont)
	{
		for (int i = 0; i < MAX_ROWS; ++i)
		{
			float labelStartTime = i * ROW_DELAY;

			// ラベルの開始時間を過ぎていたら描画
			if (g_ResultSceneTimer >= labelStartTime) {
				g_pLabelFont->SetPos({ g_ResultRows[i].currentX + (i * 20.0f), g_ResultRows[i].y });
				g_pLabelFont->SetText(g_ResultRows[i].label);
				g_pLabelFont->Draw();
			}

			// valueStr が空でなければ数値を描画（Update側で制御済み）
			if (!g_ResultRows[i].valueStr.empty()) {
				g_pValueFont->SetPos({ g_ResultRows[i].currentX + 260.0f + (i * 20.0f), g_ResultRows[i].y});
				g_pValueFont->SetText(g_ResultRows[i].valueStr);
				g_pValueFont->Draw();
			}
		}
	}
}

void Result_Finalize(void)
{
	//⑤解放
	SAFE_DELETE(g_pResultBG);
	SAFE_DELETE(g_pResultBackUI);
	SAFE_DELETE(g_pChangeSceneText);
	SAFE_DELETE(g_pMusicText);
	SAFE_DELETE(g_pRankTextre);
	SAFE_DELETE(g_pLabelFont);
	SAFE_DELETE(g_pValueFont);
	SAFE_DELETE(g_pRankTextre);

}

void Result_DebugUIDraw(void)
{
	ImGui::Begin("Result Scene Editor");
	ImGui::End();
}

