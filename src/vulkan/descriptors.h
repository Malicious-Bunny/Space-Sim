#pragma once

#include "device.h"

#include <memory>
#include <unordered_map>
#include <vector>

class DescriptorSetLayout {
public:
	class Builder {
	public:
		Builder(Device& device): m_Device(device) {}

		void AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1);
		std::shared_ptr<DescriptorSetLayout> Build() const;

	private:
		Device& m_Device;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings {};
	};

	DescriptorSetLayout(Device& device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
	~DescriptorSetLayout();
	DescriptorSetLayout(const DescriptorSetLayout&)            = delete;
	DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

	inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

private:
	Device& m_Device;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings;

	friend class DescriptorWriter;
};

class DescriptorPool {
public:
	class Builder {
	public:
		Builder(Device& device): m_Device {device} {}

		Builder& AddPoolSize(VkDescriptorType descriptorType, uint32_t count);
		Builder& SetPoolFlags(VkDescriptorPoolCreateFlags flags);
		Builder& SetMaxSets(uint32_t count);
		std::unique_ptr<DescriptorPool> Build() const;

	private:
		Device& m_Device;
		std::vector<VkDescriptorPoolSize> m_PoolSizes {};
		uint32_t m_MaxSets                      = 1000;
		VkDescriptorPoolCreateFlags m_PoolFlags = 0;
	};

	DescriptorPool(Device& device, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes);
	~DescriptorPool();
	DescriptorPool(const DescriptorPool&)            = delete;
	DescriptorPool& operator=(const DescriptorPool&) = delete;

	bool AllocateDescriptorSets(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

	void FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

	inline VkDescriptorPool GetDescriptorPool() { return m_DescriptorPool; }

	void ResetPool();

private:
	Device& m_Device;
	VkDescriptorPool m_DescriptorPool;

	friend class DescriptorWriter;
};

class DescriptorWriter {
public:
	DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool);

	void WriteBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
	void WriteImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

	bool Build(VkDescriptorSet& set);
	void Overwrite(VkDescriptorSet& set);

private:
	DescriptorSetLayout& m_SetLayout;
	DescriptorPool& m_Pool;
	std::vector<VkWriteDescriptorSet> m_Writes;
};