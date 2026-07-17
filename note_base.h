#pragma once
#include "sprite3d.h"

enum class NoteType {
	Enemy,
	Orb,
	Barrier,
	Hold,
	RopeHold
};

class NoteBase : public Sprite3D
{
protected:
	int   m_LaneIndex;
	int   m_Face;
	float m_Speed;
	bool  m_IsActive;
	bool  m_IsHit;
	float m_Beat;
	float m_SpawnZ;
	float m_SpawnTimer;

public:
	NoteBase() : Sprite3D(), m_LaneIndex(0), m_Face(0), m_Speed(0.0f), m_IsActive(false), m_IsHit(false), m_Beat(0.0f), m_SpawnZ(0.0f), m_SpawnTimer(0.0f) {}
	virtual ~NoteBase() = default;

	virtual NoteType GetType() const = 0;

	virtual void Init(int lane, int face, float spawnZ, float speed, const char* modelPath);
	virtual void Update();
	virtual void Draw();
	virtual void Finalize();
	virtual void OnHit();
	virtual void OnMiss();

	bool IsActive() const { return m_IsActive; }
	bool IsHit()    const { return m_IsHit; }
	int  GetLaneIndex() const { return m_LaneIndex; }
	int  GetFace()      const { return m_Face; }
	void  SetBeat(float beat) { m_Beat = beat; }
	float GetBeat() const { return m_Beat; }
	float GetFadeInAlpha() const
	{
		if (m_SpawnZ > 40.0f)
		{
			float alpha = m_SpawnTimer / 0.5f;
			return alpha > 1.0f ? 1.0f : alpha;
		}
		return 1.0f;
	}
};

// 面をまたいだ隣接レーンの角判定
// (Floor LANE_LEFT<->LeftWall LANE_LEFT / Floor LANE_RIGHT<->RightWall LANE_LEFT /
//  Ceiling LANE_LEFT<->LeftWall LANE_RIGHT / Ceiling LANE_RIGHT<->RightWall LANE_RIGHT)
// face: 0=FLOOR, 1=LEFT_WALL, 2=CEILING, 3=RIGHT_WALL / lane: -1=LEFT, 0=CENTER, 1=RIGHT
bool IsCornerAdjacent(int lane1, int face1, int lane2, int face2);

// 完全一致、または隣接面の角どうしなら true（Enemy・Hold連撃など、角越しのヒットを許容したい判定に使う）
bool IsSameOrCornerPosition(int lane1, int face1, int lane2, int face2);