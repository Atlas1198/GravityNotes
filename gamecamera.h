#pragma once
#include "camera.h"

class Player;

class GameCamera:public Camera
{
private:
	static GameCamera* s_Instance;

	// カメラ角度の補間用
	float m_CurrentYaw;     // 現在のカメラ水平角度
	float m_CurrentPitch;   // 現在のカメラ垂直角度
	float m_TargetYaw;      // 目標の水平角度
	float m_TargetPitch;    // 目標の垂直角度
	float m_AngleLerpSpeed; // 角度補間速度（0.0f～1.0f）
	float m_DamageShakeRemaining;
	float m_DamageShakeElapsed;

	void ApplyDamageShake(XMFLOAT3& cameraPos, XMFLOAT3& targetPos);

public:
	static void Init();
	static void Update(Player* player);
	static void Draw();
	static void Finalize();
	static void StartDamageShake();

	void UpdateCameraAngleByGravity(int gravityFace);
};
