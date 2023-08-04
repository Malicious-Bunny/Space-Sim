#pragma once

#include "vulkan/descriptors.h"
#include "vulkan/image.h"
#include "vulkan/model.h"
#include "vulkan/sampler.h"
#include "vulkan/uniform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vulkan/vulkan.h>

struct ObjectInfo {
	Device* device;
	Sampler* sampler;
	DescriptorPool* descriptorPool;
};

struct Transform {
	glm::dvec3 translation {0.0, 0.0, 0.0};
	glm::dvec3 scale {1.0, 1.0, 1.0};
	glm::dvec3 rotation {};

	glm::mat4 mat4(const glm::dvec3& cameraTranslation) {
		auto transform = glm::translate(glm::dmat4 {1.0f}, (translation - cameraTranslation));

		transform = glm::rotate(transform, glm::radians(rotation.y), {0.0f, -1.0f, 0.0f});
		transform = glm::rotate(transform, glm::radians(rotation.x), {1.0f, 0.0f, 0.0f});
		transform = glm::rotate(transform, glm::radians(rotation.z), {0.0f, 0.0f, 1.0f});
		transform = glm::scale(transform, scale);
		return transform;
	}
};

struct Properties {};

class Object {
public:
	Object(const ObjectInfo& objInfo, const Transform& objTransform, const std::string& modelFilepath, const std::string& albedoMap, const std::string& metallicMap, const std::string& normalMap,
	       const std::string& roughnessMap);
	~Object() = default;

	Properties& GetObjectProperties() { return m_Properties; }

	Transform& GetObjectTransform() { return m_Transform; }

	uint32_t GetObjectID() { return m_ID; }

	void Draw(VkPipelineLayout layout, VkCommandBuffer commandBuffer);

private:
	Properties m_Properties;
	Transform m_Transform;
	uint32_t m_ID;

private:
	Device& m_Device;
	std::unique_ptr<Model> m_Model;
	std::unique_ptr<Uniform> m_Uniform;
	std::unique_ptr<Image> m_Albedo;
	std::unique_ptr<Image> m_Normal;
	std::unique_ptr<Image> m_Metallic;
	std::unique_ptr<Image> m_Roughness;
};