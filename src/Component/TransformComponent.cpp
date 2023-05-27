#include "TransformComponent.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

TransformComponent::TransformComponent(const glm::vec3& translate, const glm::vec3& rotate, const glm::vec3& scale) :
	translate{ translate }, rotate{ rotate }, scale{ scale }
{
}

glm::mat4 TransformComponent::getTransformMatrix() const
{
	glm::mat4 model{ 1.0f };
	model = glm::translate(model, translate);
	glm::vec3 rotateR = glm::radians(rotate);
	model = model * glm::eulerAngleXYZ(rotateR.x, rotateR.y, rotateR.z);
	model = glm::scale(model, scale);
	return model;
}
