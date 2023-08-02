#include "cubemap.h"

#include "image.h"
#include "stbimage/stb_image.h"

#include <memory>
#include <stdexcept>

Cubemap::Cubemap(Device& device): m_Device(device), m_CubeMapSampler(device) {}

Cubemap::~Cubemap() {
	vkDestroyImage(m_Device.GetDevice(), m_CubeMapImage, nullptr);
	vkFreeMemory(m_Device.GetDevice(), m_CubeMapImageMemory, nullptr);
	vkDestroyImageView(m_Device.GetDevice(), m_CubeMapImageView, nullptr);
}

void Cubemap::CreateImage(uint32_t width, uint32_t height) {
	m_Width  = width;
	m_Height = height;
	// Create optimal tiled target image
	VkImageCreateInfo imageCreateInfo {};
	imageCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.mipLevels     = 1;
	imageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.extent        = {(uint32_t) m_Width, (uint32_t) m_Height, 1};
	imageCreateInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	// Cube faces count as array layers in Vulkan
	imageCreateInfo.arrayLayers = 6;
	// This flag is required for cube map images
	imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	if(vkCreateImage(m_Device.GetDevice(), &imageCreateInfo, nullptr, &m_CubeMapImage) != VK_SUCCESS) { throw std::runtime_error("Couldn't create image"); }
}

void Cubemap::CreateImageFromTexture(const std::array<std::string, 6>& filepaths) {
	std::array<stbi_uc*, 6> pixels;

	for(int i = 0; i < 6; i++) { pixels[i] = stbi_load(filepaths[i].c_str(), &m_Width, &m_Height, &m_TextureChannles, STBI_rgb_alpha); }
	CreateImage(m_Width, m_Height);

	VkDeviceSize allSize   = m_Width * m_Height * 4 * 6;
	VkDeviceSize imageSize = m_Width * m_Height * 4;

	VkMemoryAllocateInfo memAllocInfo {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;

	VkBufferCreateInfo bufferCreateInfo {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size  = allSize;
	// This buffer is used as a transfer source for the buffer copy
	bufferCreateInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateBuffer(m_Device.GetDevice(), &bufferCreateInfo, nullptr, &m_StagingBuffer);

	vkGetBufferMemoryRequirements(m_Device.GetDevice(), m_StagingBuffer, &memReqs);
	memAllocInfo.allocationSize = memReqs.size;
	// Get memory type index for a host visible buffer
	memAllocInfo.memoryTypeIndex = m_Device.FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkAllocateMemory(m_Device.GetDevice(), &memAllocInfo, nullptr, &m_StagingMemory);
	vkBindBufferMemory(m_Device.GetDevice(), m_StagingBuffer, m_StagingMemory, 0);

	// Copy texture data into staging buffer
	uint8_t* data;
	vkMapMemory(m_Device.GetDevice(), m_StagingMemory, 0, memReqs.size, 0, (void**) &data);
	for(int i = 0; i < 6; i++) { memcpy((char*) data + (imageSize * i), pixels[i], static_cast<size_t>(imageSize)); }
	vkUnmapMemory(m_Device.GetDevice(), m_StagingMemory);

	vkGetImageMemoryRequirements(m_Device.GetDevice(), m_CubeMapImage, &memReqs);
	memAllocInfo.allocationSize  = memReqs.size;
	memAllocInfo.memoryTypeIndex = m_Device.FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vkAllocateMemory(m_Device.GetDevice(), &memAllocInfo, nullptr, &m_CubeMapImageMemory);
	vkBindImageMemory(m_Device.GetDevice(), m_CubeMapImage, m_CubeMapImageMemory, 0);

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel            = 0;
	subresourceRange.levelCount              = 1;
	subresourceRange.baseArrayLayer          = 0;
	subresourceRange.layerCount              = 6;

	Image::TransitionImageLayout(m_Device, m_CubeMapImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

	VkCommandBuffer commandBuffer;
	m_Device.BeginSingleTimeCommands(commandBuffer);

	VkBufferImageCopy region {};
	region.bufferOffset                    = 0;
	region.bufferRowLength                 = 0;
	region.bufferImageHeight               = 0;
	region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel       = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount     = 6;
	region.imageOffset                     = {0, 0, 0};
	region.imageExtent                     = {(uint32_t) m_Width, (uint32_t) m_Height, 1};

	vkCmdCopyBufferToImage(commandBuffer, m_StagingBuffer, m_CubeMapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	m_Device.EndSingleTimeCommands(commandBuffer);

	Image::TransitionImageLayout(m_Device, m_CubeMapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

	// Create sampler
	m_CubeMapSampler.CreateCubemapSampler();
	// Create image view
	VkImageViewCreateInfo view {};
	view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	// Cube map view type
	view.viewType         = VK_IMAGE_VIEW_TYPE_CUBE;
	view.format           = VK_FORMAT_R8G8B8A8_UNORM;
	view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	// 6 array layers (faces)
	view.subresourceRange.layerCount = 6;
	// Set number of mip levels
	view.subresourceRange.levelCount = 1;
	view.image                       = m_CubeMapImage;
	vkCreateImageView(m_Device.GetDevice(), &view, nullptr, &m_CubeMapImageView);

	vkFreeMemory(m_Device.GetDevice(), m_StagingMemory, nullptr);
	vkDestroyBuffer(m_Device.GetDevice(), m_StagingBuffer, nullptr);

	for(int i = 0; i < 6; i++) { stbi_image_free(pixels[i]); }
}