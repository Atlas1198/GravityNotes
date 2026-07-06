#pragma once
#include "note_base.h"

class RopeHoldNote : public NoteBase
{
public:
	enum class State { IDLE, HOLDING, COMPLETE, FAILED };

private:
	int   m_EndFace;
	int   m_EndLane;
	float m_RopeLength;    // Z方向のロープ長（= endZ - startZ）
	float m_HoldProgress;  // 0.0（未開始）〜 1.0（完走）
	State m_State;

public:
	RopeHoldNote()
		: NoteBase(), m_EndFace(0), m_EndLane(0),
		  m_RopeLength(0.0f), m_HoldProgress(0.0f), m_State(State::IDLE) {}

	void Init(int startLane, int endLane, int startFace, int endFace,
	          float startZ, float endZ, float speed);
	void Update() override;
	void Draw()   override;
	void OnHit()  override; // Activate() に委譲

	bool Activate(); // IDLE → HOLDING
	void Release();  // 途中離し → FAILED
	void Complete(); // 終端通過 → COMPLETE

	State GetState()        const { return m_State; }
	int   GetEndFace()      const { return m_EndFace; }
	int   GetEndLane()      const { return m_EndLane; }
	float GetHoldProgress() const { return m_HoldProgress; }
};
