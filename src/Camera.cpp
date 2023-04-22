#include <cmath>

#include "Camera.h"

const glm::vec3 CameraConstVariable::POSITION = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 CameraConstVariable::FRONT = glm::vec3(0.0f, 0.0f, -1.0f);
const glm::vec3 CameraConstVariable::UP = glm::vec3(0.0f, 1.0f, 0.0f);

const float CameraConstVariable::YAW = -90.0f;
const float CameraConstVariable::PITCH = 0.0f;
const float CameraConstVariable::ROLL = 0.0f;
const float CameraConstVariable::SPEED = 2.5f;
const float CameraConstVariable::SENSITIVITY = 0.1f;
const float CameraConstVariable::ZOOM = 45.0f;

BaseCamera::BaseCamera(glm::vec3 position, glm::vec3 up, float yaw, float pitch, float roll) :
	position{position}, up{up}, yaw{yaw}, pitch{pitch}, roll{roll},
	front{ CameraConstVariable::FRONT }, speed{ CameraConstVariable::SPEED }, sensitivity{ CameraConstVariable::SENSITIVITY }, zoom{ CameraConstVariable::ZOOM }
{
}

void BaseCamera::setOtherArgument(float speed, float sensitivity, float zoom)
{
	speed = speed;
	sensitivity = sensitivity;
	zoom = zoom;
}

void BaseCamera::move(CameraDirection direction, float deltaTime)
{
	/*TimeTool::updateAllTime();*/
	switch (direction) {
	case CameraDirection::FORWARD: position += front * (speed * deltaTime); break;
	case CameraDirection::BACK: position -= front * (speed * deltaTime); break;
	case CameraDirection::LEFT: position -= glm::normalize(glm::cross(front, up)) * (speed * deltaTime); break;
	case CameraDirection::RIGHT: position += glm::normalize(glm::cross(front, up)) * (speed * deltaTime); break;
	}
}

void BaseCamera::rotate(float dyaw, float dpitch, float droll)
{

}

glm::mat4 BaseCamera::calcLookAt(void)
{
	return glm::lookAt(position, position + front, up);
}

FPSCamera::FPSCamera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : //it has default arguments
	BaseCamera(position, up, yaw, pitch)
{
}

void FPSCamera::move(CameraDirection direction, float deltaTime)
{
	//TimeTool::updateTime();
	switch (direction) {
	case CameraDirection::FORWARD: position += glm::vec3(front.x, 0.0f, front.z) * (speed * deltaTime); break;
	case CameraDirection::BACK: position -= glm::vec3(front.x, 0.0f, front.z) * (speed * deltaTime); break;
	case CameraDirection::LEFT: position -= glm::normalize(glm::cross(front, up)) * (speed * deltaTime); break;
	case CameraDirection::RIGHT: position += glm::normalize(glm::cross(front, up)) * (speed * deltaTime); break;
	case CameraDirection::HEADUP: position += up * (speed * deltaTime); break;
	case CameraDirection::DOWN: position -= up * (speed * deltaTime); break;
	}
}

void FPSCamera::rotate(float dyaw, float dpitch)
{
	yaw += dyaw;
	pitch += dpitch;
	if (pitch > 89.0f) {
		pitch = 89.0f;
	}
	else if (pitch < -89.0f) {
		pitch = -89.0f;
	}

	glm::vec3 newFront;
	newFront.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
	newFront.y = sin(glm::radians(pitch));
	newFront.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
	front = glm::normalize(newFront);
}
