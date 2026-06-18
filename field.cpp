#include "field.h"
#include "shadermanager.h"
#include "define.h"

using namespace DirectX;

void Field::Init() {
	const XMFLOAT3 pos   = { 0.0f, 0.0f, 10.0f };
	const XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	const XMFLOAT3 rot   = { 0.0f, 0.0f, 0.0f };
	const XMFLOAT3 rotFlipped = { 0.0f, XM_PI, 0.0f };

	m_Floor = new Sprite3D(pos, scale, rot, "asset/model/Floor_model.fbx", S_UNLIT);
	m_WallLeft = new Sprite3D(pos, scale, rot, "asset/model/Wall_model.fbx", S_UNLIT);
	m_WallRight = new Sprite3D(pos, scale, rotFlipped, "asset/model/Wall_model.fbx", S_UNLIT);
	m_Ceiling = new Sprite3D(pos, scale, rot, "asset/model/Ceiling_model.fbx", S_LAMBERT);
}

void Field::Update(){

}

void Field::Draw() {
	if (m_Floor) m_Floor->Draw();
	if (m_WallLeft) m_WallLeft->Draw();
	if (m_WallRight) m_WallRight->Draw();
	if (m_Ceiling) m_Ceiling->Draw();
}

void Field::Finalize() {
	delete m_Floor; m_Floor = nullptr;
	delete m_WallLeft; m_WallLeft = nullptr;
	delete m_WallRight; m_WallRight = nullptr;
	delete m_Ceiling; m_Ceiling = nullptr;
}