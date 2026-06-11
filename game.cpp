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
#include "camera.h"
#include "komachi/debug_ui.h"
#include "sound.h"
#include "ClickFont.h"
#include "gamecamera.h"

#include "field.h"
#include "player.h"

using namespace DirectX;

// ①インスタンス、ポインタ用意
static Field* g_pField = nullptr;
static Player* g_pPlayer = nullptr;

void Game_Initialize(void)
{
	// ②各種初期化
	int pad = Gamepad_FindConnectedPlayer();
	//if (pad < 0)return;//デバック時必要なし

	GameCamera::Init();

	g_pField = new Field();
	g_pField->Init();

	g_pPlayer = new Player();
	g_pPlayer->Init();

	UnLockMouse();//マウスアンロック

}

void Game_Update(void)
{
	//3D
	{
		GameCamera::Update(g_pPlayer);
		SetCameraPosition(GetCamera()->GetPos());

		g_pField->Update();
		g_pPlayer->Update();
	}

	//2D描画
	{
		//③処理

	}
	DebugUI_Draw();

	if (Keyboard_IsKeyDownTrigger(KK_D2))Mouse_SetVisible(true);//マウス表示
	if (Keyboard_IsKeyDownTrigger(KK_D3))Mouse_SetVisible(false);//マウス非表示
}

void Game_Draw(void)
{
	//④描画
	//3D
	{
		SetDepthEnable(true);

		g_pField->Draw();
		g_pPlayer->Draw();

		SetDepthEnable(false);
	}

	//2D
	{

	}

	DebugUI_Draw();
}

void Game_Finalize(void)
{
	//⑤解放
	SAFE_DELETE(g_pField);
	SAFE_DELETE(g_pPlayer);
	GameCamera::Finalize();
}
