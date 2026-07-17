#include "game.h"
#include "barrier_note.h"
#include "camera.h"

void BarrierNote::Init(int lane, int face, float spawnZ, float speed, float beat)
{
	NoteBase::Init(lane, face, spawnZ, speed, "asset/model/barrier.fbx");
	
	m_IsHiddenByPriority = false;

	// 3x3グリッド用の配置計算
	// フィールドは5.0fスケール・モデルは1x1x1 → 内径 ±2.5f のトンネル内に3分割で敷き詰める
	static const float TUNNEL_HALF = 2.4f; // NOTE_TUNNEL_HALF と合わせる
	float S     = TUNNEL_HALF * 2.0f / 3.0f; // セル幅 = 5.0f / 3.0f
	float margin = 0.1f;
	float size   = S - margin;
	SetSize(XMFLOAT3(size, size, size));

	XMFLOAT3 pos = GetPos();
	// 面法線方向: 壁装飾へのめり込みを防ぐため、壁面から wall_offset だけ内側にオフセットする
	// 面内方向: レーン(-1,0,1) × S でセル中心に配置
	static const float wall_offset = 0.1f; // 壁の装飾の厚さ分だけ浮かせる
	// face: 0=FLOOR, 1=LEFT_WALL, 2=CEILING, 3=RIGHT_WALL
	switch (face)
	{
	case 0: // FLOOR  (底面: y = -TUNNEL_HALF, モデルは上向きに伸びる)
		pos.x =  (float)lane * S;
		pos.y = -TUNNEL_HALF + wall_offset;
		break;
	case 1: // LEFT_WALL  (左面: x = -TUNNEL_HALF, rot=-90° でモデルは右方向に伸びる)
		pos.x = -TUNNEL_HALF + wall_offset;
		pos.y =  (float)lane * S;
		break;
	case 2: // CEILING  (天面: y = +TUNNEL_HALF, rot=180° でモデルは下向きに伸びる)
		pos.x =  (float)lane * S;
		pos.y =  TUNNEL_HALF - wall_offset;
		break;
	case 3: // RIGHT_WALL  (右面: x = +TUNNEL_HALF, rot=+90° でモデルは左方向に伸びる)
		pos.x =  TUNNEL_HALF - wall_offset;
		pos.y =  (float)lane * S;
		break;
	}
	SetPos(pos);

	m_ShaderType = S_LAMBERT;
	SetColor(1.0f, 1.0f, 1.0f);
	m_Beat = beat;
}

void BarrierNote::OnHit()
{
	m_IsHit = true;
	// バリアノーツは判定が済んでもカメラ後方に下がるまで表示を残すため m_IsActive は変更しない
}

void BarrierNote::OnMiss()
{
	m_IsHit = true; // 被弾時も同様に表示を残す
}

void BarrierNote::Draw()
{
	if (m_IsHiddenByPriority) return;

	constexpr float PLAYER_Z = 2.0f;
	constexpr float FADE_END_DISTANCE = 4.0f;
	constexpr float MIN_ALPHA = 0.1f;
	const XMFLOAT3 cameraPos = GetCamera()->GetPos();
	const XMVECTOR toCamera = XMVectorSubtract(XMLoadFloat3(&cameraPos), XMLoadFloat3(&m_Position));
	const float cameraDistance = XMVectorGetX(XMVector3Length(toCamera));
	float alpha = 1.0f;

	if (m_Position.z < PLAYER_Z)
	{
		XMFLOAT3 fadeStartPos = m_Position;
		fadeStartPos.z = PLAYER_Z;
		const XMVECTOR toFadeStart = XMVectorSubtract(XMLoadFloat3(&cameraPos), XMLoadFloat3(&fadeStartPos));
		const float fadeStartDistance = XMVectorGetX(XMVector3Length(toFadeStart));
		const float fadeRange = fadeStartDistance - FADE_END_DISTANCE;
		if (fadeRange > 0.0f)
		{
			float fade = (fadeStartDistance - cameraDistance) / fadeRange;
			if (fade < 0.0f) fade = 0.0f;
			if (fade > 1.0f) fade = 1.0f;
			alpha = 1.0f + (MIN_ALPHA - 1.0f) * fade;
		}
	}

	const bool isDithered = alpha < 1.0f;
	SetColorAlpha(alpha);
	if (isDithered)
	{
		SetBlendState(BLENDSTATE_NONE);
		SetDepthWriteEnable(true);
	}
	NoteBase::Draw();
	if (isDithered)
		SetBlendState(BLENDSTATE_ALFA);
}
