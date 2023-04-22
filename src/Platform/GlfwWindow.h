#pragma once

#include <Volk/volk.h>
#include <GLFW/glfw3.h>

#include "../Vulkan/VulkanCommon.h"
#include "../Vulkan/VulkanApplication.h"

class GlfwWindow
{
public:
	GlfwWindow(VulkanApplication& app, uint32_t width = 800, uint32_t height = 600);

	~GlfwWindow();

	VkSurfaceKHR createSurface(VkInstance instance);

	VkExtent2D getExtent();

	GLFWwindow* getHandle() const;

private:
	GLFWwindow* window;

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};