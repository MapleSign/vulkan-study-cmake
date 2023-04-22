#include "GlfwWindow.h"

GlfwWindow::GlfwWindow(VulkanApplication &app, uint32_t width, uint32_t height) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(window, &app);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

GlfwWindow::~GlfwWindow() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

VkSurfaceKHR GlfwWindow::createSurface(VkInstance instance) {
	VkSurfaceKHR surface;

	VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface))

	return surface;
}

VkExtent2D GlfwWindow::getExtent() {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	return VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

GLFWwindow* GlfwWindow::getHandle() const { return window; }

void GlfwWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}