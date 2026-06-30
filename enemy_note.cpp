#include "game.h"
#include "enemy_note.h"

void EnemyNote::Init(int lane, int face, float spawnZ, float speed)
{
	NoteBase::Init(lane, face, spawnZ, speed, "asset/model/Gargoyle.fbx");
	m_Scale = { 0.05f,0.05f,0.05f };
	m_ShaderType = S_LAMBERT;
}

void EnemyNote::OnHit()
{
	NoteBase::OnHit();
	// TODO: スコア加算・コンボ継続
}

void EnemyNote::OnMiss()
{
	NoteBase::OnMiss();
	// TODO: HP減少・コンボリセット
}