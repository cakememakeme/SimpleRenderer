#pragma once

#include <directxtk/SimpleMath.h>

class Vertex
{
public:
	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector3 Color;
	DirectX::SimpleMath::Vector2 TexCoord;
	DirectX::SimpleMath::Vector3 Normal;
};