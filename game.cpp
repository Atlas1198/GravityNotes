#include "game.h"
#include "define.h"
#include "sprite2d.h"
#include "texture.h"
#include "fade.h"
#include "debug_ostream.h"
#include "font.h"
#include "mouse.h"
#include "keyboard.h"
#include "gamepad.h"
#include "model.h"
#include "debugcamera.h"
#include "debug_ui.h"
#include "sound.h"
#include "ClickFont.h"
#include "scene.h"
#include "gamepad.h"
#include "camera.h"

#include "field.h"
#include "player.h"
#include "gamecamera.h"
#include "note_manager.h"
#include "light_game.h"
#include "status_manager.h"

using namespace DirectX;

// ①インスタンス、ポインタ用意
static Sprite2D* g_pGameSprite = nullptr;
static ClickFont* g_pChangeSceneText = nullptr;
static FontRenderer* g_pSelectedJsonText = nullptr;

static Field*         g_pField         = nullptr;
static Player*        g_pPlayer        = nullptr;
static NoteManager*   g_pNoteManager   = nullptr;
static StatusManager* g_pStatusManager = nullptr;

void Game_Initialize(void)
{
	// ②各種初期化
	//g_pGameSprite = new Sprite2D(
	//	{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 3 },					//位置
	//	{ 300.0f, 300.0f },											//サイズ
	//	0.0f,														//回転（度）
	//	{ 1.0f, 1.0f, 1.0f, 1.0f },									//RGBA
	//	BLENDSTATE_NONE,											//BlendState
	//	L"asset\\texture\\tex.png"									//テクスチャパス
	//);

	//g_pChangeSceneText = new ClickFont(
	//	{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 4.0f * 3 },			//位置
	//	50.0f,														//文字サイズ
	//	0.0f,														//回転（度）
	//	{ 1.0f, 1.0f, 1.0f, 1.0f },									//通常色
	//	{ 1.0f, 0.8f, 0.2f, 1.0f },									//ホバー色
	//	"[game.cpp] リザルトへ"										//テキスト
	//);

	//前シーンで選択されたjsonの仮表示
	/*const std::string selectedJson = GetPlayJson();
	g_pSelectedJsonText = new FontRenderer(
		{ SCREEN_WIDTH / 4.0f, SCREEN_HEIGHT / 2.0f },
		28.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"Selected JSON: " + (selectedJson.empty() ? std::string("(none)") : selectedJson)
	);*/

	//int pad = Gamepad_FindConnectedPlayer();
	//if (pad < 0)return;//デバック時必要なし

  //各種初期化
	GameCamera::Init();
	GameLight::Init();

	g_pField = new Field();
	g_pField->Init();

	g_pStatusManager = new StatusManager();
	g_pStatusManager->Init();

	g_pNoteManager = new NoteManager();

	g_pNoteManager->Init("asset/score/" + GetPlayJson());
	//g_pNoteManager->Init("asset/score/shiningstar.json");

	g_pPlayer = new Player();
	g_pPlayer->Init(g_pNoteManager, g_pStatusManager);

	//UnLockMouse();//マウスアンロック
}

void Game_Update(void)
{
	//3D
	{
		GameCamera::Update(g_pPlayer);
		SetCameraPosition(GetCamera()->GetPos());

		g_pField->Update(g_pNoteManager->GetNoteSpeed());
		g_pPlayer->Update();
		g_pNoteManager->Update(g_pPlayer->GetLaneIndex(), g_pPlayer->GetGravityFace());

	}

	//2D描画
	{
		//③処理
		//g_pChangeSceneText->Update();

		//ClickFontがクリックされた
		/*if (g_pChangeSceneText->IsClick())
		{
			SetSceneFade(SCENE_RESULT);
		}*/
	}

	if (Keyboard_IsKeyDownTrigger(KK_D2))Mouse_SetVisible(true);
	if (Keyboard_IsKeyDownTrigger(KK_D3))Mouse_SetVisible(false);

	if (Keyboard_IsKeyDownTrigger(KK_ENTER)) {
		RESULT r;
		r.score = 13232;
		r.rank = "A";
		r.accurary = 87.45;
		r.maxCombo = 175;
		r.success = 312;
		r.miss = 26;

		SetResult(r);
		SetSceneFade(SCENE_RESULT);
	}
}

void Game_Draw(void)
{
	//④描画
	//3D
	{
		SetDepthEnable(true);

		g_pField->Draw();
		g_pNoteManager->Draw();
		g_pPlayer->Draw();

		SetDepthEnable(false);
	}

	//2D
	{
		//g_pGameSprite->Draw();
		//g_pChangeSceneText->Draw();
		//g_pSelectedJsonText->Draw();
	}
}

void Game_Finalize(void)
{
	//⑤解放
	SAFE_DELETE(g_pGameSprite);
	SAFE_DELETE(g_pSelectedJsonText);
	SAFE_DELETE(g_pChangeSceneText);

	SAFE_DELETE(g_pField);
	SAFE_DELETE(g_pPlayer);
	if (g_pNoteManager)   { g_pNoteManager->Finalize();   SAFE_DELETE(g_pNoteManager); }
	if (g_pStatusManager) { g_pStatusManager->Finalize(); SAFE_DELETE(g_pStatusManager); }
	GameLight::Finalize();
	GameCamera::Finalize();
}
