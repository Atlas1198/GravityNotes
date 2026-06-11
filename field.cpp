#include "shadermanager.h"
#include "model.h"
#include "field.h"

void Field::Init() {
	m_Position = { 0.0f,0.0f,10.0f };
	m_Rotation = { 0.0f,0.0f,0.0f };
}
void Field::Update() {

}
void Field::Draw() {
	if (m_Model) {
		ModelDraw(
			m_Model,
			m_Position,
			m_Rotation,
			m_Scale,
			m_Color,
			false,
			S_UNLIT
		);
	}
}
void Field::Finalize() {
}