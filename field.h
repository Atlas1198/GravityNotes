#pragma once
#include "renderer.h"
#include "sprite3d.h"

class Field: public Sprite3D
{
private:
	static constexpr int NUM_FIELDS = 15;
	float m_ScrollPos[NUM_FIELDS];

public:
	Field() : Sprite3D() {}
    void Init();
    void Update(float speed);
    void Draw();
    void Finalize();
};
