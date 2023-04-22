#pragma once

#include <glm/glm.hpp>

struct DirLight {
    alignas(16) glm::vec3 direction;

    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec3 specular;
};

struct PointLight {
    alignas(16) glm::vec3 position;
    
    alignas(4) float constant;
    alignas(4) float linear;
    alignas(4) float quadratic;
    
    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec3 specular;
};

class Light
{
public:
    Light() {};
    ~Light() {};

private:

};