#include "field.h"
#include "shadermanager.h"
#include "define.h"
#include "game.h"
#include <float.h>

using namespace DirectX;

void Field::Init() {
	m_Scale = { 5.0f,5.0f,5.0f };
	m_ModelNormal = ModelLoad("asset/model/field_allnormal.fbx");
	m_ModelHasiranashi = ModelLoad("asset/model/field_hasiranashi.fbx");
	m_Model = m_ModelHasiranashi;
	m_ShaderType = S_LAMBERT;

	// Sprite3Dのメンバ変数を初期化
	m_ModelSize = ModelGetSize(m_ModelNormal);
	m_OriginalColor = ModelGetAverageMaterialColor(m_ModelNormal);

	float L = GetDisplaySize().z;
	if (L <= 0.1f) {
		L = 20.0f; // フォールバック値
	}

	for (int i = 0; i < NUM_FIELDS; ++i) {
		m_ScrollPos[i] = (float)i * L;
		m_FieldModels[i] = m_ModelHasiranashi;
	}
	m_bInitializedModels = false;
}

void Field::Update(float speed, float bpm, float elapsedTime){
	if (bpm <= 0.0f || speed <= 0.0f) return;

	// 1小節のZ長さを計算
	float L_bar = 4.0f * (60.0f / bpm) * speed;

	// モデルのZスケールを調整し、モデル1枚の長さがちょうど1小節になるようにする
	if (m_ModelSize.z > 0.01f) {
		m_Scale.z = L_bar / m_ModelSize.z;
	}
	float L = GetDisplaySize().z; // スケール後の長さ

	// 初回呼び出し時に各配置を小節線に同期（スナップ）して初期化
	// ※判定に使うHIT_ZONE_Zは3.0f。i=0は手前カバー用（3.0f - L）
	if (!m_bInitializedModels) {
		for (int i = 0; i < NUM_FIELDS; ++i) {
			m_ScrollPos[i] = 3.0f + (float)(i - 1) * L;

			float beat = ((m_ScrollPos[i] - 3.0f) / speed + elapsedTime) * (bpm / 60.0f);
			int measure = (int)roundf(beat / 4.0f);
			if (measure % 4 == 0) {
				m_FieldModels[i] = m_ModelNormal;
			} else {
				m_FieldModels[i] = m_ModelHasiranashi;
			}
		}
		m_bInitializedModels = true;
	}

	// すべてのフィールドを手前にスクロール
	for (int i = 0; i < NUM_FIELDS; ++i) {
		m_ScrollPos[i] -= speed * dt;
	}

	// 画角から見切れたらまた奥に戻す
	// カメラのZ座標は -8.0f。手前見切れ境界を -10.0f とする。
	float limitZ = -10.0f;
	for (int i = 0; i < NUM_FIELDS; ++i) {
		if (m_ScrollPos[i] + L / 2.0f < limitZ) {
			// 最も奥にあるモデルのZ座標を探す
			float maxZ = -FLT_MAX;
			for (int j = 0; j < NUM_FIELDS; ++j) {
				if (m_ScrollPos[j] > maxZ) {
					maxZ = m_ScrollPos[j];
				}
			}
			// その奥に連結して配置
			m_ScrollPos[i] = maxZ + L;

			// 新しい奥の位置のモデルタイプを判定
			float beat = ((m_ScrollPos[i] - 3.0f) / speed + elapsedTime) * (bpm / 60.0f);
			int measure = (int)roundf(beat / 4.0f);
			if (measure % 4 == 0) {
				m_FieldModels[i] = m_ModelNormal;
			} else {
				m_FieldModels[i] = m_ModelHasiranashi;
			}
		}
	}
}

void Field::Draw() {
	XMFLOAT3 originalPos = GetPos();
	for (int i = 0; i < NUM_FIELDS; ++i) {
		XMFLOAT3 tempPos = originalPos;
		tempPos.z = m_ScrollPos[i];
		SetPos(tempPos);
		m_Model = m_FieldModels[i];
		Sprite3D::Draw();
	}
	SetPos(originalPos);
}

void Field::Finalize() {
	if (m_ModelNormal) {
		ModelRelease(m_ModelNormal);
		m_ModelNormal = nullptr;
	}
	if (m_ModelHasiranashi) {
		ModelRelease(m_ModelHasiranashi);
		m_ModelHasiranashi = nullptr;
	}
	m_Model = nullptr; // Sprite3Dの二重解放防止
}