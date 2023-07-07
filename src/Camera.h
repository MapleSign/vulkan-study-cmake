#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

enum class CameraDirection { FORWARD, BACK, LEFT, RIGHT, HEADUP, DOWN };

class CameraConstVariable {
public:
	static const glm::vec3 POSITION;
	static const glm::vec3 FRONT;
	static const glm::vec3 UP;

	static const float YAW;
	static const float PITCH;
	static const float ROLL;
	static const float SPEED;
	static const float SENSITIVITY;
	static const float ZOOM;
};

class BaseCamera {
public:
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;

	float yaw;
	float pitch;
	float roll;

	float speed;
	float sensitivity;
	float zoom;

	BaseCamera(glm::vec3 position = CameraConstVariable::POSITION, glm::vec3 up = CameraConstVariable::UP, 
		float yaw = CameraConstVariable::YAW, float pitch = CameraConstVariable::PITCH, float roll = CameraConstVariable::ROLL);
	void setOtherArgument(float speed, float sensitivity, float zoom);

	virtual void move(CameraDirection direction, float deltaTime);
	virtual void rotate(float dyaw, float dpitch, float droll);

	glm::mat4 calcLookAt(void) const;
};

class FPSCamera : public BaseCamera {
public:
	FPSCamera(glm::vec3 position = CameraConstVariable::POSITION, glm::vec3 up = CameraConstVariable::UP, 
		float yaw = CameraConstVariable::YAW, float pitch = CameraConstVariable::PITCH);

	void move(CameraDirection direction, float deltaTime) override;
	void rotate(float dyaw, float dpitch);
};

class VRCamera : public FPSCamera {
public:
	float eyeDistant;

	VRCamera(float eyeDistant = 0.5f, glm::vec3 position = CameraConstVariable::POSITION, glm::vec3 up = CameraConstVariable::UP,
		float yaw = CameraConstVariable::YAW, float pitch = CameraConstVariable::PITCH);

	glm::mat4 calcLookAtLeft() const;
	glm::mat4 calcLookAtRight() const;
};
