#include "camera.h"

#include "glm/trigonometric.hpp"
#include "input.h"

#include <iostream>

void printMat(const glm::mat4& mat) {
	std::cout << "------------------------------------------------" << std::endl;
	std::cout << " | " << mat[0][0] << ", " << mat[1][0] << ", " << mat[2][0] << ", " << mat[3][0] << " | " << std::endl;
	std::cout << " | " << mat[0][1] << ", " << mat[1][1] << ", " << mat[2][1] << ", " << mat[3][1] << " | " << std::endl;
	std::cout << " | " << mat[0][2] << ", " << mat[1][2] << ", " << mat[2][2] << ", " << mat[3][2] << " | " << std::endl;
	std::cout << " | " << mat[0][3] << ", " << mat[1][3] << ", " << mat[2][3] << ", " << mat[3][3] << " | " << std::endl;
	std::cout << "------------------------------------------------" << std::endl;
}

void Camera::SetPerspective(const float& fov, const float& aspectRatio, const float& near, const float& far) {
	m_Projection = glm::perspective(glm::radians(fov), aspectRatio, near, far);
}

void Camera::MoveCamera(const float& x, const float& y) {
	static float lastMousePosX = x, lastMousePosY = y;
	static float sensitivity = 0.05;

	if(!Input::captureMouse) {
		if(x != lastMousePosX) {
			glm::quat quat;
			quat.w        = glm::cos(glm::radians((x - lastMousePosX) * sensitivity));
			quat.x        = (glm::sin(glm::radians((x - lastMousePosX) * sensitivity)) * m_CameraUp).x;
			quat.y        = (glm::sin(glm::radians((x - lastMousePosX) * sensitivity)) * m_CameraUp).y;
			quat.z        = (glm::sin(glm::radians((x - lastMousePosX) * sensitivity)) * m_CameraUp).z;
			m_Orientation = m_Orientation * quat;
		}
		if(y != lastMousePosY) {
			glm::quat quat;
			quat.w        = glm::cos(glm::radians((y - lastMousePosY) * -sensitivity));
			quat.x        = (glm::sin(glm::radians((y - lastMousePosY) * -sensitivity)) * m_CameraRight).x;
			quat.y        = (glm::sin(glm::radians((y - lastMousePosY) * -sensitivity)) * m_CameraRight).y;
			quat.z        = (glm::sin(glm::radians((y - lastMousePosY) * -sensitivity)) * m_CameraRight).z;
			m_Orientation = m_Orientation * quat;
		}
	}
	lastMousePosX = x;
	lastMousePosY = y;

	m_View = glm::toMat4(m_Orientation);

	m_CameraFront = glm::vec3(m_View[0][2], m_View[1][2], m_View[2][2]);
	m_CameraRight = glm::vec3(m_View[0][0], m_View[1][0], m_View[2][0]);
	m_CameraUp    = glm::vec3(m_View[0][1], m_View[1][1], m_View[2][1]);
}