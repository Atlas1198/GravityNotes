//#include "light_game.h"
//#include "light.h"
//
//AmbientLight m_ambientLight(XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f));
//PointLight* m_pointLight = nullptr;
//
//void GameLight::Init()
//{
//	m_pointLight = new PointLight(
//		TRUE,
//		{ 0.0f, 5.0f, -5.0f, 1.0f },
//		{ 1.0f, 1.0f, 1.0f, 1.0f },
//		20.0f,
//		1.0f
//	);
//
//	m_pointLight->Apply(m_ambientLight);
//}
//
//void GameLight::Finalize()
//{
//	SAFE_DELETE(m_pointLight);
//}
//
//void GameLight::Update()
//{
//	if (m_pointLight) m_pointLight->Apply(m_ambientLight);
//}
//
//void GameLight::Draw()
//{
//	
//}
