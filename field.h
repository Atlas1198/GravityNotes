#pragma once
#include "renderer.h"
#include "sprite3d.h"

class Field: public Sprite3D
{
public:
	Field() : Sprite3D() {}
    void Init();
    void Update();
    void Draw();
    void Finalize();
};
