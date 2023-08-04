#pragma once
#include "camera.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#include "utilities.h"

class Input {
public:
	static void Instantiate(WindowInfo windowInfo);
	static void ProcessInput();
	static void SetCallbacks();
	static void GetInput(Camera& camera);
	static float mouseX, mouseY;
	static float exposure;
	static float temperature;
	static bool showShadowMap;
	static bool captureMouse;

private:
	static WindowInfo windowInfo;
};