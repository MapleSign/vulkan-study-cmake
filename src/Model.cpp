#include <iostream>

#include "Model.h"

Model::Model(const char* objFilename)
{
    loadModel(objFilename);
}

Model::~Model()
{
}

const std::vector<Mesh>& Model::getMeshes() const
{
    return meshes;
}

void Model::loadModel(std::string path)
{
    Assimp::Importer import{};
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    // Traverse every meshes of nodes 
    // and convert it into Mesh object
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }
    // Repeat for every child nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

void Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices{};
    std::vector<unsigned int> indices{};
    std::vector<Texture> textures{};

    // Convert vertices into Vertex, 
    // and store them into the vector
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex{}; // store converted positon, normal and textureCoord
        glm::vec3 vector{}; // a temp vec3 for converting

        // deal with vertices
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.pos = vector;

        // deal with normals
        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.normal = vector;

        // deal with texture coordinates
        if (mesh->mTextureCoords[0]) // Does it have texture coordinates?
        {
            glm::vec2 vec{};
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.texCoord = vec;
        }
        else
            vertex.texCoord = glm::vec2(0.0f, 0.0f);

        // load tangent & bitangent
        vector.x = mesh->mTangents[i].x;
        vector.y = mesh->mTangents[i].y;
        vector.z = mesh->mTangents[i].z;
        vertex.tangent = vector;
        vector.x = mesh->mBitangents[i].x;
        vector.y = mesh->mBitangents[i].y;
        vector.z = mesh->mBitangents[i].z;
        vertex.bitangent = vector;

        vertices.push_back(vertex);
    }

    // Traverse every indices in every faces, 
    // push them into the vector in order
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // See if it has material index, 
    // and if true, load the diffuse and the specular texture
    RenderMaterial mat{};
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        loadMaterialProperties(material, mat);

        std::vector<Texture> diffuseMaps = loadMaterialTextures(material,
            aiTextureType_DIFFUSE, TextureType::DIFFUSE);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<Texture> specularMaps = loadMaterialTextures(material,
            aiTextureType_SPECULAR, TextureType::SPECULAR);
        if (specularMaps.empty())
            textures.emplace_back(TextureType::SPECULAR, "textures/black.jpg");
        else
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        std::vector<Texture> normalMaps = loadMaterialTextures(material,
            aiTextureType_NORMALS, TextureType::NORMAL);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    }

    meshes.emplace_back(this, vertices, indices, textures, mat);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, TextureType typeName)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str{};
        mat->GetTexture(type, i, &str);
        Texture texture{};
        texture.type = typeName;
        texture.path = directory + '/' + str.C_Str();

        textures.push_back(texture);
    }
    return textures;
}

void Model::loadMaterialProperties(aiMaterial* aiMat, RenderMaterial& mat)
{
    aiColor3D color{};
    aiMat->Get(AI_MATKEY_COLOR_AMBIENT, color);
    mat.ambient = { color.r, color.g, color.b };
    aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    mat.diffuse = { color.r, color.g, color.b };
    aiMat->Get(AI_MATKEY_COLOR_SPECULAR, color);
    mat.specular = { color.r, color.g, color.b };
    aiMat->Get(AI_MATKEY_COLOR_TRANSPARENT, color);
    mat.transmittance = { color.r, color.g, color.b };
    aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, color);
    mat.emission = { color.r, color.g, color.b };

    aiMat->Get(AI_MATKEY_SHININESS, mat.shininess);
}
