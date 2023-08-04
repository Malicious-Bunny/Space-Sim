#include "input.h"

#include <iostream>

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if(key == GLFW_KEY_M && action == GLFW_PRESS) Input::showShadowMap = !Input::showShadowMap;
	if(key == GLFW_KEY_I && action == GLFW_PRESS) {
		static bool enabled = true;
		glfwSetInputMode(window, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
		enabled             = !enabled;
		Input::captureMouse = !enabled;
	}
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	Input::mouseX = xpos;
	Input::mouseY = ypos;
	glfwSetKeyCallback(window, key_callback);
}

void Input::Instantiate(WindowInfo windowInfo) {
	Input::windowInfo = windowInfo;
	Input::mouseX     = windowInfo.windowSize.x / 2;
	Input::mouseY     = windowInfo.windowSize.y / 2;
}

void Input::ProcessInput() {
	if(glfwGetKey(windowInfo.windowPtr, GLFW_KEY_ESCAPE) == GLFW_PRESS) { glfwSetWindowShouldClose(windowInfo.windowPtr, true); }
}

void Input::GetInput(Camera& camera) {
	static float sensitivity = 1.0;
	if(glfwGetKey(windowInfo.windowPtr, GLFW_KEY_W) == GLFW_PRESS) { camera.m_Translation -= 0.1f * camera.m_CameraFront * sensitivity; }
	if(glfwGetKey(windowInfo.windowPtr, GLFW_KEY_S) == GLFW_PRESS) { camera.m_Translation += 0.1f * camera.m_CameraFront * sensitivity; }
	if(glfwGetKey(windowInfo.windowPtr, GLFW_KEY_A) == GLFW_PRESS) { camera.m_Translation -= 0.1f * camera.m_CameraRight * sensitivity; }
	if(glfwGetKey(windowInfo.windowPtr, GLFW_KEY_D) == GLFW_PRESS) { camera.m_Translation += 0.1f * camera.m_CameraRight * sensitivity; }
	if(glfwGetKey(windowInfo.windowPtr, GLFW_KEY_SPACE) == GLFW_PRESS) { camera.m_Translation -= 0.1f * camera.m_CameraUp * sensitivity; }
	if(glfwGetKey(windowInfo.windowPtr, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) { camera.m_Translation += 0.1f * camera.m_CameraUp * sensitivity; }

	if(glfwGetKey(windowInfo.windowPtr, GLFW_KEY_Q) == GLFW_PRESS) {
		glm::quat quat;
		quat.w               = glm::cos(glm::radians(0.5f));
		quat.x               = (glm::sin(glm::radians(0.5f)) * camera.m_CameraFront).x;
		quat.y               = (glm::sin(glm::radians(0.5f)) * camera.m_CameraFront).y;
		quat.z               = (glm::sin(glm::radians(0.5f)) * camera.m_CameraFront).z;
		camera.m_Orientation = camera.m_Orientation * quat;
	}
	if(glfwGetKey(windowInfo.windowPtr, GLFW_KEY_E) == GLFW_PRESS) {
		glm::quat quat;
		quat.w               = glm::cos(glm::radians(-0.5f));
		quat.x               = (glm::sin(glm::radians(-0.5f)) * camera.m_CameraFront).x;
		quat.y               = (glm::sin(glm::radians(-0.5f)) * camera.m_CameraFront).y;
		quat.z               = (glm::sin(glm::radians(-0.5f)) * camera.m_CameraFront).z;
		camera.m_Orientation = camera.m_Orientation * quat;
	}

	if(glfwGetKey(windowInfo.windowPtr, GLFW_KEY_O) == GLFW_PRESS) {
		exposure -= 0.01;
		std::cout << Input::exposure << std::endl;
	}
	if(glfwGetKey(windowInfo.windowPtr, GLFW_KEY_P) == GLFW_PRESS) {
		exposure += 0.01;
		std::cout << Input::exposure << std::endl;
	}

	if(glfwGetKey(windowInfo.windowPtr, GLFW_KEY_T) == GLFW_PRESS) {
		temperature += 50.0;
		std::cout << Input::temperature << std::endl;
	}
	if(glfwGetKey(windowInfo.windowPtr, GLFW_KEY_R) == GLFW_PRESS) {
		temperature -= 50.0;
		std::cout << Input::temperature << std::endl;
	}
}

void Input::SetCallbacks() { glfwSetCursorPosCallback(windowInfo.windowPtr, cursor_position_callback); }

WindowInfo Input::windowInfo;
float Input::mouseX       = 0;
float Input::mouseY       = 0;
float Input::exposure     = 3.0;
float Input::temperature  = 5778.0f;
bool Input::showShadowMap = false;
bool Input::captureMouse  = false;