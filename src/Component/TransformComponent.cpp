#include "TransformComponent.h"

TransformComponent::TransformComponent(const glm::vec3& translate, const glm::vec4& rotate, const glm::vec3& scale) :
	translate{ translate }, rotate{ rotate }, scale{ scale }
{
}

glm::mat4 TransformComponent::getTransformMatrix() const
{
	glm::mat4 model{ 1.0f };
	model = glm::translate(model, translate);
	auto rotateR = glm::angleAxis(glm::radians(rotate.w), glm::vec3(rotate));
	model = model * glm::mat4_cast(rotateR);
	model = glm::scale(model, scale);
	return model * transform;
}
