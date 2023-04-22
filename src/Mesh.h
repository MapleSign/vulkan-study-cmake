#pragma once

#include "Vulkan/VulkanCommon.h"
#include "Vulkan/VulkanResource.h"

#include "Vertex.h"
#include "Texture.h"
#include "Component/TransformComponent.h"

class Model;

class Mesh
{
public:
    Model* parent;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    RenderMaterial mat;

    TransformComponent transComp{};

    Mesh(Model* parent, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, 
        std::vector<Texture>& textures, const RenderMaterial& mat);
private:
    void setupMesh();
};