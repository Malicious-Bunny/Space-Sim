#pragma once

#include "device.h"
#include "sampler.h"
#include "string.h"

#include <array>

class Cubemap {
public:
	Cubemap(Device& device);
	~Cubemap();

	void CreateImage(uint32_t width, uint32_t height);
	void CreateImageFromTexture(const std::array<std::string, 6>& filepaths);

	inline VkSampler GetCubeMapImageSampler() { return m_CubeMapSampler.GetSampler(); }

	inline VkImage GetCubeMapImage() { return m_CubeMapImage; }

	inline VkImageView GetCubeMapImageView() { return m_CubeMapImageView; }

private:
	Device& m_Device;

	int32_t m_Width, m_Height, m_TextureChannles;

	VkImage m_CubeMapImage;
	VkImageView m_CubeMapImageView;
	VkDeviceMemory m_CubeMapImageMemory;

	Sampler m_CubeMapSampler;

	// Create a host-visible staging buffer that contains the raw image data
	VkBuffer m_StagingBuffer;
	VkDeviceMemory m_StagingMemory;
};