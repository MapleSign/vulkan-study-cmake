#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class TransformComponent {
public:
	TransformComponent() = default;
	TransformComponent(const glm::vec3& translate, const glm::vec4& rotate, const glm::vec3& scale);

	glm::vec3 translate{ 0.0f };
	glm::vec4 rotate{ 1.0f, 0.0f, 0.0f, 0.0f }; // axis angle
	glm::vec3 scale{ 1.0f };

	glm::mat4 transform = glm::identity<glm::mat4>();

	glm::mat4 getTransformMatrix() const;
};