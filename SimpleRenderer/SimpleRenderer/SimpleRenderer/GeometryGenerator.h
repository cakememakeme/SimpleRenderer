#pragma once

#include "Mesh.h"

class GeometryGenerator
{
public:
	static std::vector<Mesh> ReadFromFile(std::string basePath, std::string filename);

	static Mesh MakeBox();

	static Mesh MakePlane();

	static Mesh MakeSphere(const float radius, const int numSlices, const int numStacks);

	static std::tuple<std::vector<Vertex>, std::vector<uint16_t>> MakeBox_TEMP();

	static std::tuple<std::vector<Vertex>, std::vector<uint16_t>> MakeSquare_TEMP();
};
