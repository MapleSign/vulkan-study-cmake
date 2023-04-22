#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class TransformComponent {
public:
	TransformComponent() = default;
	TransformComponent(const glm::vec3& translate, const glm::vec3& rotate, const glm::vec3& scale);

	glm::vec3 translate{ 0.0f };
	glm::vec3 rotate{ 0.0f };
	glm::vec3 scale{ 1.0f };

	glm::mat4 getTransformMatrix() const;
};