#pragma once
#include <fstream>
#include <vector>
#include "geometry.h"
#include <iostream>
#include <sstream>
#include "tgaimage.h"

class Model
{
public:
	Model(std::string filename);

	int nfaces();
	std::vector<std::vector<int>> face(int i);
	Vec3f vert(int i);
	Vec2i uv(int i);
	TGAColor diffuse(Vec2i uv);
private:
	std::vector<Vec3f> _vertices{ {0., 0., 0.} };
	std::vector<std::vector<std::vector<int>>> _faces;
	std::vector<Vec2f> _texVertices{ {0., 0.} };

	void load_texture(std::string filename, const char* suffix, TGAImage& img);

	TGAImage _diffMap;
};