#include "game.h"
#include "enemy_note.h"

namespace
{
	// 通常Gargoyleの初期リム色。個別変更はSetRimLightColor()を使う。
	const XMFLOAT3 NORMAL_GARGOYLE_RIM_COLOR = { 0.25f, 0.55f, 1.0f };
	constexpr float NORMAL_GARGOYLE_RIM_INTENSITY = 0.5f;
}

void EnemyNote::Init(int lane, int face, float spawnZ, float speed, const char* modelPath)
{
	NoteBase::Init(lane, face, spawnZ, speed, modelPath ? modelPath : "asset/model/Gargoyle.fbx");
	m_Scale = { 0.03f,0.03f,0.03f };
	m_ShaderType = S_RIM_LIGHT;
	m_RimLightColor = NORMAL_GARGOYLE_RIM_COLOR;
	m_RimLightIntensity = NORMAL_GARGOYLE_RIM_INTENSITY;
}

void EnemyNote::Draw()
{
	// Parameter.rgbへ色、Parameter.aへ強度を送り、このGargoyleだけのリム色にする。
	SetParameter({
		m_RimLightColor.x,
		m_RimLightColor.y,
		m_RimLightColor.z,
		m_RimLightIntensity
	});
	NoteBase::Draw();
}

