#pragma once
#include <string>

enum class TextureType
{
	DIFFUSE = 0,
	SPECULAR,
	NORMAL
};

struct Texture
{
	TextureType type;
	std::string path;
};