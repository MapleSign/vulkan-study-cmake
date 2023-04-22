#include "Mesh.h"
#include "Model.h"

Mesh::Mesh(Model* parent, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, std::vector<Texture>& textures, const RenderMaterial& mat) :
	parent{ parent }, vertices{ vertices }, indices{ indices }, textures{ textures }, mat{ mat }
{
	setupMesh();
}

void Mesh::setupMesh()
{

}
