#pragma once
#include "enemy_note.h"
#include <vector>
#include <queue>

// ======================================================
// HoldNote: EnemyNote を一定間隔で並べた長押しノーツ
// 子ノートの間隔は hold_note.cpp の HOLD_BEAT_INTERVAL で変更可能
// ======================================================
class HoldNote : public NoteBase
{
private:
	std::vector<EnemyNote*> m_ChildNotes;

	// 子ノートの押し逃しMiss判定用（NoteManagerが毎フレームSetPlayerPositionで更新する）
	int m_PlayerLane = 0;
	int m_PlayerFace = 0;
	// true = lane一致（ダメージあり）、false = lane不一致（ダメージなし）
	std::queue<bool> m_PendingMissJudges;

public:
	HoldNote() : NoteBase() {}
	~HoldNote();

	// lane    : 始点レーン (0-2)
	// endLane : 終点レーン (0-2)
	// face    : 壁面 (FACE_*)
	// initZ   : 始点の初期Z座標
	// endZ    : 終点の初期Z座標
	// speed   : ノーツ速度
	// bpm     : 曲のBPM
	void Init(int lane, int endLane, int face, float initZ, float endZ, float speed, float bpm);
	void Update() override;
	void Draw()   override;

	// lane・face が一致する最も手前の未ヒット子ノートを返す（なければ nullptr）
	EnemyNote* GetNearestActiveChild(int lane, int face) const;

	// 子ノートの押し逃しMiss判定に使うプレイヤー位置。Update()より前に呼ぶこと
	void SetPlayerPosition(int lane, int face) { m_PlayerLane = lane; m_PlayerFace = face; }
	bool HasPendingMissJudge() const { return !m_PendingMissJudges.empty(); }
	bool PopMissJudge() { bool v = m_PendingMissJudges.front(); m_PendingMissJudges.pop(); return v; }
};
