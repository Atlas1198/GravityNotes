#define NOMINMAX
#include "rainbow_note.h"
#include "game.h"
#include "debug_ostream.h"
#include "debug_params.h"
#include <algorithm>
#include <cmath>

static const float ROPE_HIT_ZONE_Z   = 0.0f; // 叩く基準(3.0f)に対して後ろ判定の限界Z値（3.0f - 1.5f より少し後ろに設定）
static const float ROPE_START_ZONE_Z = 1.0f;   // 活性化（開始）の基準Z座標（HIT_ZONE_Zと同値）
static const float ROPE_ACTIVE_RANGE = 0.0f;
static const float TUNNEL_HALF       = 2.5f;
static const float RIBBON_INSET      = 0.13f;  // z-fighting防止：壁から内側にずらす量
static const int   TILE_COLS         = 6;
static const int   TILE_ROWS         = 5;
static const int   TILE_COUNT        = TILE_COLS * TILE_ROWS; // 30
static const int   RIBBON_SUBDIV     = 4; // 1タイルあたりの細分数（大きいほど滑らか）

// ---------- 3D quad vertex buffer ----------
static ID3D11Buffer* g_RibbonVB = nullptr;

static void EnsureRibbonVB()
{
	if (g_RibbonVB) return;
	D3D11_BUFFER_DESC bd = {};
	bd.Usage          = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth      = sizeof(Vertex3D) * 4;
	bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	GetDevice()->CreateBuffer(&bd, nullptr, &g_RibbonVB);
}

// ---------- geometry helpers ----------
static XMFLOAT2 FaceToXY(int face, int lane)
{
	float v = lane * LANE_WIDTH;
	switch (face) {
	case 0: return {  v,           -TUNNEL_HALF };
	case 1: return { -TUNNEL_HALF,  v           };
	case 2: return {  v,            TUNNEL_HALF };
	case 3: return {  TUNNEL_HALF,  v           };
	}
	return { 0.0f, 0.0f };
}

static XMFLOAT2 FaceNormal(int face)
{
	switch (face) {
	case 0: return {  0.0f,  1.0f };
	case 1: return {  1.0f,  0.0f };
	case 2: return {  0.0f, -1.0f };
	case 3: return { -1.0f,  0.0f };
	}
	return { 0.0f, 1.0f };
}

// ベジェ曲線の制御点（トンネルの内角）
static XMFLOAT2 CornerXY(int face0, int face1)
{
	auto isHoriz = [](int f) { return f == 0 || f == 2; };
	int horizFace = isHoriz(face0) ? face0 : face1;
	int vertFace  = isHoriz(face0) ? face1 : face0;
	float cx = (vertFace  == 3) ?  TUNNEL_HALF : -TUNNEL_HALF;
	float cy = (horizFace == 2) ?  TUNNEL_HALF : -TUNNEL_HALF;
	return { cx, cy };
}

static XMFLOAT2 QuadBezier(XMFLOAT2 p0, XMFLOAT2 p1, XMFLOAT2 p2, float t)
{
	float u = 1.0f - t;
	return {
		u*u*p0.x + 2.0f*u*t*p1.x + t*t*p2.x,
		u*u*p0.y + 2.0f*u*t*p1.y + t*t*p2.y
	};
}

static XMFLOAT2 Lerp2(XMFLOAT2 a, XMFLOAT2 b, float t)
{
	return { a.x + (b.x - a.x)*t, a.y + (b.y - a.y)*t };
}

static XMFLOAT2 Norm2(XMFLOAT2 v)
{
	float len = sqrtf(v.x*v.x + v.y*v.y);
	if (len < 1e-6f) return { 0.0f, 1.0f };
	return { v.x / len, v.y / len };
}

// ---------- 3D quad draw ----------
static void DrawRibbonQuad(XMFLOAT3 corners[4],
                            float u0, float u1, float vFar, float vNear,
                            ID3D11ShaderResourceView* tex)
{
	EnsureRibbonVB();
	if (!g_RibbonVB || !tex) return;

	auto* ctx = GetDeviceContext();
	auto* sh  = GetShader(S_UNLIT);

	ctx->IASetInputLayout(sh->GetVertexLayout());
	ctx->VSSetShader(sh->GetVertexShader(), nullptr, 0);
	ctx->PSSetShader(sh->GetPixelShader(),  nullptr, 0);

	SetWorldMatrix(XMMatrixIdentity());
	ctx->PSSetShaderResources(0, 1, &tex);
	SetBlendState(BLENDSTATE_ALFA);

	D3D11_MAPPED_SUBRESOURCE msr;
	ctx->Map(g_RibbonVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	Vertex3D* v = (Vertex3D*)msr.pData;
	XMFLOAT3 n = { 0.0f, 0.0f, 1.0f };
	XMFLOAT4 c = { 1.0f, 1.0f, 1.0f, 1.0f };
	// TRIANGLESTRIP: 0-1-2, 1-3-2
	v[0] = { corners[0], n, c, { u0, vNear } }; // near-left
	v[1] = { corners[1], n, c, { u1, vNear } }; // near-right
	v[2] = { corners[2], n, c, { u0, vFar  } }; // far-left
	v[3] = { corners[3], n, c, { u1, vFar  } }; // far-right
	ctx->Unmap(g_RibbonVB, 0);

	UINT stride = sizeof(Vertex3D), offset = 0;
	ctx->IASetVertexBuffers(0, 1, &g_RibbonVB, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	ctx->Draw(4, 0);
}

// ==========================================================

void RopeHoldNote::FinalizeSharedResources()
{
	if (g_RibbonVB)
	{
		g_RibbonVB->Release();
		g_RibbonVB = nullptr;
	}
}

void RopeHoldNote::Init(int startLane, int endLane, const std::vector<int>& facePath,
                        float startZ, float endZ, float speed)
{
	NoteBase::Init(startLane, facePath.front(), startZ, speed, nullptr);
	m_FacePath      = facePath;
	m_EndLane       = endLane;
	m_RopeLength    = endZ - startZ;
	m_HoldProgress  = 0.0f;
	m_State         = State::IDLE;
	m_MissedAtStart = false;
	m_InitialSpawnZ = startZ;

	if (!m_Texture)
		m_Texture = LoadTexture(L"asset/texture/30ver.png");

	// 始点の面情報から位置と角度を求める
	// face: 0=FLOOR, 1=LEFT_WALL, 2=CEILING, 3=RIGHT_WALL
	int startFace = m_FacePath.front();
	float angleZ = 0.0f;
	XMFLOAT2 startXY = EvalCurveXY(0.0f);
	XMFLOAT2 normal = EvalNormal(0.0f);

	// 壁から浮かせるインセット（z-fighting防止：RIBBON_INSET=0.1f と同様）
	// normal方向へ少し内側（トンネルの中心方向）へ移動し、
	// さらに高さが1.0fなので、面（壁面・床面）に接地するようにnormal方向へオフセットする。
	// 板のサイズは 幅 5.0f (TUNNEL_HALF * 2.0f)、高さ 1.0f。板のローカル座標で下端が接地するように、中心から normal * 0.5f オフセットする。
	float inset = RIBBON_INSET + 0.5f; // 接地のため高さの半分(0.5f) + 浮かせる分
	XMFLOAT3 bbPos = {
		startXY.x + normal.x * inset,
		startXY.y + normal.y * inset,
		startZ
	};

	// 角度の設定: 面に応じてZ軸周りに回転させ、壁・天井・床に接地させる
	// デフォルト (0.0f, 0.0f, 0.0f) は床(FLOOR)の上に立つ板
	// face 0(FLOOR):   Z回転 0°
	// face 1(LEFT):    Z回転 -90° (左壁に接地)
	// face 2(CEILING): Z回転 180° (天井に接地)
	// face 3(RIGHT):   Z回転 90° (右壁に接地)
	switch (startFace)
	{
	case 0: angleZ = 0.0f; break;
	case 1: angleZ = -90.0f; break;
	case 2: angleZ = 180.0f; break;
	case 3: angleZ = 90.0f; break;
	}

	// 6列5行の分割ビルボードとして初期化
	m_StartBillboard = SplitBilBoard(6, 5, bbPos, { 4.5, 0.9f }, { 0.0f, 0.0f, angleZ }, "asset/texture/rainbow_start.png", true);
	m_StartBillboard.SetBillboardMode(false);
	m_StartBillboard.SetWallFadeEnabled(false);
	m_StartBillboard.SetColor({ 1.0f, 1.0f, 1.0f, 0.4f });
	m_StartBillboard.SetFPS(30.0f);
	m_StartBillboard.SetAnimationEnabled(true);
	m_StartBillboard.SetLoop(true);
}

// t(0~1) が属するセグメント（面ペア）とそのローカルtを求める
void RopeHoldNote::ResolveSegment(float t, int& faceA, int& faceB, float& localT) const
{
	t = std::max(0.0f, std::min(t, 1.0f));
	const int numSegments = std::max<int>(1, (int)m_FacePath.size() - 1);

	int segIndex = (int)(t * numSegments);
	if (segIndex >= numSegments) segIndex = numSegments - 1;

	localT = t * numSegments - (float)segIndex;

	if (m_FacePath.size() <= 1)
	{
		faceA = faceB = m_FacePath.front();
		localT = 0.0f;
		return;
	}

	faceA = m_FacePath[segIndex];
	faceB = m_FacePath[segIndex + 1];
}

XMFLOAT2 RopeHoldNote::EvalCurveXY(float t) const
{
	int faceA, faceB; float localT;
	ResolveSegment(t, faceA, faceB, localT);

	XMFLOAT2 p0 = FaceToXY(faceA, 0);
	if (faceA == faceB) return p0;

	XMFLOAT2 p2 = FaceToXY(faceB, 0);
	XMFLOAT2 corner = CornerXY(faceA, faceB);
	// コーナー制御点を角(softness=0)〜p0-p2の中点(softness=1)の間でブレンドし、
	// カーブの鋭さを緩和する（回転数が多くセグメントが短いノーツほど効果が大きい）
	XMFLOAT2 mid = Lerp2(p0, p2, 0.5f);
	XMFLOAT2 p1  = Lerp2(corner, mid, D_PARAMS.rainbowCornerSoftness);
	return QuadBezier(p0, p1, p2, localT);
}

XMFLOAT2 RopeHoldNote::EvalNormal(float t) const
{
	int faceA, faceB; float localT;
	ResolveSegment(t, faceA, faceB, localT);
	return Norm2(Lerp2(FaceNormal(faceA), FaceNormal(faceB), localT));
}

void RopeHoldNote::Update()
{
	AddPosZ(-m_Speed * dt);

	if (m_State == State::HOLDING)
	{
		float passed   = ROPE_START_ZONE_Z - GetPosZ();
		m_HoldProgress = std::max(0.0f, std::min(passed / m_RopeLength, 1.0f));
		if (m_HoldProgress >= 1.0f)
			Complete();
		return;
	}

	// IDLE: 判定窓を過ぎたら FAILED_START（始点を取れなかった場合、途中離しと同様に即座に非表示）
	// FAILED（途中離し）とは区別し、NoteManager側でMiss判定を確実に発行できるようにする
	if (m_State == State::IDLE && GetPosZ() < ROPE_HIT_ZONE_Z - ROPE_ACTIVE_RANGE)
	{
		m_State    = State::FAILED_START;
		m_IsActive = false;
	}

	// 始点ビルボードの座標同期と更新
	m_BillboardTimer += dt;
	float wave = sinf(m_BillboardTimer * 4.0f) * 0.4f; // 周期約1.5秒、振幅0.6f

	// 接地位置（法線方向にRIBBON_INSET + 0.5f）をベースに、さらにふわふわ揺らす
	XMFLOAT2 startXY = EvalCurveXY(0.0f);
	XMFLOAT2 normal = EvalNormal(0.0f);
	float inset = RIBBON_INSET + 1.1f + wave;
	XMFLOAT3 bbPos = {
		startXY.x + normal.x * inset,
		startXY.y + normal.y * inset,
		m_Position.z - 1.5f
	};
	m_StartBillboard.SetPos(bbPos);
	m_StartBillboard.Update();
}

void RopeHoldNote::Draw()
{
	if (!m_IsActive || !m_Texture) return;

	const float tileZWidth   = m_Speed * m_LoopTime / (float)TILE_COUNT;
	const float distTraveled = m_InitialSpawnZ - m_Position.z;
	const int   baseTile     = (int)(distTraveled / tileZWidth);

	float halfWidth = TUNNEL_HALF;

	const float drawNear = (m_State == State::HOLDING)
		? -0.2f
		: std::max(-0.2f, m_Position.z);
	const float drawFar  = m_Position.z + m_RopeLength;

	const float geoStep = tileZWidth / RIBBON_SUBDIV;

	for (float z = drawNear; z < drawFar + tileZWidth; z += tileZWidth)
	{
		// タイルインデックスと UV 範囲をタイル単位で決定
		int slotFromHit  = (int)((ROPE_HIT_ZONE_Z - z) / tileZWidth);
		int tileIndex    = ((baseTile + slotFromHit) % TILE_COUNT + TILE_COUNT) % TILE_COUNT;

		int   col      = tileIndex % TILE_COLS;
		int   row      = tileIndex / TILE_COLS;
		float u0       = col       / (float)TILE_COLS;
		float u1       = (col + 1) / (float)TILE_COLS;
		float vNearFull = (row + 1) / (float)TILE_ROWS; // プレイヤー側（z 小）
		float vFarFull  =  row      / (float)TILE_ROWS; // 奥側（z 大）

		for (int s = 0; s < RIBBON_SUBDIV; s++)
		{
			float z0s = z + s       * geoStep;
			float z1s = z + (s + 1) * geoStep;

			// t はリボン全長 m_RopeLength に対する固定割合（描画範囲の縮小で再正規化しない）
			float t0 = std::max(0.0f, std::min((z0s - m_Position.z) / m_RopeLength, 1.0f));
			float t1 = std::max(0.0f, std::min((z1s - m_Position.z) / m_RopeLength, 1.0f));

			XMFLOAT2 xy0 = EvalCurveXY(t0);
			XMFLOAT2 xy1 = EvalCurveXY(t1);

			XMFLOAT2 nrm0 = EvalNormal(t0);
			XMFLOAT2 nrm1 = EvalNormal(t1);

			xy0.x += nrm0.x * RIBBON_INSET; xy0.y += nrm0.y * RIBBON_INSET;
			xy1.x += nrm1.x * RIBBON_INSET; xy1.y += nrm1.y * RIBBON_INSET;

			XMFLOAT2 ac0 = { -nrm0.y, nrm0.x };
			XMFLOAT2 ac1 = { -nrm1.y, nrm1.x };

			XMFLOAT3 corners[4] = {
				{ xy0.x - ac0.x * halfWidth, xy0.y - ac0.y * halfWidth, z0s },
				{ xy0.x + ac0.x * halfWidth, xy0.y + ac0.y * halfWidth, z0s },
				{ xy1.x - ac1.x * halfWidth, xy1.y - ac1.y * halfWidth, z1s },
				{ xy1.x + ac1.x * halfWidth, xy1.y + ac1.y * halfWidth, z1s },
			};

			// タイル内のどの割合に当たるかで UV を補間
			float localT0 = (float)s       / RIBBON_SUBDIV;
			float localT1 = (float)(s + 1) / RIBBON_SUBDIV;
			float vNear = vNearFull + localT0 * (vFarFull - vNearFull);
			float vFar  = vNearFull + localT1 * (vFarFull - vNearFull);

			DrawRibbonQuad(corners, u0, u1, vFar, vNear, m_Texture);
		}
	}

	// 始点ビルボードの描画（IDLE状態のみ表示）
	if (m_State == State::IDLE)
	{
		m_StartBillboard.Draw();
	}
}

void RopeHoldNote::OnHit()
{
	Activate();
}

bool RopeHoldNote::Activate()
{
	if (m_State != State::IDLE) return false;
	m_State = State::HOLDING;
	return true;
}

void RopeHoldNote::Release()
{
	if (m_State != State::HOLDING) return;
	m_State    = State::FAILED;
	m_IsActive = false;
}

void RopeHoldNote::Complete()
{
	m_HoldProgress = 1.0f;
	m_State        = State::COMPLETE;
	m_IsHit        = true;
	m_IsActive     = false;
}
