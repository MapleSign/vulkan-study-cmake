#include "Scene.h"

#include <cmath>
#include <string>
#include <filesystem>

#include <glm/gtx/quaternion.hpp>

#include "GLTF/GLTFHelper.h"

Scene::Scene() :
	activeCamera{ nullptr }
{
	auto defaultCameraName = "default";
	addFPSCamera(defaultCameraName);
	setCamera(defaultCameraName);

	addDirLight("light0", { -0.2f, -1.0f, -0.3f }, { 1.f, 1.f, 1.f }, 1.f);
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

	auto slashpos = std::string(filename).find_last_of('/');
	std::string filepath = std::string(filename).substr(0, slashpos + 1);

	for (auto& tNode : tModel.nodes) {
		if (tNode.mesh < 0) {
			continue;
		}

		auto& tMesh = tModel.meshes[tNode.mesh];
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
				tangents.resize(vertices.size(), glm::vec3(0.f));
				bitangents.resize(vertices.size(), glm::vec3(0.f));
				createTangents(indices, positions, normals, texCoords, tangents, bitangents);
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
					Texture texture{ TextureType::DIFFUSE, filepath + tImage.uri};
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
				setTexture(mat.clearcoatNormalTexture);
			}

			meshes.emplace_back(nullptr, vertices, indices, textures, mat);
		}

		auto model = new Model(tNode.name, std::move(meshes));
		if (!tNode.translation.empty())
			model->transComp.translate = { tNode.translation[0], tNode.translation[1], tNode.translation[2] };
		if (!tNode.scale.empty())
			model->transComp.scale = { tNode.scale[0], tNode.scale[1], tNode.scale[2] };

		if (!tNode.rotation.empty()) {
			glm::quat rotation{ float(tNode.rotation[3]), float(tNode.rotation[0]), float(tNode.rotation[1]), float(tNode.rotation[2]) };
			model->transComp.rotate = glm::vec4(glm::axis(rotation), 0.0f);
			model->transComp.rotate.w = glm::degrees(glm::angle(rotation));

			/*model->transComp.rotate = static_cast<float>(glm::degrees(tNode.rotation[3])) *
				glm::vec3(
					glm::dot(glm::vec3(1.f, 0.f, 0.f), axis),
					glm::dot(glm::vec3(0.f, 1.f, 0.f), axis),
					glm::dot(glm::vec3(0.f, 0.f, 1.f), axis)
				);*/
		}
		models.push_back(model);
	}

	return models;
}

std::vector<Model*> Scene::addModelsFromGltfFile(const char* filename)
{
	auto models = loadGLTFFile(filename);
	for (auto model : models) {
		if (modelMap.find(model->getName()) == modelMap.end())
			modelMap.emplace(model->getName(), model);
		else {
			for (int i = 1;; ++i) {
				std::string name = model->getName() + "_" + std::to_string(i);
				if (modelMap.find(name) == modelMap.end()) {
					model->setName(name);
					modelMap.emplace(name, model);
					break;
				}
			}
		}
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

DirLight* Scene::addDirLight(const char* name, glm::vec3 dir, glm::vec3 color, float intensity)
{
	auto light = new DirLight();
	light->direction = dir;
	light->color = color;
	light->intensity = intensity;
	light->update();

	dirLightMap.emplace(name, light);

	return light;
}

DirLight* Scene::getDirLight(const char* name)
{
	return dirLightMap[name].get();
}

std::unordered_map<std::string, std::unique_ptr<DirLight>>& Scene::getDirLightMap()
{
	return dirLightMap;
}

const std::unordered_map<std::string, std::unique_ptr<DirLight>>& Scene::getDirLightMap() const
{
	return dirLightMap;
}

PointLight* Scene::addPointLight(const char* name, glm::vec3 pos, glm::vec3 color, float intensity)
{
	auto light = new PointLight();
	light->position = pos;
	light->color = color;
	light->intensity = intensity;

	light->constant = 0.0f;
	light->linear = 0.0f;
	light->quadratic = 0.2f;

	light->update();

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
