#pragma once

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"

class Camera {
public:
	void SetPerspective(const float& fov, const float& aspectRatio, const float& near, const float& far);
	void MoveCamera(const float& x, const float& y);

	inline glm::mat4& GetView() { return m_View; }

	inline glm::mat4& GetProj() { return m_Projection; }

	glm::dvec3 m_Translation = {0.0, 0.0, 0.0};
	glm::vec3 m_CameraFront  = {0.0, 0.0, 1.0};
	glm::vec3 m_CameraRight  = {1.0, 0.0, 0.0};
	glm::vec3 m_CameraUp     = {0.0, -1.0, 0.0};

	glm::quat m_Orientation {1.0, 0.0, 0.0, 0.0};

private:
	glm::mat4 m_View;
	glm::mat4 m_Projection;
};