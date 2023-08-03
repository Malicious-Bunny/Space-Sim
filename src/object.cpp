#include "object.h"

#include <memory>

Object::Object(const ObjectInfo& objInfo, const Transform& objTransform, const std::string& modelFilepath, const std::string& albedoMap, const std::string& normalMap, const std::string& metallicMap,
               const std::string& roughnessMap)
: m_Device(*objInfo.device), m_Transform(objTransform) {
	m_Model = Model::CreateModelFromFile(*objInfo.device, modelFilepath);

	m_Albedo    = std::make_unique<Image>(m_Device, albedoMap);
	m_Normal    = std::make_unique<Image>(m_Device, normalMap);
	m_Metallic  = std::make_unique<Image>(m_Device, metallicMap);
	m_Roughness = std::make_unique<Image>(m_Device, roughnessMap);

	static uint32_t IDTotal = 0;

	m_ID = IDTotal++;

	std::vector<Binding> bindings;
	bindings.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, objInfo.sampler->GetSampler(), m_Albedo->GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
	bindings.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, objInfo.sampler->GetSampler(), m_Normal->GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
	bindings.push_back(
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, objInfo.sampler->GetSampler(), m_Metallic->GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
	bindings.push_back(
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, objInfo.sampler->GetSampler(), m_Roughness->GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
	m_Uniform = std::make_unique<Uniform>(m_Device, bindings, *objInfo.descriptorPool);
}

void Object::Draw(VkPipelineLayout layout, VkCommandBuffer commandBuffer, int firstSet) {
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, firstSet, 1, &m_Uniform->GetDescriptorSet(), 0, nullptr);

	m_Model->Bind(commandBuffer);
	m_Model->Draw(commandBuffer);
}