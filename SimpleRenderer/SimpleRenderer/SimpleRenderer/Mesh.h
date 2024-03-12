#pragma once
#include "Object.h"

#include <vector>

#include "Vertex.h"
#include "Material.h"

class Mesh : public Object
{
public:
	std::vector<Vertex> Vertices;
	std::vector<uint32_t> Indices;
	Material Material;

public:
	virtual ~Mesh();

private:

};

