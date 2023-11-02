#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include <tiny_gltf.h>

#include "Camera.h"
#include "Model.h"
#include "Light.h"

class Scene
{
public:
	Scene();
	~Scene();

	std::vector<Model*> loadGLTFFile(const char* filename);
	std::vector<Model*> addModelsFromGltfFile(const char* filename);
	Model* addModel(const char* modelName, const char* objFilename);
	Model* getModel(const char* modelName);
	std::unordered_map<std::string, std::unique_ptr<Model>>& getModelMap();

	BaseCamera* addBaseCamera(const char* cameraName);
	FPSCamera* addFPSCamera(const char* cameraName);
	BaseCamera* getCamera(const char* cameraName);
	const BaseCamera* getCamera(const char* cameraName) const;
	BaseCamera* getActiveCamera();
	const BaseCamera* getActiveCamera() const;
	void setCamera(const char* cameraName);
	std::unordered_map<std::string, std::unique_ptr<BaseCamera>>& getCameraMap();

	DirLight* addDirLight(const char* name, glm::vec3 dir = {0.f, 0.5f, -0.5f}, glm::vec3 color = { 1.f, 1.f, 1.f }, float intensity = 80.f);
	DirLight* getDirLight(const char* name);
	void removeDirLight(const char* name);
	std::unordered_map<std::string, std::unique_ptr<DirLight>>& getDirLightMap();
	const std::unordered_map<std::string, std::unique_ptr<DirLight>>& getDirLightMap() const;

	PointLight* addPointLight(const char* name, glm::vec3 pos = {0.f, 0.f, 10.f}, glm::vec3 color = {1.f, 1.f, 1.f }, float intensity = 80.f);
	PointLight* getPointLight(const char* name);
	void removePointLight(const char* name);
	std::unordered_map<std::string, std::unique_ptr<PointLight>>& getPointLightMap();
	const std::unordered_map<std::string, std::unique_ptr<PointLight>>& getPointLightMap() const;
private:
	BaseCamera* activeCamera;

	std::unordered_map<std::string, std::unique_ptr<BaseCamera>> cameraMap;
	std::unordered_map<std::string, std::unique_ptr<Model>> modelMap;

	std::unordered_map<std::string, std::unique_ptr<DirLight>> dirLightMap;
	std::unordered_map<std::string, std::unique_ptr<PointLight>> pointLightMap;
};
