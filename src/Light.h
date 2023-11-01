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
    glm::mat4 lightSpaces[6];

    glm::vec3 position;
    float padding0;

    glm::vec3 color;
    float intensity;
    
    float constant;
    float linear;
    float quadratic;

    float padding1;

    void update();
};

class Light
{
public:
    Light() {};
    ~Light() {};

private:

};