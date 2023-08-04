#pragma once

#include "../vulkan/model.h"
#include "cubemap.h"
#include "device.h"

#include <memory>

class Skybox {
public:
	Skybox(Device& device, const std::string& folderPath);
	~Skybox() = default;

	inline Cubemap& GetCubemap() { return m_Cubemap; }

	inline Model* GetSkyboxModel() { return m_SkyboxModel.get(); }

private:
	Device& m_Device;
	std::unique_ptr<Model> m_SkyboxModel;
	glm::mat4 m_ModelTransform;

	Cubemap m_Cubemap {m_Device};
};