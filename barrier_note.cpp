#include "game.h"
#include "barrier_note.h"

void BarrierNote::Init(int lane, int face, float spawnZ, float speed)
{
	NoteBase::Init(lane, face, spawnZ, speed, "asset/model/cube.fbx");
	m_ShaderType = S_LAMBERT;
	SetColor(0.0f, 1.0f, 0.0f);
}

