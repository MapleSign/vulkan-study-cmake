#include <iostream>

#include "Model.h"

Model::Model(const char* objFilename)
{
}

Model::Model(std::string name, std::vector<Mesh>&& meshes) :
    name{ name }, meshes{ std::move(meshes) }
{
    for (auto& m : this->meshes) {
        m.parent = this;
    }
}

Model::~Model()
{
}

const std::vector<Mesh>& Model::getMeshes() const
{
    return meshes;
}
