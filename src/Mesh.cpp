#include "Mesh.h"
#include "Model.h"

Mesh::Mesh(
	Model* parent, 
	const std::vector<Vertex>& vertices, 
	const std::vector<unsigned int>& indices, 
	const std::vector<Texture>& textures, 
	const GltfMaterial& mat
) :
	parent{ parent }, vertices{ vertices }, indices{ indices }, textures{ textures }, mat{ mat }
{
	setupMesh();
}

void Mesh::setupMesh()
{

}
