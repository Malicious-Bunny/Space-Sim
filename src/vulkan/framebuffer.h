#pragma once

#include "device.h"
#include "image.h"

#include <vector>
#include <vulkan/vulkan.h>

enum FramebufferAttachment {
	Depth,
	Color,
};

class Framebuffer {
public:
	Framebuffer(Device& device, const std::vector<FramebufferAttachment> attachmentsFormats, VkRenderPass renderPass, VkExtent2D extent, VkFormat depthFormat);
	~Framebuffer();

	inline VkFramebuffer GetFramebuffer() { return m_Framebuffer; } 

private:
	void CreateColorAttachment();
	void CreateDepthAttachment();

	VkExtent2D m_Extent;
	Device& m_Device;

	std::vector<VkImageView> m_Attachments;
	VkFramebuffer m_Framebuffer;
	std::vector<Image> m_ColorAttachments;
	std::vector<Image> m_DepthAttachments;

	VkFormat m_DepthFormat;
};