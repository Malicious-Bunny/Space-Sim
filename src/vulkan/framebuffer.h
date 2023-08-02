#pragma once

#include "device.h"
#include "image.h"

#include <vector>
#include <vulkan/vulkan_core.h>

enum FramebufferAttachment {
	Depth,
	Color,
};

class Framebuffer {
public:
	Framebuffer(Device& device, const std::vector<FramebufferAttachment>& attachmentsFormats, const VkRenderPass& renderPass, VkExtent2D& extent, VkFormat& depthFormat);
	~Framebuffer();

private:
	void CreateColorAttachment();
	void CreateDepthAttachment();

	VkExtent2D m_Extent;
	Device& m_Device;

	VkFramebuffer m_Framebuffer;
	std::vector<Image> m_ColorAttachments;
	std::vector<Image> m_DepthAttachments;

	VkFormat m_DepthFormat;
};