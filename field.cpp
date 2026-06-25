#include "field.h"
#include "shadermanager.h"
#include "define.h"

using namespace DirectX;

void Field::Init() {
	m_Scale = { 5.0f,5.0f,5.0f };
	m_Model = ModelLoad("asset/model/field_normal.fbx");
	m_ShaderType = S_LAMBERT;
}

void Field::Update(){

}

void Field::Draw() {
	Sprite3D::Draw();
}

void Field::Finalize() {
}