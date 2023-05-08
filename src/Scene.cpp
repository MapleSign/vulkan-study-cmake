#include "Scene.h"

#include <string>
#include <filesystem>

#include "GLTF/GLTFHelper.h"

Scene::Scene() :
	activeCamera{ nullptr }, dirLight{ new DirLight() }
{
	auto defaultCameraName = "default";
	addFPSCamera(defaultCameraName);
	setCamera(defaultCameraName);

	dirLight->direction = glm::vec3(-0.2f, -1.0f, -0.3f);
	dirLight->ambient = { 0.2f, 0.2f, 0.2f };
	dirLight->diffuse = { 0.5f, 0.5f, 0.5f };
	dirLight->specular = { 1.0f, 1.0f, 1.0f };
}

Scene::~Scene()
{
}

std::vector<Model*> Scene::loadGLTFFile(const char* filename)
{
	std::vector<Model*> models;

	tinygltf::Model tModel;
	tinygltf::TinyGLTF tContext;
	std::string warn, error;

	if (!tContext.LoadASCIIFromFile(&tModel, &error, &warn, filename))
		assert(!"Error while loading scene");

	std::filesystem::path filepath{ filename };

	for (auto& tMesh : tModel.meshes) {
		std::vector<Mesh> meshes{};
		meshes.reserve(tMesh.primitives.size());
		for (auto& tPrim : tMesh.primitives) {
			std::vector<uint32_t> indices{};
			std::vector<Vertex> vertices{};
			std::vector<Texture> textures{};
			GltfMaterial mat{};

			/* Indices */
			if (tPrim.indices >= 0) {
				getAccessorDataScalar(tModel, tModel.accessors[tPrim.indices], indices);
			}
			else {
				const auto& tAcces = tModel.accessors[tPrim.attributes.find("POSITION")->second];
				for (auto i = 0; i < tAcces.count; i++)
					indices.push_back(i);
			}

			/* Vertices Begin */

			/* Positions */
			std::vector<glm::vec3> positions;
			if (!getAttribute(tModel, tPrim, positions, "POSITION")) {
				throw std::runtime_error(std::string() + filename + ": a GLTF primitive with NO POSITION!");
			}
			vertices.resize(positions.size());
			for (size_t i = 0; i < vertices.size(); ++i) {
				vertices[i].pos = positions[i];
			}

			/* Normals */
			std::vector<glm::vec3> normals;
			if (!getAttribute(tModel, tPrim, normals, "NORMAL")) {
				// Need to compute the normals
				createNormals(indices, positions, normals);
			}
			for (size_t i = 0; i < vertices.size(); ++i) {
				vertices[i].normal = normals[i];
			}

			/* TexCoords */
			std::vector<glm::vec2> texCoords{};
			bool texcoordCreated = getAttribute(tModel, tPrim, texCoords, "TEXCOORD_0");
			if (!texcoordCreated) {
				texcoordCreated = getAttribute(tModel, tPrim, texCoords, "TEXCOORD");
			}
			if (!texcoordCreated) {
				texCoords.insert(texCoords.end(), vertices.size(), glm::vec2(0.f));
			}
			for (size_t i = 0; i < vertices.size(); ++i) {
				vertices[i].texCoord = texCoords[i];
			}

			/* Tangent and BiTangent */
			std::vector<glm::vec3> tangents;
			std::vector<glm::vec3> bitangents;

			std::vector<glm::vec4> gltfTangents;
			if (!getAttribute(tModel, tPrim, gltfTangents, "TANGENT")) {
				tangents.resize(vertices.size());
				bitangents.resize(vertices.size());
				createTangents(indices, positions, texCoords, tangents, bitangents);
			}
			else {
				for (size_t i = 0; i < vertices.size(); ++i) {
					auto& gt = gltfTangents[i];
					auto& n = normals[i];
					glm::vec3 t = gt;
					glm::vec3 b = glm::normalize(glm::cross(n, t)) * gt.w;
					tangents.push_back(t);
					bitangents.push_back(b);
				}
			}
			for (size_t i = 0; i < vertices.size(); ++i) {
				vertices[i].tangent = tangents[i];
				vertices[i].bitangent = bitangents[i];
			}
			/* Vertices are filled */

			/* Material and Textures */
			auto& tMat = tModel.materials[tPrim.material];
			importGLTFMaterial(mat, tMat);
			// deal with textures
			auto setTexture = [&](int& textureId)
			{
				if (textureId > -1) {
					auto& tTex = tModel.textures[textureId];
					auto& tImage = tModel.images[tTex.source];
					Texture texture{ TextureType::DIFFUSE, filepath.root_directory().string() + tImage.uri };
					textures.push_back(texture);
					textureId = textures.size() - 1;
				}
			};
			{
				setTexture(mat.pbrBaseColorTexture);
				setTexture(mat.pbrMetallicRoughnessTexture);
				setTexture(mat.normalTexture);
				setTexture(mat.occlusionTexture);
				setTexture(mat.emissiveTexture);
				setTexture(mat.transmissionTexture);
				setTexture(mat.khrDiffuseTexture);
				setTexture(mat.khrSpecularGlossinessTexture);
				setTexture(mat.clearcoatTexture);
				setTexture(mat.clearcoatRoughnessTexture);
			}

			meshes.emplace_back(nullptr, vertices, indices, textures, mat);
		}

		auto model = new Model(std::move(meshes));
		modelMap.emplace(tMesh.name, model);
		models.push_back(model);
	}

	return models;
}

Model* Scene::addModel(const char* modelName, const char* objFilename)
{
	auto model = new Model(objFilename);
	modelMap.emplace(modelName, model);

	return model;
}

Model* Scene::getModel(const char* modelName)
{
	return modelMap[modelName].get();
}

std::unordered_map<std::string, std::unique_ptr<Model>>& Scene::getModelMap()
{
	return modelMap;
}

BaseCamera* Scene::addBaseCamera(const char* cameraName)
{
	auto camera = new BaseCamera();
	cameraMap.emplace(cameraName, camera);
	return camera;
}

FPSCamera* Scene::addFPSCamera(const char* cameraName)
{
	auto camera = new FPSCamera();
	cameraMap.emplace(cameraName, camera);
	return camera;
}

BaseCamera* Scene::getCamera(const char* cameraName)
{
	return cameraMap[cameraName].get();
}

const BaseCamera* Scene::getCamera(const char* cameraName) const
{
	return cameraMap.at(cameraName).get();
}

BaseCamera* Scene::getActiveCamera()
{
	return activeCamera;
}

const BaseCamera* Scene::getActiveCamera() const
{
	return activeCamera;
}

void Scene::setCamera(const char* cameraName)
{
	activeCamera = cameraMap[cameraName].get();
}

std::unordered_map<std::string, std::unique_ptr<BaseCamera>>& Scene::getCameraMap()
{
	return cameraMap;
}

DirLight* Scene::getDirLight()
{
	return dirLight.get();
}

const DirLight* Scene::getDirLight() const
{
	return dirLight.get();
}

PointLight* Scene::addPointLight(const char* name, glm::vec3 pos, glm::vec3 color, float intensity)
{
	auto light = new PointLight();
	light->position = pos;
	light->ambient = color * intensity / 100.f;
	light->diffuse = color * intensity;
	light->specular = color * intensity;

	light->constant = 1.0f;
	light->linear = 0.09f;
	light->quadratic = 0.032f;

	pointLightMap.emplace(name, light);

	return light;
}

PointLight* Scene::getPointLight(const char* name)
{
	return pointLightMap[name].get();
}

std::unordered_map<std::string, std::unique_ptr<PointLight>>& Scene::getPointLightMap()
{
	return pointLightMap;
}

const std::unordered_map<std::string, std::unique_ptr<PointLight>>& Scene::getPointLightMap() const
{
	return pointLightMap;
}
