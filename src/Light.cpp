#include "Light.h"

#include <glm/gtc/matrix_transform.hpp>

void DirLight::update()
{
    auto lightProj = glm::ortho(-20.f, 20.f, -20.f, 20.f, 0.1f, 60.f);
    lightProj[1][1] *= -1;
    auto lightView = glm::lookAt(direction * -50.f, glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
    lightSpace = lightProj * lightView;
}

void PointLight::update()
{
    float aspect = 1.0;
    float near = 0.0f;
    float far = 25.0f;
    glm::mat4 lightProj = glm::perspective(glm::radians(90.0f), aspect, near, far);

    lightSpaces[0] = lightProj * glm::lookAt(position, position + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
    lightSpaces[1] = lightProj * glm::lookAt(position, position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
    lightSpaces[2] = lightProj * glm::lookAt(position, position + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
    lightSpaces[3] = lightProj * glm::lookAt(position, position + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
    lightSpaces[4] = lightProj * glm::lookAt(position, position + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
    lightSpaces[5] = lightProj * glm::lookAt(position, position + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));
}
