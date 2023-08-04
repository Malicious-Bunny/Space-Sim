#pragma once

#include "buffer.h"
#include "descriptors.h"
#include "device.h"

#include <memory>
#include <vulkan/vulkan_core.h>

struct Binding {
	VkDescriptorType bindingType;
	VkShaderStageFlagBits bindingStage;
	uint32_t bufferSize;
	VkSampler sampler;
	VkImageView imageView;
	VkImageLayout imageLayout;
};

class Uniform {
public:
	Uniform(Device& device, const std::vector<Binding>& bindings, DescriptorPool& pool);
	~Uniform();

	inline std::shared_ptr<Buffer> GetUboBuffer(const int& index) { return m_UboBuffers[index]; }

	inline std::shared_ptr<DescriptorSetLayout> GetDescriptorSetLayout() { return m_DescriptorSetLayout; }

	inline VkDescriptorSet& GetDescriptorSet() { return m_DescriptorSet; }

private:
	Device& m_Device;

	std::vector<std::shared_ptr<Buffer>> m_UboBuffers;
	std::shared_ptr<DescriptorSetLayout> m_DescriptorSetLayout;
	VkDescriptorSet m_DescriptorSet;

	std::vector<VkDescriptorImageInfo> m_ImageDescriptors;
	std::vector<VkDescriptorBufferInfo> m_BufferDescriptors;

	uint32_t m_BufferCount = 0;
};