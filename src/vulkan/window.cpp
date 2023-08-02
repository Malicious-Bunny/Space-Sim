#include "window.h"

#include <stdexcept>

Window::Window(int width, int height, std::string name): m_Width(width), m_Height(height), m_Name(name) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_Window = glfwCreateWindow(m_Width, m_Height, m_Name.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(m_Window, this);
	glfwSetFramebufferSizeCallback(m_Window, ResizeCallback);
}

Window::~Window() {
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void Window::CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
	if(glfwCreateWindowSurface(instance, m_Window, nullptr, surface) != VK_SUCCESS) { throw std::runtime_error("failed to create window surface"); }
}

void Window::ResizeCallback(GLFWwindow* window, int width, int height) {
	auto _window       = (Window*) (glfwGetWindowUserPointer(window));
	_window->m_Resized = true;
	_window->m_Width   = width;
	_window->m_Height  = height;
}