#pragma once

#include "device.h"

class Buffer {
public:
	Buffer(Device& device, VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize minOffsetAlignment = 1);
	~Buffer();

	Buffer(const Buffer&)            = delete;
	Buffer& operator=(const Buffer&) = delete;

	VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void Unmap();

	void WriteToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	VkDescriptorBufferInfo DescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

	inline VkBuffer GetBuffer() const { return m_Buffer; }

	inline void* GetMappedMemory() const { return m_Mapped; }

	inline uint32_t GetInstanceCount() const { return m_InstanceCount; }

	inline VkDeviceSize GetInstanceSize() const { return m_InstanceSize; }

	inline VkDeviceSize GetAlignmentSize() const { return m_InstanceSize; }

	inline VkBufferUsageFlags GetUsageFlags() const { return m_UsageFlags; }

	inline VkMemoryPropertyFlags GetMemoryPropertyFlags() const { return m_MemoryPropertyFlags; }

	inline VkDeviceSize GetBufferSize() const { return m_BufferSize; }

private:
	static VkDeviceSize GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

	Device& m_Device;
	void* m_Mapped          = nullptr;
	VkBuffer m_Buffer       = VK_NULL_HANDLE;
	VkDeviceMemory m_Memory = VK_NULL_HANDLE;

	VkDeviceSize m_BufferSize;
	uint32_t m_InstanceCount;
	VkDeviceSize m_InstanceSize;
	VkDeviceSize m_AlignmentSize;
	VkBufferUsageFlags m_UsageFlags;
	VkMemoryPropertyFlags m_MemoryPropertyFlags;
};