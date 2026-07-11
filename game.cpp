#include "game.h"
#include "define.h"
#include "sprite2d.h"
#include "texture.h"
#include "fade.h"
#include "debug_ostream.h"
#include "font.h"
#include "mouse.h"
#include "keyboard.h"
#include "input_manager.h"
#include "model.h"
#include "debugcamera.h"
#include "debug_ui.h"
#include "sound.h"
#include "ClickFont.h"
#include "scene.h"
#include "camera.h"
#include "field.h"
#include "player.h"
#include "gamecamera.h"
#include "note_manager.h"
#include "light_game.h"
#include "status_manager.h"
#include "game_ui.h"

using namespace DirectX;

// ゲーム状態の定義
enum class GameState {
	PLAYING,
	FINISHED_WAIT,    // 終了検知後、表示開始までの2秒待機中（何も表示しない）
	FINISHED_DISPLAY, // 2秒経過後、ロゴ等を表示する終了演出中（2秒間表示して自動遷移）
};

static GameState      g_GameState = GameState::PLAYING;
static float          g_FinishTimer = 0.0f;

// ①インスタンス、ポインタ用意
static Sprite2D* g_pGameSprite = nullptr;
static ClickFont* g_pChangeSceneText = nullptr;
static FontRenderer* g_pSelectedJsonText = nullptr;
static Field*         g_pField         = nullptr;
static Player*        g_pPlayer        = nullptr;
static NoteManager*   g_pNoteManager   = nullptr;
static StatusManager* g_pStatusManager = nullptr;
static bool           g_IsMouseCursorVisible = false;
static GameUI*        g_pGameUI        = nullptr;

void Game_Initialize(void)
{
	//int pad = Gamepad_FindConnectedPlayer();
	//if (pad < 0)return;//デバック時必要なし

  // 各状態の初期化
	g_GameState   = GameState::PLAYING;
	g_FinishTimer = 0.0f;

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

	g_pGameUI = new GameUI();
	g_pGameUI->Init();

	UnLockMouse();//マウスアンロック
}

void Game_Update(void)
{
	if (g_GameState == GameState::FINISHED_WAIT)
	{
		// 終了演出待機中：3秒間余韻を持たせる（黒フェードが徐々に表示される）
		g_FinishTimer += dt;
		if (g_FinishTimer >= 3.0f)
		{
			g_GameState = GameState::FINISHED_DISPLAY;
			g_FinishTimer = 0.0f;

			// UI側にロゴの表示開始を通知
			g_pGameUI->ShowResultLogos();
		}
	}
	else if (g_GameState == GameState::FINISHED_DISPLAY)
	{
		// 終了演出表示中：3秒間ロゴ等を表示し続け、3秒経過したら自動でリザルト画面へ遷移
		g_FinishTimer += dt;
		if (g_FinishTimer >= 3.0f)
		{
			SetResult(g_pStatusManager->GetResult());
			SetSceneFade(SCENE_RESULT);
		}
	}

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
		g_pGameUI->Update(g_pStatusManager);
		if (g_pStatusManager->HasNewJudge())
			g_pGameUI->NotifyJudge(g_pStatusManager->ConsumeJudge());
		//g_pChangeSceneText->Update();

		//ClickFontがクリックされた
		/*if (g_pChangeSceneText->IsClick())
		{
			SetSceneFade(SCENE_RESULT);
		}*/
	}

	//マウスカーソルを表示/非表示切り替え(デバッグ用)
	if (Keyboard_IsKeyDownTrigger(KK_U))
	{
		g_IsMouseCursorVisible = !g_IsMouseCursorVisible;
		if (g_IsMouseCursorVisible)
		{
			UnLockMouse();
		}
		else
		{
			LockMouse();
		}
	}

	// 状態更新
	if (g_GameState == GameState::PLAYING)
	{
		bool isDead = g_pStatusManager->IsDead();
		bool isFinished = g_pNoteManager->IsFinished();
		if (isDead || isFinished)
		{
			g_GameState = GameState::FINISHED_WAIT;
			g_FinishTimer = 0.0f;

			// UI側に終了演出（フェードイン）開始を通知
			bool isAllHit = (!isDead && g_pStatusManager->GetResult().misses == 0);
			g_pGameUI->StartEndSequence(isDead, isAllHit);

			// 6.0秒かけて音をフェードアウト
			g_pNoteManager->StartBgmFadeOut(6.0f);
		}
	}

	if (Input_IsActionTrigger(INPUT_ACTION_DEBUG_RESULT)) {
		SetResult(g_pStatusManager->GetResult());
		SetSceneFade(SCENE_RESULT);
	}
}

void Game_Draw(void)
{
	//④描画

	// --- 影パス（4面ShadowMap作成）---
	// トンネルの4面それぞれを、その面の内側からライトで照らして影を焼く。
	// キャスターは Player(自分の重力面へ) と Enemy/Orbノーツ(各自の面へ)。受け手は Field。
	{
		XMFLOAT3 pPos = g_pPlayer->GetPos();
		int playerFace = g_pPlayer->GetGravityFace();

		const float FACE_HALF = 2.5f;   // 各面(床/壁/天井)のトンネル半径
		const float lightDist = 12.0f;  // 面から内側へどれだけ離れた所にライトを置くか
		const float centerZ   = pPos.z + 6.0f; // 影の中心Z（プレイヤーの少し奥）

		// 影の濃さ(0=真っ黒〜1=影なし。小さいほど濃い)。Player/Enemyで個別に設定できる。
		const float SHADOW_BIAS              = 0.003f;
		const float PLAYER_SHADOW_BRIGHTNESS = 0.35f; // ← Playerの影の濃さ
		const float ENEMY_SHADOW_BRIGHTNESS  = 0.2f; // ← Enemyの影の濃さ

		XMMATRIX faceView[NUM_SHADOW_FACES];
		XMMATRIX faceProj[NUM_SHADOW_FACES];
		XMMATRIX faceVP[NUM_SHADOW_FACES];
		XMVECTOR camUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // 視線が±X/±Yなのでトンネル軸Zをup

		for (int f = 0; f < NUM_SHADOW_FACES; f++)
		{
			XMVECTOR target, eye;
			switch (f)
			{
			case FACE_FLOOR:      target = XMVectorSet(0.0f, -FACE_HALF, centerZ, 1.0f); eye = XMVectorSet(0.0f, -FACE_HALF + lightDist, centerZ, 1.0f); break;
			case FACE_LEFT_WALL:  target = XMVectorSet(-FACE_HALF, 0.0f, centerZ, 1.0f); eye = XMVectorSet(-FACE_HALF + lightDist, 0.0f, centerZ, 1.0f); break;
			case FACE_CEILING:    target = XMVectorSet(0.0f,  FACE_HALF, centerZ, 1.0f); eye = XMVectorSet(0.0f,  FACE_HALF - lightDist, centerZ, 1.0f); break;
			case FACE_RIGHT_WALL: target = XMVectorSet( FACE_HALF, 0.0f, centerZ, 1.0f); eye = XMVectorSet( FACE_HALF - lightDist, 0.0f, centerZ, 1.0f); break;
			}
			faceView[f] = XMMatrixLookAtLH(eye, target, camUp);
			// 正射影：幅20(レーン方向) × 奥行50(Z)
			faceProj[f] = XMMatrixOrthographicLH(20.0f, 50.0f, 0.5f, lightDist * 2.0f);
			faceVP[f]   = faceView[f] * faceProj[f];
		}

		SetFaceShadowMatrices(faceVP, SHADOW_BIAS, PLAYER_SHADOW_BRIGHTNESS, ENEMY_SHADOW_BRIGHTNESS);

		for (int f = 0; f < NUM_SHADOW_FACES; f++)
		{
			// Playerスライス(0-3)：今いる重力面にだけ影を落とす
			BeginFaceShadowMap(f);
			SetCullState(CULLSTATE_BACK);
			if (playerFace == f)
				g_pPlayer->DrawShadowMap(faceView[f], faceProj[f]);
			SetCullState(CULLSTATE_NONE);

			// ノーツスライス(4-7)：その面にいる Enemy/Orb ノーツの影
			BeginFaceShadowMap(f + NUM_SHADOW_FACES);
			SetCullState(CULLSTATE_BACK);
			g_pNoteManager->DrawShadowMapForFace(f, faceView[f], faceProj[f]);
			SetCullState(CULLSTATE_NONE);
		}
		EndFaceShadowMap();
	}

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
		g_pGameUI->Draw();
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

	if (g_pField)         { g_pField->Finalize();         SAFE_DELETE(g_pField); }
	if (g_pPlayer)        { g_pPlayer->Finalize();        SAFE_DELETE(g_pPlayer); }
	if (g_pNoteManager)   { g_pNoteManager->Finalize();   SAFE_DELETE(g_pNoteManager); }
	if (g_pStatusManager) { g_pStatusManager->Finalize(); SAFE_DELETE(g_pStatusManager); }
	if (g_pGameUI)        { g_pGameUI->Finalize();        SAFE_DELETE(g_pGameUI); }
	GameLight::Finalize();
	GameCamera::Finalize();
}
