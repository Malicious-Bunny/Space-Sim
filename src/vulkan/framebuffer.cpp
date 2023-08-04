#include "framebuffer.h"
#include <stdexcept>

#include <vulkan/vulkan_core.h>

Framebuffer::Framebuffer(Device& device, const std::vector<FramebufferAttachment>& attachmentsFormats, const VkRenderPass& renderPass, VkExtent2D& extent, VkFormat& depthFormat)
: m_Device(device), m_Extent(extent), m_DepthFormat(depthFormat) {
	for(int i = 0; i < attachmentsFormats.size(); i++) {
		switch(attachmentsFormats[i]) {
			case FramebufferAttachment::Color: CreateColorAttachment(); break;

			case FramebufferAttachment::Depth: CreateDepthAttachment(); break;
		}
	}

	std::vector<VkImageView> attachments;
	for(int i = 0; i < m_ColorAttachments.size(); i++) attachments.push_back(m_ColorAttachments[i].GetImageView());
	for(int i = 0; i < m_DepthAttachments.size(); i++) attachments.push_back(m_DepthAttachments[i].GetImageView());

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass              = renderPass;
	framebufferInfo.attachmentCount         = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments            = attachments.data();
	framebufferInfo.width                   = m_Extent.width;
	framebufferInfo.height                  = m_Extent.height;
	framebufferInfo.layers                  = 1;

	if(vkCreateFramebuffer(m_Device.GetDevice(), &framebufferInfo, nullptr, &m_Framebuffer) != VK_SUCCESS) { throw std::runtime_error("failed to create framebuffer!"); }
}

Framebuffer::~Framebuffer() { vkDestroyFramebuffer(m_Device.GetDevice(), m_Framebuffer, nullptr); }

void Framebuffer::CreateColorAttachment() {
	m_ColorAttachments.emplace_back(m_Device, m_Extent.width, m_Extent.height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	//Image::TransitionImageLayout(m_Device, m_ColorAttachments[m_ColorAttachments.size()-1].GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    // No idea if Transitioning layout of attachments to read optimal is right or wrong to do
}

void Framebuffer::CreateDepthAttachment() {
	m_ColorAttachments.emplace_back(m_Device, m_Extent.width, m_Extent.height, m_DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	                                VK_IMAGE_ASPECT_DEPTH_BIT);
	//Image::TransitionImageLayout(m_Device, m_ColorAttachments[m_ColorAttachments.size()-1].GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    // No idea if Transitioning layout of attachments to read optimal is right or wrong to do
}