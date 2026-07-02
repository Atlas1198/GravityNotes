#pragma once
#include <vector>
#include "note_base.h"
#include "scoreloader.h"

enum JUDGE {
	JUDGE_PERFECT,
	JUDGE_GOOD,
	JUDGE_MISS
};

struct SoundData;

class NoteManager
{
private:
	std::vector<NoteBase*> m_Notes;
	float    m_NoteSpeed;
	float    m_SpawnZ;

	ScoreData m_ScoreData;
	float     m_ElapsedTime;
	int       m_NextEventIndex;
	SoundData* m_pBgmData = nullptr;
	bool      m_BgmStarted = false;

	float BeatToSpawnTime(float beat) const;
	int   WallToFace(ScoreWall wall)  const;

public:
	void  Init(const std::string& scoreFilePath);
	void  Update(int playerLane, int playerFace);
	void  Draw();
	void  Finalize();
	float GetNoteSpeed() const { return m_NoteSpeed; }
	float GetBPM() const { return m_ScoreData.bpm; }
	float GetElapsedTime() const { return m_ElapsedTime; }

	JUDGE Judge(int lane, int face);
	JUDGE JudgeHold(int lane, int face); // Hold 長押し中の継続判定
};