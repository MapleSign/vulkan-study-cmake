#include "TransformComponent.h"

TransformComponent::TransformComponent(const glm::vec3& translate, const glm::vec3& rotate, const glm::vec3& scale) :
	translate{ translate }, rotate{ rotate }, scale{ scale }
{
}

glm::mat4 TransformComponent::getTransformMatrix() const
{
	glm::mat4 model{ 1.0f };
	model = glm::translate(model, translate);
	model = glm::rotate(model, glm::radians(rotate.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(rotate.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(rotate.z), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, scale);
	return model;
}
