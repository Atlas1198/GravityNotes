#include "define.h"
#include "keyboard.h"
#include "mouse.h"
#include "gamepad.h"

#include "player.h"

const float dt = 1.0f / FPS;

int pad;

void Player::Init()
{
	pad = Gamepad_FindConnectedPlayer();//現在仮置き、多分フレームワークに置いた方が各シーンで使いやすい
	m_GravityFace = FACE::FACE_FLOOR;

	m_Position = { 0.0f,-2.5f,0.0f };
	m_Rotation = { 0.0f,180.0f,0.0f };
}

void Player::Update()
{
	if (m_GravityFace == FACE::FACE_FLOOR || m_GravityFace == FACE::FACE_CEILING)
	{
		//lane移動入力
		if (Gamepad_GetLeftStick(pad).x < 0||Keyboard_IsKeyDownTrigger(KK_A))//左移動
		{

		}
		else if (Gamepad_GetLeftStick(pad).x > 0||Keyboard_IsKeyDownTrigger(KK_D))//右移動
		{

		}
	}
	else {
		//lane移動入力
		if (Gamepad_GetLeftStick(pad).y > 0 || Keyboard_IsKeyDownTrigger(KK_W))//上移動
		{

		}
		else if (Gamepad_GetLeftStick(pad).y < 0 || Keyboard_IsKeyDownTrigger(KK_S))//下移動
		{

		}
	}

	//重力変更入力
	
	//ノーツヒット入力

}

void Player::Draw()
{
	if (m_Model) {
		ModelDraw(
			m_Model,
			m_Position,
			m_Rotation,
			m_Scale,
			m_Color,
			false,
			S_UNLIT
		);
	}
}

void Player::Finalize()
{
}
