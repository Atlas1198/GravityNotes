#include "game.h"
#include "orb_note.h"
#include "split_bilboard.h"

namespace
{
	const int   ORB_SHEET_COLS   = 5;
	const int   ORB_SHEET_ROWS   = 11;
	const int   ORB_USABLE_FRAMES = 54; // グリッドは55マスだが最後の1マスは未使用
	const float ORB_ANIM_FPS     = 18.0f;
	const XMFLOAT2 ORB_BILLBOARD_SIZE = { 2.0f, 2.0f };
}

OrbNote::~OrbNote()
{
	if (m_pBillboard)
	{
		delete m_pBillboard;
		m_pBillboard = nullptr;
	}
}

void OrbNote::Init(int lane, int face, float spawnZ, float speed)
{
	// fbxモデルは使用しないため modelPath は渡さない
	NoteBase::Init(lane, face, spawnZ, speed, nullptr);

	m_AnimTimer = 0.0f;

	if (!m_pBillboard)
	{
		m_pBillboard = new SplitBilBoard(
			ORB_SHEET_COLS, ORB_SHEET_ROWS,
			GetPos(), ORB_BILLBOARD_SIZE, { 0.0f, 0.0f, 0.0f },
			"asset/texture/OrbAnimationSpriteSheetRed.png",
			false
		);
		m_pBillboard->SetBillboardMode(true);
	}
	m_pBillboard->SetPos(GetPos());
	m_pBillboard->SetTextureIndex(0);
}

void OrbNote::Update()
{
	NoteBase::Update();

	if (m_pBillboard)
	{
		m_pBillboard->SetPos(GetPos());

		// SplitBilBoardの自動アニメーションはグリッド全体(55駒)を回してしまうため、
		// 使用駒数(54駒)だけを手動でループさせる
		m_AnimTimer += dt;
		float interval = 1.0f / ORB_ANIM_FPS;
		if (m_AnimTimer >= interval)
		{
			m_AnimTimer -= interval;
			int frame = (m_pBillboard->GetTextureIndex() + 1) % ORB_USABLE_FRAMES;
			m_pBillboard->SetTextureIndex(frame);
		}
	}
}

void OrbNote::Draw()
{
	if (m_pBillboard)
		m_pBillboard->Draw();
}
