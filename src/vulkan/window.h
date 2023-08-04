#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>

#include <string>

class Window {
public:
	Window(int width, int height, std::string name);
	~Window();

	Window(const Window&)            = delete;
	Window& operator=(const Window&) = delete;

	void CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

	inline bool WasWindowResized() { return m_Resized; }

	inline void ResetWindowResizedFlag() { m_Resized = false; }

	inline bool ShouldClose() { return glfwWindowShouldClose(m_Window); }

	inline VkExtent2D GetExtent() { return {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height)}; }

	inline GLFWwindow* GetGLFWwindow() const { return m_Window; }

private:
	static void ResizeCallback(GLFWwindow* window, int width, int height);

	int m_Width;
	int m_Height;
	std::string m_Name;
	bool m_Resized = false;

	GLFWwindow* m_Window;
};