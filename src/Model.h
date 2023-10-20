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
	Model(std::string name, std::vector<Mesh>&& meshes);
	~Model();

	TransformComponent transComp{};

	const std::vector<Mesh>& getMeshes() const;
	const std::string getName() const { return name; }
	void setName(std::string name) { this->name = name; }

private:
	std::string name;

	std::vector<Mesh> meshes;
	
};

