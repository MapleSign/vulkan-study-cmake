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
	Model* addModel(const char* modelName, const char* objFilename);
	Model* getModel(const char* modelName);
	std::unordered_map<std::string, std::unique_ptr<Model>>& getModelMap();

	BaseCamera* addBaseCamera(const char* cameraName);
	FPSCamera* addFPSCamera(const char* cameraName);
	VRCamera* addVRCamera(const char* cameraName);
	BaseCamera* getCamera(const char* cameraName);
	const BaseCamera* getCamera(const char* cameraName) const;
	BaseCamera* getActiveCamera();
	const BaseCamera* getActiveCamera() const;
	void setCamera(const char* cameraName);
	std::unordered_map<std::string, std::unique_ptr<BaseCamera>>& getCameraMap();

	DirLight* getDirLight();
	const DirLight* getDirLight() const;

	PointLight* addPointLight(const char* name, glm::vec3 pos = {0.f, 0.f, 10.f}, glm::vec3 color = {1.f, 1.f, 1.f }, float intensity = 80.f);
	PointLight* getPointLight(const char* name);
	std::unordered_map<std::string, std::unique_ptr<PointLight>>& getPointLightMap();
	const std::unordered_map<std::string, std::unique_ptr<PointLight>>& getPointLightMap() const;
private:
	BaseCamera* activeCamera;

	std::unordered_map<std::string, std::unique_ptr<BaseCamera>> cameraMap;
	std::unordered_map<std::string, std::unique_ptr<Model>> modelMap;

	std::unique_ptr<DirLight> dirLight;
	std::unordered_map<std::string, std::unique_ptr<PointLight>> pointLightMap;
};
