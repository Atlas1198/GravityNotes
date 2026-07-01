#pragma once
#include "note_manager.h"
#include "scene.h"

class StatusManager
{
private:
	int   m_HP;
	int   m_MaxHP;
	int   m_Score;
	int   m_Combo;
	int   m_MaxCombo;
	int   m_HitCount;
	int   m_MissCount;

public:
	void Init(int maxHP = 10);
	void Finalize();

	void OnJudge(JUDGE result);
	void OnJudgeHold(JUDGE result);

	int  GetHP()       const { return m_HP; }
	int  GetMaxHP()    const { return m_MaxHP; }
	int  GetScore()    const { return m_Score; }
	int  GetCombo()    const { return m_Combo; }
	int  GetMaxCombo() const { return m_MaxCombo; }
	bool IsDead()      const { return m_HP <= 0; }

	RESULT GetResult() const;
};
