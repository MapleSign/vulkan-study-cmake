#pragma once

#include <vector>

#include "Vertex.h"
#include "Texture.h"
#include "Mesh.h"
#include "Component/TransformComponent.h"

class Model
{
public:
	Model(const char* objFilename);
	Model(std::vector<Mesh>&& meshes);
	~Model();

	TransformComponent transComp{};

	const std::vector<Mesh>& getMeshes() const;

private:
	std::string directory;

	std::vector<Mesh> meshes;

};

