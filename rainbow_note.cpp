#define NOMINMAX
#include "rainbow_note.h"
#include "game.h"
#include <algorithm>
#include <cmath>

static const float ROPE_HIT_ZONE_Z   = 3.0f;
static const float ROPE_ACTIVE_RANGE = 2.5f;
static const float TUNNEL_HALF       = 2.5f;
static const float RIBBON_HALF_WIDTH = 0.75f;
static const float RIBBON_NEAR_Z     = -3.0f;
static const int   TILE_COLS         = 6;
static const int   TILE_ROWS         = 5;
static const int   TILE_COUNT        = TILE_COLS * TILE_ROWS; // 30

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
static void DrawRibbonQuad(XMFLOAT3 corners[4], int tileIndex,
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

	int col = tileIndex % TILE_COLS;
	int row = tileIndex / TILE_COLS;
	float u0 = col / (float)TILE_COLS, u1 = (col + 1) / (float)TILE_COLS;
	float v0 = row / (float)TILE_ROWS, v1 = (row + 1) / (float)TILE_ROWS;

	D3D11_MAPPED_SUBRESOURCE msr;
	ctx->Map(g_RibbonVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	Vertex3D* v = (Vertex3D*)msr.pData;
	XMFLOAT3 n = { 0.0f, 0.0f, 1.0f };
	XMFLOAT4 c = { 1.0f, 1.0f, 1.0f, 1.0f };
	// TRIANGLESTRIP: 0-1-2, 1-3-2
	v[0] = { corners[0], n, c, { u0, v1 } }; // near-left
	v[1] = { corners[1], n, c, { u1, v1 } }; // near-right
	v[2] = { corners[2], n, c, { u0, v0 } }; // far-left
	v[3] = { corners[3], n, c, { u1, v0 } }; // far-right
	ctx->Unmap(g_RibbonVB, 0);

	UINT stride = sizeof(Vertex3D), offset = 0;
	ctx->IASetVertexBuffers(0, 1, &g_RibbonVB, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	ctx->Draw(4, 0);
}

// ==========================================================

void RopeHoldNote::Init(int startLane, int endLane, int startFace, int endFace,
                        float startZ, float endZ, float speed)
{
	NoteBase::Init(startLane, startFace, startZ, speed, nullptr);
	m_EndFace       = endFace;
	m_EndLane       = endLane;
	m_RopeLength    = endZ - startZ;
	m_HoldProgress  = 0.0f;
	m_State         = State::IDLE;
	m_InitialSpawnZ = startZ;

	if (!m_Texture)
		m_Texture = LoadTexture(L"asset/texture/30ver.png");
}

void RopeHoldNote::Update()
{
	AddPosZ(-m_Speed * dt);

	if (m_State == State::HOLDING)
	{
		float passed   = ROPE_HIT_ZONE_Z - GetPosZ();
		m_HoldProgress = std::max(0.0f, std::min(passed / m_RopeLength, 1.0f));
		if (m_HoldProgress >= 1.0f)
			Complete();
		return;
	}

	if (m_State == State::IDLE && GetPosZ() < ROPE_HIT_ZONE_Z - ROPE_ACTIVE_RANGE)
	{
		m_State    = State::FAILED;
		m_IsActive = false;
	}
}

void RopeHoldNote::Draw()
{
	if (!m_IsActive || !m_Texture) return;

	const float tileZWidth   = m_Speed * m_LoopTime / (float)TILE_COUNT;
	const float distTraveled = m_InitialSpawnZ - m_Position.z;
	const int   baseTile     = (int)(distTraveled / tileZWidth);

	XMFLOAT2 p0 = FaceToXY(m_Face,    m_LaneIndex);
	XMFLOAT2 p2 = FaceToXY(m_EndFace, m_EndLane);
	// 同一面なら直線、異なる面なら角を制御点とするベジェ
	XMFLOAT2 p1 = (m_Face == m_EndFace)
		? Lerp2(p0, p2, 0.5f)
		: CornerXY(m_Face, m_EndFace);

	XMFLOAT2 n0 = FaceNormal(m_Face);
	XMFLOAT2 n2 = FaceNormal(m_EndFace);

	const float ropeEnd = m_Position.z + m_RopeLength;
	const float farZ    = ropeEnd + tileZWidth;

	for (float z = RIBBON_NEAR_Z; z < farZ; z += tileZWidth)
	{
		float z1 = z + tileZWidth;

		float safeLen = (m_RopeLength > 0.001f) ? m_RopeLength : 0.001f;
		float t0 = std::max(0.0f, std::min((z  - m_Position.z) / safeLen, 1.0f));
		float t1 = std::max(0.0f, std::min((z1 - m_Position.z) / safeLen, 1.0f));

		XMFLOAT2 xy0 = QuadBezier(p0, p1, p2, t0);
		XMFLOAT2 xy1 = QuadBezier(p0, p1, p2, t1);

		// リボン幅方向 = 法線を XY 平面で 90° 回転（面の接線方向）
		XMFLOAT2 nrm0 = Norm2(Lerp2(n0, n2, t0));
		XMFLOAT2 nrm1 = Norm2(Lerp2(n0, n2, t1));
		XMFLOAT2 ac0  = { -nrm0.y, nrm0.x };
		XMFLOAT2 ac1  = { -nrm1.y, nrm1.x };

		XMFLOAT3 corners[4] = {
			{ xy0.x - ac0.x * RIBBON_HALF_WIDTH, xy0.y - ac0.y * RIBBON_HALF_WIDTH, z  },
			{ xy0.x + ac0.x * RIBBON_HALF_WIDTH, xy0.y + ac0.y * RIBBON_HALF_WIDTH, z  },
			{ xy1.x - ac1.x * RIBBON_HALF_WIDTH, xy1.y - ac1.y * RIBBON_HALF_WIDTH, z1 },
			{ xy1.x + ac1.x * RIBBON_HALF_WIDTH, xy1.y + ac1.y * RIBBON_HALF_WIDTH, z1 },
		};

		// タイルインデックス：ヒットゾーンからの距離ベースでノーツごとに0スタート
		int slotFromHit = (int)((ROPE_HIT_ZONE_Z - z) / tileZWidth);
		int tileIndex   = ((baseTile + slotFromHit) % TILE_COUNT + TILE_COUNT) % TILE_COUNT;

		DrawRibbonQuad(corners, tileIndex, m_Texture);
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
