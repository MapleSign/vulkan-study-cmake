#include "Light.h"

#include <glm/gtc/matrix_transform.hpp>

void DirLight::update()
{
    auto lightProj = glm::ortho(-20.f, 20.f, -20.f, 20.f, 0.1f, 60.f);
    lightProj[1][1] *= -1;
    auto lightView = glm::lookAt(direction * -50.f, glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
    lightSpace = lightProj * lightView;
}
