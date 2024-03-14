#pragma once

#include "Camera.h"

#include <vector>

#include <glm/glm.hpp>

std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);

const int MAX_CSM_LEVEL = 6;

struct DirLight {
    glm::vec3 direction;
    float width;

    glm::vec3 color;
    float intensity;

    glm::mat4 lightSpace[MAX_CSM_LEVEL];

    float csmFarPlanes[MAX_CSM_LEVEL];
    int csmLevel = 3;

    float padding;

    void update(const BaseCamera& camera, float aspect);
};

struct PointLight {
    glm::mat4 lightSpaces[6];

    glm::vec3 position;
    float width;

    glm::vec3 color;
    float intensity;
    
    float constant;
    float linear;
    float quadratic;

    float padding;

    void update();
};

class Light
{
public:
    Light() {};
    ~Light() {};

private:

};