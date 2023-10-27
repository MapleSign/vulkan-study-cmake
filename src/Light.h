#pragma once

#include <glm/glm.hpp>

struct DirLight {
    glm::vec3 direction;
    float pad;

    glm::vec3 color;
    float intensity;

    glm::mat4 lightSpace;

    void update();
};

struct PointLight {
    alignas(16) glm::vec3 position;

    alignas(4) glm::vec3 color;
    alignas(4) float intensity;
    
    alignas(4) float constant;
    alignas(4) float linear;
    alignas(8) float quadratic;

    glm::mat4 lightSpace[6];
};

class Light
{
public:
    Light() {};
    ~Light() {};

private:

};