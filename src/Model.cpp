#include <iostream>

#include "Model.h"

Model::Model(const char* objFilename)
{
}

Model::Model(std::vector<Mesh>&& meshes) :
    meshes{ std::move(meshes) }
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
