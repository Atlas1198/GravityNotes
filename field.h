#pragma once
#include "renderer.h"
#include "sprite3d.h"

class Field: public Sprite3D
{
private:
	Sprite3D* m_Floor;
	Sprite3D* m_WallLeft;
	Sprite3D* m_WallRight;
	Sprite3D* m_Ceiling;

public:
	Field()
		: Sprite3D(), m_Floor(nullptr), m_WallLeft(nullptr), m_WallRight(nullptr), m_Ceiling(nullptr)
	{}
    void Init();
    void Update();
    void Draw();
    void Finalize();
};
