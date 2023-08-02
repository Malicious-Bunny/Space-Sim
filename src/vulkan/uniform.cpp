#include "uniform.h"

#include <stdexcept>
#include <vulkan/vulkan_core.h>

Uniform::Uniform(Device& device, const std::vector<Binding>& bindings, DescriptorPool& pool): m_Device(device) {
	auto setLayoutBuilder = DescriptorSetLayout::Builder(device);
	for(int i = 0; i < bindings.size(); i++) { setLayoutBuilder.AddBinding(i, bindings[i].bindingType, bindings[i].bindingStage); }
	m_DescriptorSetLayout = setLayoutBuilder.Build();

	DescriptorWriter writer(*m_DescriptorSetLayout, pool);
	for(int i = 0; i < bindings.size(); i++) {
		if(bindings[i].bindingType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
			VkDescriptorImageInfo imageDescriptor {};
			imageDescriptor.sampler     = bindings[i].sampler;
			imageDescriptor.imageView   = bindings[i].imageView;
			imageDescriptor.imageLayout = bindings[i].imageLayout;
			m_ImageDescriptors.push_back(imageDescriptor);
		}
		if(bindings[i].bindingType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
			m_UboBuffers.push_back(std::make_shared<Buffer>(device, bindings[i].bufferSize, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
			m_UboBuffers[m_BufferCount]->Map();
			m_BufferDescriptors.push_back(m_UboBuffers[m_BufferCount]->DescriptorInfo());
			m_BufferCount++;
		}
	}
	for(int i = 0; i < bindings.size(); i++) {
		switch(bindings[i].bindingType) {
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
				writer.WriteImage(i, &m_ImageDescriptors[i]);
			} break;

			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
				writer.WriteBuffer(i, &m_BufferDescriptors[i]);
			} break;

			default: throw std::runtime_error("Binding type not supported!"); break;
		}
	}
	writer.Build(m_DescriptorSet);
}

Uniform::~Uniform() {}