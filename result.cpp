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
		"SCORE ：\nHIT数 ：\nCOMBO ：\nMAXCOMBO ：\nSUCCESS ：\nMISS ：",						 // 初期テキスト（\nで改行）
		1.4f,																				 // 行間倍率
		TA_START
	);

	g_pScoreText = new MultiLineFontRenderer(
		{ SCREEN_WIDTH / 3, SCREEN_HEIGHT / 6  },            
		30.0f,												
		0.0f,												
		{ 1.0f, 1.0f, 0.0f, 1.0f },							
		"102\n130\n95\n80\n85\n102",						 
		1.4f,												
		TA_MIDDLE
	);


	UnLockMouse();//マウスアンロック
}

void Result_Update(void)
{
	//③処理
	g_pChangeSceneText->Update();

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
