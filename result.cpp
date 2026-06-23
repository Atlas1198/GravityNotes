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

using namespace DirectX;

// ①インスタンス、ポインタ用意
static Sprite2D* g_pResultSprite = nullptr;
static ClickFont* g_pChangeSceneText = nullptr;
static ScoreSummary g_ResultScoreSummary;
static RESULT g_Result;
static MultiLineFontRenderer* g_pDetailText = nullptr;
static MultiLineFontRenderer* g_pScoreText = nullptr;
static float g_CountUpTimer = 0.0f;
static const float COUNT_UP_MAX_TIME = 90.0f; // 90フレーム(1.5秒)でカウントアップ

	

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
	g_Result.accurary = 99.99f;
	g_Result.rank = "SSS";

	//g_pResultSprite = new Sprite2D(
	//	{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 3 },					//位置
	//	{ 300.0f, 300.0f },											//サイズ
	//	0.0f,														//回転（度）
	//	{ 1.0f, 1.0f, 1.0f, 1.0f },									//RGBA
	//	BLENDSTATE_NONE,											//BlendState
	//	L"asset\\texture\\tex.png"									//テクスチャパス
	//);

	g_pChangeSceneText = new ClickFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 4.0f * 3 },			//位置
		50.0f,														//文字サイズ
		0.0f,														//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },									//通常色
		{ 1.0f, 0.8f, 0.2f, 1.0f },									//ホバー色
		"[result.cpp] タイトルへ"										//テキスト
	);

	g_pDetailText = new MultiLineFontRenderer(
		{ SCREEN_WIDTH / 5, SCREEN_HEIGHT / 6 },											 // 表示基準位置
		30.0f,																				 // フォントサイズ
		0.0f,																				 // 回転角（度）
		{ 1.0f, 1.0f, 0.0f, 1.0f },															 // 文字色 RGBA
		"SCORE :\nHIT数 :\nMAXCOMBO :\nSUCCESS :\nMISS :",						 // 初期テキスト（\nで改行）
		0.0f,																				 // 行間倍率（アニメーションで広げるため初期値0）
		TA_START
	);

	g_pScoreText = new MultiLineFontRenderer(
		{ SCREEN_WIDTH / 3, SCREEN_HEIGHT / 6  },            
		30.0f,												
		0.0f,												
		{ 1.0f, 1.0f, 0.0f, 1.0f },							
		"0\n0\n0\n0\n0",						 
		0.0f,												// 行間倍率（アニメーションで広げるため初期値0）
		TA_START
	);

	g_CountUpTimer = 0.0f;

	UnLockMouse();//マウスアンロック
}

void Result_Update(void)
{
	//③処理
	g_pChangeSceneText->Update();

	if (g_CountUpTimer < COUNT_UP_MAX_TIME)
	{
		g_CountUpTimer += 1.0f;
		float progress = g_CountUpTimer / COUNT_UP_MAX_TIME;
		if (progress > 1.0f) progress = 1.0f;

		// イージング (Ease-Out Quad)
		float easeProgress = 1.0f - (1.0f - progress) * (1.0f - progress);

		// 行間の展開アニメーション (0.0f から 1.4f へ)
		float currentSpacing = 1.4f * easeProgress;
		g_pDetailText->SetLineSpacing(currentSpacing);
		g_pScoreText->SetLineSpacing(currentSpacing);

		// スロット＆グリッチ風の数字変換ヘルパー
		auto ApplySlotGlitch = [&](int currentVal, int targetVal) -> std::string {
			if (progress >= 1.0f) return std::to_string(targetVal);

			std::string str = std::to_string(currentVal);
			if (str.empty()) return str;

			// イージングの進行度に応じて、右側の文字をランダム化
			int randomCount = static_cast<int>(str.length() * (1.0f - progress));
			if (randomCount < 1 && progress < 1.0f) {
				randomCount = 1; // 完了するまでは最低下1桁を回す
			}

			// 右側をランダムな数字にする
			for (size_t i = str.length() - randomCount; i < str.length(); ++i) {
				str[i] = '0' + (rand() % 10);
			}

			// 10%の確率で、それ以外の確定している桁も一瞬だけ文字化けする（グリッチ演出）
			if (rand() % 100 < 10) {
				int idx = rand() % str.length();
				str[idx] = '0' + (rand() % 10);
			}

			return str;
		};

		int curScore = static_cast<int>(g_Result.score * easeProgress);
		int curHit = static_cast<int>((g_Result.success + g_Result.miss) * easeProgress);
		int curMaxCombo = static_cast<int>(g_Result.maxCombo * easeProgress);
		int curSuccess = static_cast<int>(g_Result.success * easeProgress);
		int curMiss = static_cast<int>(g_Result.miss * easeProgress);

		int targetHit = g_Result.success + g_Result.miss;

		std::string scoreStr = ApplySlotGlitch(curScore, g_Result.score) + "\n" +
							   ApplySlotGlitch(curHit, targetHit) + "\n" +
							   ApplySlotGlitch(curMaxCombo, g_Result.maxCombo) + "\n" +
							   ApplySlotGlitch(curSuccess, g_Result.success) + "\n" +
							   ApplySlotGlitch(curMiss, g_Result.miss);
		g_pScoreText->SetText(scoreStr);
	}

	//ClickFontがクリックされた
	if (g_pChangeSceneText->IsClick())
	{
		SetPlayJson("");//resultを抜けるときに初期化
		SetSceneFade(SCENE_TITLE);
	}
}

void Result_Draw(void)
{
	//④描画
	//g_pResultSprite->Draw();
	g_pChangeSceneText->Draw();
	g_pDetailText->Draw();
	g_pScoreText->Draw();
}

void Result_Finalize(void)
{
	//⑤解放
	//SAFE_DELETE(g_pResultSprite);
	SAFE_DELETE(g_pChangeSceneText);
	SAFE_DELETE(g_pDetailText);
	SAFE_DELETE(g_pScoreText);
}
