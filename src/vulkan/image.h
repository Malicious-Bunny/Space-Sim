#pragma once

#include "buffer.h"
#include "device.h"

#include <memory>
#include <vulkan/vulkan_core.h>

struct Size {
	int width;
	int height;
};

class Image {
public:
	Image(Device& device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlagBits aspect);
	Image(Device& device, const std::string& filepath);
	~Image();
	static void TransitionImageLayout(Device& device, const VkImage& image, const VkImageLayout& oldLayout, const VkImageLayout& newLayout, const VkImageSubresourceRange& subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
	void CopyBufferToImage(VkBuffer buffer, uint32_t width, uint32_t height);

	inline VkImage GetImage() { return m_Image; }

	inline VkImageView GetImageView() { return m_ImageView; }

	inline VkDeviceMemory GetImageMemory() { return m_ImageMemory; }

private:
	void CreateImageView(VkFormat format, VkImageAspectFlagBits aspect);
	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage);
	Device& m_Device;

	VkImage m_Image;
	VkImageView m_ImageView;
	VkDeviceMemory m_ImageMemory;

	VkBuffer m_Buffer;
	VkDeviceMemory m_BufferMemory;

	Size m_Size;
};