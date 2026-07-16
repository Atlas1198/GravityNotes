#pragma once
#include "note_base.h"

class BarrierNote : public NoteBase
{
private:
	float m_Beat = 0.0f;
	bool  m_IsHiddenByPriority = false;
public:
	BarrierNote() : NoteBase() {}

	NoteType GetType() const override { return NoteType::Barrier; }

	void Init(int lane, int face, float spawnZ, float speed, float beat);
	float GetBeat() const { return m_Beat; }

	void OnHit() override;
	void OnMiss() override;
	void Draw() override;

	void SetHiddenByPriority(bool hidden) { m_IsHiddenByPriority = hidden; }
	bool IsHiddenByPriority() const { return m_IsHiddenByPriority; }
};