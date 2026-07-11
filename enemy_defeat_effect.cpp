#include "enemy_defeat_effect.h"

#include <cmath>

using namespace DirectX;

namespace
{
	constexpr int PARTICLE_COUNT = 12;
	constexpr float PI = 3.14159265f;

	XMFLOAT3 MoveInsideTunnel(XMFLOAT3 position, int face)
	{
		// 壁面とのZ-fightingや埋まりを避けるため、発生位置を内側へ寄せる。
		constexpr float INSET = 0.15f;
		switch (face)
		{
		case 0: position.y += INSET; break;
		case 1: position.x += INSET; break;
		case 2: position.y -= INSET; break;
		case 3: position.x -= INSET; break;
		}
		return position;
	}

	void AimInsideTunnel(XMFLOAT3& velocity, int face)
	{
		// 壁の外へ飛ぶ速度成分を反転し、エフェクトをトンネル内に見せる。
		switch (face)
		{
		case 0: velocity.y = std::fabs(velocity.y); break;
		case 1: velocity.x = std::fabs(velocity.x); break;
		case 2: velocity.y = -std::fabs(velocity.y); break;
		case 3: velocity.x = -std::fabs(velocity.x); break;
		}
	}
}

EnemyDefeatEffect::EnemyDefeatEffect()
	: ParticleManager("asset/texture/enemy_defeat_particle.png", 160),
	  m_RandomState(0x4A3B2C1Du)
{
}

float EnemyDefeatEffect::Random01()
{
	m_RandomState = m_RandomState * 1664525u + 1013904223u;
	return static_cast<float>((m_RandomState >> 8) & 0x00FFFFFFu) / 16777215.0f;
}

float EnemyDefeatEffect::RandomRange(float minValue, float maxValue)
{
	return minValue + (maxValue - minValue) * Random01();
}

void EnemyDefeatEffect::Spawn(const XMFLOAT3& position, int face)
{
	const XMFLOAT3 origin = MoveInsideTunnel(position, face);

	// 最初に大きな閃光を出し、その周囲へ小さな破片を飛ばす。
	Particle flash;
	flash.position = origin;
	flash.startSize = 1.8f;
	flash.endSize = 0.35f;
	flash.rotation = RandomRange(0.0f, 360.0f);
	flash.angularVelocity = 180.0f;
	flash.lifetime = 0.18f;
	Emit(flash);

	for (int i = 0; i < PARTICLE_COUNT; ++i)
	{
		const float angle = (2.0f * PI * i / PARTICLE_COUNT) + RandomRange(-0.18f, 0.18f);
		const float speed = RandomRange(2.5f, 5.5f);

		Particle particle;
		particle.position = origin;
		particle.velocity = {
			std::cos(angle) * speed,
			std::sin(angle) * speed,
			RandomRange(-1.4f, 1.4f)
		};
		AimInsideTunnel(particle.velocity, face);
		particle.acceleration = { 0.0f, -0.8f, 0.0f };
		particle.startSize = RandomRange(0.28f, 0.55f);
		particle.endSize = 0.03f;
		particle.rotation = RandomRange(0.0f, 360.0f);
		particle.angularVelocity = RandomRange(-420.0f, 420.0f);
		particle.damping = 1.8f;
		particle.lifetime = RandomRange(0.35f, 0.65f);
		Emit(particle);
	}
}
