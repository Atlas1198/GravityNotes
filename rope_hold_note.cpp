#define NOMINMAX
#include "rope_hold_note.h"
#include "game.h"
#include <algorithm>

static const float ROPE_HIT_ZONE_Z   = 3.0f;
static const float ROPE_ACTIVE_RANGE = 2.5f; // Activate 可能な判定窓

void RopeHoldNote::Init(int startLane, int endLane, int startFace, int endFace,
                        float startZ, float endZ, float speed)
{
	NoteBase::Init(startLane, startFace, startZ, speed, "asset/model/cube.fbx");
	m_EndFace      = endFace;
	m_EndLane      = endLane;
	m_RopeLength   = endZ - startZ;
	m_HoldProgress = 0.0f;
	m_State        = State::IDLE;
	SetColor(1.0f, 0.5f, 0.0f); // オレンジ色
}

void RopeHoldNote::Update()
{
	AddPosZ(-m_Speed * dt);

	if (m_State == State::HOLDING)
	{
		// 先端が HIT_ZONE_Z を通過した距離 / ロープ全長 = 進捗
		float passed   = ROPE_HIT_ZONE_Z - GetPosZ();
		m_HoldProgress = std::max(0.0f, std::min(passed / m_RopeLength, 1.0f));
		if (m_HoldProgress >= 1.0f)
			Complete();
		return;
	}

	// IDLE：判定窓を通り過ぎたら押し逃し
	if (m_State == State::IDLE && GetPosZ() < ROPE_HIT_ZONE_Z - ROPE_ACTIVE_RANGE)
	{
		m_State    = State::FAILED;
		m_IsActive = false;
	}
}

void RopeHoldNote::Draw()
{
	if (!m_IsActive) return;

	// ロープをZ方向に伸ばしたキューブで描画（中心をロープ中点に合わせる）
	XMFLOAT3 savedPos   = m_Position;
	XMFLOAT3 savedScale = m_Scale;

	m_Position.z += m_RopeLength * 0.5f;
	m_Scale = { 0.3f, 0.3f, m_RopeLength * 0.5f };

	Sprite3D::Draw();

	m_Position = savedPos;
	m_Scale    = savedScale;
}

void RopeHoldNote::OnHit()
{
	Activate();
}

bool RopeHoldNote::Activate()
{
	if (m_State != State::IDLE) return false;
	m_State = State::HOLDING;
	return true;
}

void RopeHoldNote::Release()
{
	if (m_State != State::HOLDING) return;
	m_State    = State::FAILED;
	m_IsActive = false;
}

void RopeHoldNote::Complete()
{
	m_HoldProgress = 1.0f;
	m_State        = State::COMPLETE;
	m_IsHit        = true;
	m_IsActive     = false;
}
