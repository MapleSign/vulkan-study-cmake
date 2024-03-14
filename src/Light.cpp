#include "Light.h"

#include <limits>

#include <glm/gtc/matrix_transform.hpp>

void DirLight::update(const BaseCamera& camera, float aspect)
{
    float range = (camera.zFar - camera.zNear);
    for (int i = 0; i < csmLevel; ++i) {
        csmFarPlanes[i] = range * (i + 1) / csmLevel + camera.zNear;
    }

    auto view = camera.calcLookAt();

    float near = camera.zNear;
    for (int i = 0; i < csmLevel; ++i) {
        auto proj = glm::perspective(
            glm::radians(camera.zoom),
            aspect,
            near,
            csmFarPlanes[i]
        );
        proj[1][1] *= -1;
        near = csmFarPlanes[i];

        auto corners = getFrustumCornersWorldSpace(proj, view);
        glm::vec3 center = glm::vec3(0, 0, 0);
        for (const auto& v : corners)
        {
            center += glm::vec3(v);
        }
        center /= corners.size();

        const auto lightView = glm::lookAt(
            center,
            center + glm::normalize(direction) * 1.f,
            glm::vec3(0.0, 1.0f, 0.0f)
        );

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();
        for (const auto& v : corners)
        {
            const auto trf = lightView * v;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        // Tune this parameter according to the scene
        constexpr float zMult = 10.f;
        if (minZ < 0)
        {
            minZ *= zMult;
        }
        else
        {
            minZ /= zMult;
        }
        if (maxZ < 0)
        {
            maxZ /= zMult;
        }
        else
        {
            maxZ *= zMult;
        }

        glm::mat4 lightProj = glm::orthoRH_ZO(minX, maxX, maxY, minY, minZ, maxZ);

        lightSpace[i] = lightProj * lightView;
    }
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

std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
    const auto inv = glm::inverse(proj * view);

    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const glm::vec4 pt =
                    inv * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        (float)z,
                        1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }

    return frustumCorners;
}
