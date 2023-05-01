#include "Scene.h"

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
