#pragma once

#include "Mesh.h"

class GeometryGenerator
{
public:
	static std::vector<Mesh> ReadFromFile(std::string basePath, std::string filename);

	static Mesh MakeBox();

	static std::tuple<std::vector<Vertex>, std::vector<uint16_t>> MakeBox_TEMP();

	static std::tuple<std::vector<Vertex>, std::vector<uint16_t>> MakeSquare_TEMP();
};
