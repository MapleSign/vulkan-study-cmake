#pragma once

#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	void processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, TextureType typeName);
	void loadMaterialProperties(aiMaterial* aiMat, GltfMaterial& mat);
};

