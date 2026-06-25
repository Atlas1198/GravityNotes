#include "light_game.h"

AmbientLight GameLight::m_ambientLight(XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
PointLight* GameLight::m_pointLight = nullptr;

void GameLight::Init()
{
	m_pointLight = new PointLight(
		TRUE,
		{ 0.0f, 5.0f, -5.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		20.0f,
		1.0f
	);

	m_pointLight->Apply(m_ambientLight);
}

void GameLight::Finalize()
{
	SAFE_DELETE(m_pointLight);
}

void GameLight::Update()
{
	//if (m_pointLight) m_pointLight->Apply(m_ambientLight);
}

void GameLight::Draw()
{
	
}
