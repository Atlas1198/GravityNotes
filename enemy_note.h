#pragma once
#include "note_base.h"

class EnemyNote : public NoteBase
{
private:
	DirectX::XMFLOAT3 m_RimLightColor = { 0.25f, 0.75f, 1.0f };
	float m_RimLightIntensity = 0.75f;

public:
	EnemyNote() : NoteBase() {}

	NoteType GetType() const override { return NoteType::Enemy; }

	// modelPath : nullptrの場合は既定の "asset/model/Gargoyle.fbx" を使用
	void Init(int lane, int face, float spawnZ, float speed, const char* modelPath = nullptr);
	void Draw() override;

	// Gargoyleごとにリムライトの色と強さを変更する。
	void SetRimLightColor(const DirectX::XMFLOAT3& color) { m_RimLightColor = color; }
	void SetRimLightIntensity(float intensity) { m_RimLightIntensity = intensity; }
};
