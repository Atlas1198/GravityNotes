#include "model.h"

#include "game.h"
#include "note_base.h"
#include "debug_ostream.h"

static const float NOTE_TUNNEL_HALF = 2.5f;
static const float NOTE_DESPAWN_Z   = -5.0f;

void NoteBase::Init(int lane, int face, float spawnZ, float speed, const char* modelPath)
{
	m_LaneIndex = lane;
	m_Face      = face;
	m_Speed     = speed;
	m_IsActive  = true;
	m_IsHit     = false;
	m_SpawnZ    = spawnZ;
	m_SpawnTimer = 0.0f;

	// 初回のみモデルをロード（プール再利用時はスキップ）
	if (!m_Model && modelPath)
		m_Model = ModelLoad(modelPath);

	// 面とレーンから初期座標を計算
	// face: 0=FLOOR, 1=LEFT_WALL, 2=CEILING, 3=RIGHT_WALL
	float laneVal = lane * LANE_WIDTH;
	XMFLOAT3 pos = { 0.0f, 0.0f, spawnZ };
	switch (face)
	{
	case 0: pos.x =  laneVal;           pos.y = -NOTE_TUNNEL_HALF; break;
	case 1: pos.x = -NOTE_TUNNEL_HALF;  pos.y =  laneVal;          break;
	case 2: pos.x =  laneVal;           pos.y =  NOTE_TUNNEL_HALF; break;
	case 3: pos.x =  NOTE_TUNNEL_HALF;  pos.y =  laneVal;          break;
	}
	SetPos(pos);
	switch (face)
	{
	case 0: SetRot({ 0.0f, 0.0f,   0.0f }); break; // FLOOR
	case 1: SetRot({ 0.0f, 0.0f, -90.0f }); break; // LEFT_WALL
	case 2: SetRot({ 0.0f, 0.0f, 180.0f }); break; // CEILING
	case 3: SetRot({ 0.0f, 0.0f,  90.0f }); break; // RIGHT_WALL
	}
	SetSize({ 1.0f, 1.0f, 1.0f });
}

void NoteBase::Update()
{
	AddPosZ(-m_Speed * dt);
	if (GetPosZ() < NOTE_DESPAWN_Z)
		m_IsActive = false;

	m_SpawnTimer += dt;
}

void NoteBase::Draw()
{
	SetColorAlpha(GetFadeInAlpha());
	Sprite3D::Draw();
}

void NoteBase::Finalize()
{
	m_IsActive = false;
}

void NoteBase::OnHit()
{
	m_IsHit    = true;
	m_IsActive = false;
}

void NoteBase::OnMiss()
{
	m_IsActive = false;

	const char* typeStr = "Unknown";
	switch (GetType())
	{
	case NoteType::Enemy:    typeStr = "Enemy"; break;
	case NoteType::Orb:      typeStr = "Orb"; break;
	case NoteType::Barrier:  typeStr = "Barrier"; break;
	case NoteType::Hold:     typeStr = "Hold"; break;
	case NoteType::RopeHold: typeStr = "RopeHold"; break;
	}

	hal::dout << "[MISS] Type: " << typeStr
	          << ", Beat: " << m_Beat
	          << ", Lane: " << m_LaneIndex
	          << ", Face: " << m_Face << std::endl;
}

bool IsCornerAdjacent(int lane1, int face1, int lane2, int face2)
{
	static const int pairs[4][4] = {
		// { faceA, laneA, faceB, laneB }
		{ 0, -1, 1, -1 }, // Floor LANE_LEFT  <-> LeftWall  LANE_LEFT
		{ 0,  1, 3, -1 }, // Floor LANE_RIGHT <-> RightWall LANE_LEFT
		{ 2, -1, 1,  1 }, // Ceiling LANE_LEFT  <-> LeftWall  LANE_RIGHT
		{ 2,  1, 3,  1 }, // Ceiling LANE_RIGHT <-> RightWall LANE_RIGHT
	};

	for (const auto& p : pairs)
	{
		if ((face1 == p[0] && lane1 == p[1] && face2 == p[2] && lane2 == p[3]) ||
			(face1 == p[2] && lane1 == p[3] && face2 == p[0] && lane2 == p[1]))
			return true;
	}
	return false;
}

bool IsSameOrCornerPosition(int lane1, int face1, int lane2, int face2)
{
	if (lane1 == lane2 && face1 == face2) return true;
	return IsCornerAdjacent(lane1, face1, lane2, face2);
}