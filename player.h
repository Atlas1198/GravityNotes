#pragma once
#include "renderer.h"
#include "sprite3d.h"
enum LANE {
	LANE_LEFT = -1,
	LANE_CENTER,
	LANE_RIGHT
};

enum FACE {
	FACE_FLOOR = 0,
	FACE_LEFT_WALL,
	FACE_CEILING,
	FACE_RIGHT_WALL
};

class Player :public Sprite3D
{
private:
	int m_LaneIndex;
	int m_TargetLaneIndex;
	int m_GravityFace;
	int m_TargetFace;
	XMFLOAT3 m_TargetPos;
	XMFLOAT3 m_TargetRot;
	bool m_IsMoving;

public:
	// デフォルトコンストラクタを追加し、基底 Sprite3D のコンストラクタを呼ぶ
	Player()
		: Sprite3D(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), "asset/model/kirbyanim.fbx", S_UNLIT)
	{}
	void Init();
	void Update();
	void Draw();
	void Finalize();
};