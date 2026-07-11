#include "define.h"
#include "game.h"
#include "note_manager.h"
#include "sound.h"
#include "enemy_note.h"
#include "orb_note.h"
#include "barrier_note.h"
#include "hold_note.h"
#include "rainbow_note.h"
#include "debug_params.h"
#include "enemy_defeat_effect.h"
#include "orb_collect_effect.h"
#include "camera.h"
#include <algorithm>

static const float HIT_ZONE_Z     = 3.0f;
static const float PASSIVE_ZONE_Z = 0.5f; // Orb・Barrier の自動判定Z
static const float PERFECT_WINDOW = 1.0f;
static const float GOOD_WINDOW    = 2.5f;
static const float ROPE_ACTIVATE_WINDOW = 0.5f; // レインボーはプレイヤーの足元でのみ活性化（PASSIVE_ZONE_Z基準）

// beat を「そのノーツをスポーンすべき時刻（秒）」に変換
float NoteManager::BeatToSpawnTime(float beat) const
{
	float hitTime    = BeatToSeconds(beat);
	float travelTime = (m_SpawnZ - HIT_ZONE_Z) / m_NoteSpeed;
	return hitTime - travelTime;
}

float NoteManager::BeatToSeconds(float beat) const
{
	return beat * 60.0f / m_ScoreData.bpm;
}

float NoteManager::BeatToZ(float beat) const
{
	return (BeatToSeconds(beat) - m_ElapsedTime) * m_NoteSpeed + HIT_ZONE_Z;
}

int NoteManager::WallToFace(ScoreWall wall) const
{
	switch (wall)
	{
	case ScoreWall::Down:  return 0; // FACE_FLOOR
	case ScoreWall::Left:  return 1; // FACE_LEFT_WALL
	case ScoreWall::Up:    return 2; // FACE_CEILING
	case ScoreWall::Right: return 3; // FACE_RIGHT_WALL
	default:               return 0;
	}
}

JUDGE NoteManager::JudgeByDistance(NoteBase* note, float targetZ)
{
	float dist = fabsf(note->GetPosZ() - targetZ);
	JUDGE result = JUDGE_NONE;
	if (dist < PERFECT_WINDOW) result = JUDGE_PERFECT;
	else if (dist < GOOD_WINDOW) result = JUDGE_GOOD;
	if (result == JUDGE_NONE) return result;

	// エネミーが消える前の座標を使って撃破パーティクルを生成する。
	if (m_pEnemyDefeatEffect && dynamic_cast<EnemyNote*>(note))
		m_pEnemyDefeatEffect->Spawn(note->GetPos(), note->GetFace());

	note->OnHit();
	return result;
}

void NoteManager::Init(const std::string& scoreFilePath)
{
	// 撃破エフェクトはNoteManagerが生成から解放まで所有する。
	if (!m_pEnemyDefeatEffect)
		m_pEnemyDefeatEffect = new EnemyDefeatEffect();
	if (!m_pOrbCollectEffect)
		m_pOrbCollectEffect = new OrbCollectEffect();

	m_NoteSpeed      = 10.0f;
	m_SpawnZ         = 80.0f;
	m_ElapsedTime    = -3.0f;
	m_NextEventIndex = 0;
	m_BgmStarted     = false;
	m_IsFadingOut    = false;
	m_FadeOutDuration = 0.0f;
	m_FadeOutTimer   = 0.0f;
	m_FadeOutStartVolume = 1.0f;

	m_ScoreData = LoadScore(scoreFilePath);

	std::string bgmPath = "asset/score/" + m_ScoreData.music;
	m_pBgmData = LoadMP3(bgmPath);
	m_pOrbGetsSe = LoadMP3("asset/sound/se/orbgets.wav");
	m_pRainbowSe = LoadMP3("asset/sound/se/Rainbow.wav");
	m_RainbowSePlaying = false;
}

void NoteManager::Update(int playerLane, int playerFace)
{
	// BGMフェードアウト処理
	if (m_IsFadingOut && m_pBgmData && m_pBgmData->pSourceVoice)
	{
		m_FadeOutTimer += dt;
		float volume = m_FadeOutStartVolume * (1.0f - (m_FadeOutTimer / m_FadeOutDuration));
		if (volume < 0.0f) volume = 0.0f;
		m_pBgmData->pSourceVoice->SetVolume(volume);
	}

	if (!m_BgmStarted)
	{
		m_ElapsedTime += dt;
		if (m_ElapsedTime >= 0.0f)
		{
			if (m_pBgmData != nullptr)
			{
				PlaySound(m_pBgmData, false);
			}
			m_BgmStarted = true;
		}
	}
	else
	{
		if (m_pBgmData != nullptr)
		{
			m_ElapsedTime = (float)GetPlaybackPositionSec(m_pBgmData);
		}
		else
		{
			m_ElapsedTime += dt;
		}
	}

	// スポーン処理：時刻が来たイベントを順番に生成
	while (m_NextEventIndex < (int)m_ScoreData.events.size())
	{
		const ScoreEvent& ev = m_ScoreData.events[m_NextEventIndex];
		if (m_ElapsedTime < BeatToSpawnTime(ev.beat)) break;

		int face = WallToFace(ev.wall);

		// 現時刻でノーツが居るべきZ座標を計算（遅延スポーン時は手前に補正）
		float initZ = BeatToZ(ev.beat);

		// JSON lane (0=左, 1=中央, 2=右) → ゲーム lane (-1=左, 0=中央, 1=右) に変換
		int gameLane = ev.lane - 1;

		switch (ev.type)
		{
		case ScoreType::Enemy:
		{
			EnemyNote* note = new EnemyNote();
			note->Init(gameLane, face, initZ, m_NoteSpeed);
			m_Notes.push_back(note);
			break;
		}
		case ScoreType::Orb:
		{
			OrbNote* note = new OrbNote();
			note->Init(gameLane, face, initZ, m_NoteSpeed);
			m_Notes.push_back(note);
			break;
		}
		case ScoreType::Barrier:
		{
			BarrierNote* note = new BarrierNote();
			note->Init(gameLane, face, initZ, m_NoteSpeed, ev.beat);
			m_Notes.push_back(note);
			break;
		}
		case ScoreType::Hold:
		{
			float endZ = BeatToZ(ev.endBeat);

			HoldNote* note = new HoldNote();
			note->Init(ev.lane, ev.endLane, face, initZ, endZ, m_NoteSpeed, m_ScoreData.bpm);
			m_Notes.push_back(note);
			break;
		}
		case ScoreType::RopeHold:
		{
			float endZ        = BeatToZ(ev.endBeat);
			int   endFace     = WallToFace(ev.endWall);
			int   gameEndLane = ev.endLane - 1;

			RopeHoldNote* note = new RopeHoldNote();
			note->Init(gameLane, gameEndLane, face, endFace, initZ, endZ, m_NoteSpeed);
			m_Notes.push_back(note);
			break;
		}
		default:
			break;
		}
		m_NextEventIndex++;
	}

	// 更新・自動判定・削除
	for (int i = (int)m_Notes.size() - 1; i >= 0; i--)
	{
		m_Notes[i]->Update();

		if (!m_Notes[i]->IsHit())
		{
			float z = m_Notes[i]->GetPosZ();

			// Orb: PASSIVE_ZONE_Zよりwindow分手前からlane・faceの一致判定を開始する（早期HIT用の猶予）。
			// ただしMiss確定ラインはPASSIVE_ZONE_Zのまま変えない（プレイヤーの足元まで表示され続けるのを防ぐ）
			if (OrbNote* orb = dynamic_cast<OrbNote*>(m_Notes[i]))
			{
				if (z <= HIT_ZONE_Z)
				{
					if (m_Notes[i]->GetLaneIndex() == playerLane &&
						m_Notes[i]->GetFace()      == playerFace)
					{
						// Orbが消える前の表示座標から取得パーティクルを生成する。
						if (m_pOrbCollectEffect)
							m_pOrbCollectEffect->Spawn(orb->GetEffectPosition(), orb->GetFace());
						orb->OnHit();
						m_PendingOrbEvents.push(ORB_EVENT_HIT);
						if (m_pOrbGetsSe != nullptr)
						{
							PlaySound(m_pOrbGetsSe, false);
						}
					}
					else if (z < HIT_ZONE_Z - GOOD_WINDOW)
					{
						orb->OnMiss();
						m_PendingOrbEvents.push(ORB_EVENT_MISS);
					}
				}
			}
			// Barrier: タイミングはEnemyノーツと一緒（z < HIT_ZONE_Z - GOOD_WINDOW）
			else if (BarrierNote* barrier = dynamic_cast<BarrierNote*>(m_Notes[i]))
			{
				if (z < HIT_ZONE_Z - GOOD_WINDOW)
				{
					float beat = barrier->GetBeat();
					if (m_Notes[i]->GetLaneIndex() == playerLane &&
						m_Notes[i]->GetFace()      == playerFace)
					{
						barrier->OnMiss();
						m_PendingJudges.push(JUDGE_MISS);
						m_ProcessedBarrierBeats.insert(beat);
					}
					else
					{
						// 操作をしなかった（元から安全な場所にいた）場合は、音や判定を出さずに自然消滅
						barrier->OnHit();
						if (m_ProcessedBarrierBeats.find(beat) == m_ProcessedBarrierBeats.end())
						{
							m_PendingJudges.push(JUDGE_SILENT_COMBO);
							m_ProcessedBarrierBeats.insert(beat);
						}
					}
				}
			}
			// Enemy: 判定窓を通過したら押し逃しMiss
			// HoldNote・RopeHoldNote は自身で Miss 処理するのでスキップ
			else if (!dynamic_cast<HoldNote*>(m_Notes[i]) &&
			         !dynamic_cast<RopeHoldNote*>(m_Notes[i]) &&
			         z < HIT_ZONE_Z - GOOD_WINDOW)
			{
				m_Notes[i]->OnMiss();
				m_PendingJudges.push(JUDGE_MISS); // StatusManager に伝える
			}
		}

		if (!m_Notes[i]->IsActive())
		{
			// RopeHoldNote 完了時のスコアをキューに積む
			if (RopeHoldNote* rope = dynamic_cast<RopeHoldNote*>(m_Notes[i]))
			{
				if (rope->GetState() == RopeHoldNote::State::COMPLETE)
					m_PendingJudges.push(JUDGE_PERFECT);
			}
			delete m_Notes[i];
			m_Notes.erase(m_Notes.begin() + i);
		}
	}

	// RopeHoldNote (Rainbow) の再生・フェードアウト制御
	RopeHoldNote* holdingRope = GetHoldingRope();
	if (holdingRope != nullptr)
	{
		if (!m_RainbowSePlaying)
		{
			if (m_pRainbowSe != nullptr)
			{
				PlaySound(m_pRainbowSe, true);
			}
			m_RainbowSePlaying = true;
		}

		// 音量のフェードアウト制御 (進捗85%以降で音量1.0から0.0へ減衰)
		float progress = holdingRope->GetHoldProgress();
		float vol = 1.0f;
		if (progress >= 0.85f)
		{
			vol = (1.0f - progress) / 0.15f;
			if (vol < 0.0f) vol = 0.0f;
		}

		if (m_pRainbowSe != nullptr && m_pRainbowSe->pSourceVoice != nullptr)
		{
			m_pRainbowSe->pSourceVoice->SetVolume(vol * SOUND_SE_VOLUME);
		}
	}
	else
	{
		if (m_RainbowSePlaying)
		{
			if (m_pRainbowSe != nullptr)
			{
				StopSound(m_pRainbowSe);
				if (m_pRainbowSe->pSourceVoice != nullptr)
				{
					m_pRainbowSe->pSourceVoice->SetVolume(SOUND_SE_VOLUME);
				}
			}
			m_RainbowSePlaying = false;
		}
	}

	// ノーツが消えた後も、残っている粒子は寿命まで更新する。
	if (m_pEnemyDefeatEffect)
		m_pEnemyDefeatEffect->Update(dt);
	if (m_pOrbCollectEffect)
		m_pOrbCollectEffect->Update(dt);
}

void NoteManager::Draw()
{
	std::vector<OrbNote*> sortedOrbs;
	sortedOrbs.reserve(m_Notes.size());

	// 不透明ノーツを先に描き、透過Orbは深度ソート用に分ける。
	for (NoteBase* note : m_Notes)
	{
		if (OrbNote* orb = dynamic_cast<OrbNote*>(note))
			sortedOrbs.push_back(orb);
		else
			note->Draw();
	}

	const XMMATRIX view = GetCamera()->GetView();
	std::stable_sort(
		sortedOrbs.begin(),
		sortedOrbs.end(),
		[&view](const OrbNote* lhs, const OrbNote* rhs)
		{
			return lhs->GetDrawDepth(view) > rhs->GetDrawDepth(view);
		});
	for (OrbNote* orb : sortedOrbs)
		orb->Draw();

	// ノーツより後に描き、撃破エフェクトを手前へ見せる。
	if (m_pEnemyDefeatEffect)
		m_pEnemyDefeatEffect->Draw();
	if (m_pOrbCollectEffect)
		m_pOrbCollectEffect->Draw();
}

void NoteManager::DrawShadowMapForFace(int face, const XMMATRIX& lightView, const XMMATRIX& lightProjection)
{
	// 指定された面のEnemyモデルとOrbビルボードをShadowMapへ描く。
	for (NoteBase* note : m_Notes)
	{
		if (!note->IsActive() || note->IsHit()) continue;
		if (note->GetFace() != face) continue;
		if (dynamic_cast<EnemyNote*>(note) || dynamic_cast<OrbNote*>(note))
		{
			note->DrawShadowMap(lightView, lightProjection);
		}
	}
}

void NoteManager::Finalize()
{
	if (m_pBgmData != nullptr) {
		StopSound(m_pBgmData);
		UnloadSound(m_pBgmData);
		m_pBgmData = nullptr;
	}
	if (m_pOrbGetsSe != nullptr) {
		UnloadSound(m_pOrbGetsSe);
		m_pOrbGetsSe = nullptr;
	}
	if (m_pRainbowSe != nullptr) {
		StopSound(m_pRainbowSe);
		UnloadSound(m_pRainbowSe);
		m_pRainbowSe = nullptr;
	}
	m_RainbowSePlaying = false;

	for (NoteBase* note : m_Notes)
		delete note;
	m_Notes.clear();
	delete m_pEnemyDefeatEffect;
	m_pEnemyDefeatEffect = nullptr;
	delete m_pOrbCollectEffect;
	m_pOrbCollectEffect = nullptr;
	RopeHoldNote::FinalizeSharedResources();
}

JUDGE NoteManager::Judge(int lane, int face)
{
	// Enemy の判定（KeyTrigger）
	NoteBase* bestNote = nullptr;
	float bestDist = FLT_MAX;

	for (NoteBase* note : m_Notes)
	{
		if (!note->IsActive() || note->IsHit()) continue;
		if (note->GetLaneIndex() != lane || note->GetFace() != face) continue;
		if (dynamic_cast<OrbNote*>(note) || dynamic_cast<BarrierNote*>(note)) continue;
		if (dynamic_cast<RopeHoldNote*>(note)) continue; // ロープホールドは別扱い
		if (dynamic_cast<HoldNote*>(note)) continue;     // Hold本体は非対象（子ノートで判定）

		float dist = fabsf(note->GetPosZ() - HIT_ZONE_Z);
		if (dist < bestDist)
		{
			bestDist = dist;
			bestNote = note;
		}
	}

	if (bestNote)
	{
		return JudgeByDistance(bestNote, HIT_ZONE_Z);
	}

	// HoldNote（連撃）の最初の一撃はKeyTriggerで取る
	for (NoteBase* note : m_Notes)
	{
		HoldNote* hold = dynamic_cast<HoldNote*>(note);
		if (!hold || !hold->IsActive()) continue;

		EnemyNote* child = hold->GetNearestActiveChild(lane, face);
		if (!child) continue;

		JUDGE j = JudgeByDistance(child, HIT_ZONE_Z);
		if (j != JUDGE_NONE) return j;
	}

	return JUDGE_NONE;
}

JUDGE NoteManager::JudgeHold(int lane, int face)
{
	// RopeHoldNote: 足元（PASSIVE_ZONE_Z）に来た時だけKeyDownで活性化（スコアは完了時に加算）
	for (NoteBase* note : m_Notes)
	{
		RopeHoldNote* rope = dynamic_cast<RopeHoldNote*>(note);
		if (!rope || rope->GetState() != RopeHoldNote::State::IDLE) continue;
		if (rope->GetLaneIndex() != lane || rope->GetFace() != face) continue;

		float dist = fabsf(rope->GetPosZ() - PASSIVE_ZONE_Z);
		if (dist < ROPE_ACTIVATE_WINDOW)
		{
			rope->Activate();
			return JUDGE_NONE; // 活性化のみ。スコアは Complete 時に PendingJudge で加算
		}
	}

	// RopeHoldNote が HOLDING 中はスコア加算なし（完了時に PendingJudge で加算）
	for (NoteBase* note : m_Notes)
	{
		RopeHoldNote* rope = dynamic_cast<RopeHoldNote*>(note);
		if (rope && rope->GetState() == RopeHoldNote::State::HOLDING)
			return JUDGE_NONE;
	}

	// HoldNote（連撃）の継続判定（KeyDown）
	for (NoteBase* note : m_Notes)
	{
		HoldNote* hold = dynamic_cast<HoldNote*>(note);
		if (!hold || !hold->IsActive()) continue;

		EnemyNote* child = hold->GetNearestActiveChild(lane, face);
		if (!child) continue;

		JUDGE j = JudgeByDistance(child, HIT_ZONE_Z);
		if (j != JUDGE_NONE) return j;
	}
	return JUDGE_NONE; // HoldNote が存在しない／範囲外のときは何もしない
}

RopeHoldNote* NoteManager::GetHoldingRope()
{
	for (NoteBase* note : m_Notes)
	{
		RopeHoldNote* rope = dynamic_cast<RopeHoldNote*>(note);
		if (rope && rope->GetState() == RopeHoldNote::State::HOLDING)
			return rope;
	}
	return nullptr;
}

JUDGE NoteManager::OnButtonRelease(int lane, int face)
{
	for (NoteBase* note : m_Notes)
	{
		RopeHoldNote* rope = dynamic_cast<RopeHoldNote*>(note);
		if (!rope || rope->GetState() != RopeHoldNote::State::HOLDING) continue;

		float progress = rope->GetHoldProgress();
		rope->Release();
		return (progress >= 0.5f) ? JUDGE_GOOD : JUDGE_MISS;
	}
	return JUDGE_NONE;
}

bool NoteManager::IsFinished() const
{
	return (m_NextEventIndex >= (int)m_ScoreData.events.size()) && m_Notes.empty();
}

void NoteManager::StartBgmFadeOut(float durationSec)
{
	if (durationSec <= 0.0f)
	{
		durationSec = dt;
	}

	m_IsFadingOut = true;
	m_FadeOutDuration = durationSec;
	m_FadeOutTimer = 0.0f;
	m_FadeOutStartVolume = 1.0f;

	if (m_pBgmData && m_pBgmData->pSourceVoice)
	{
		m_pBgmData->pSourceVoice->GetVolume(&m_FadeOutStartVolume);
	}
}

bool NoteManager::CheckAndHitBarrier(int fromLane, int fromFace, int toLane, int toFace)
{
	// 1. 移動元にバリアがあるかチェック（回避成功）
	for (NoteBase* note : m_Notes)
	{
		BarrierNote* barrier = dynamic_cast<BarrierNote*>(note);
		if (!barrier || !barrier->IsActive() || barrier->IsHit()) continue;
		if (barrier->GetLaneIndex() != fromLane || barrier->GetFace() != fromFace) continue;

		float dist = fabsf(barrier->GetPosZ() - HIT_ZONE_Z);
		if (dist < GOOD_WINDOW)
		{
			barrier->OnHit();
			float beat = barrier->GetBeat();
			if (m_ProcessedBarrierBeats.find(beat) == m_ProcessedBarrierBeats.end())
			{
				m_PendingJudges.push(JUDGE_KAIHI);
				m_ProcessedBarrierBeats.insert(beat);
			}
			m_PendingBarrierEvents.push(BARRIER_EVENT_KAIHI);
			return true;
		}
	}

	// 2. 移動先にバリアがあるかチェック（被弾）
	for (NoteBase* note : m_Notes)
	{
		BarrierNote* barrier = dynamic_cast<BarrierNote*>(note);
		if (!barrier || !barrier->IsActive() || barrier->IsHit()) continue;
		if (barrier->GetLaneIndex() != toLane || barrier->GetFace() != toFace) continue;

		float dist = fabsf(barrier->GetPosZ() - HIT_ZONE_Z);
		if (dist < GOOD_WINDOW)
		{
			barrier->OnMiss();
			m_PendingJudges.push(JUDGE_MISS);
			float beat = barrier->GetBeat();
			m_ProcessedBarrierBeats.insert(beat);
			return true;
		}
	}
	return false;
}
