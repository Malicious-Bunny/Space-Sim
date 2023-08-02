#include "swapchain.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

Swapchain::Swapchain(Device& deviceRef, VkExtent2D windowExtent): m_Device(deviceRef), m_WindowExtent(windowExtent) {
	CreateSwapchain();
	CreateImageViews();
	CreateRenderPass();
	CreateDepthResources();
	CreateFramebuffers();
	CreateSyncObjects();
}

Swapchain::Swapchain(Device& deviceRef, VkExtent2D windowExtent, std::shared_ptr<Swapchain> previousSwapchain): m_Device(deviceRef), m_WindowExtent(windowExtent), m_OldSwapchain(previousSwapchain) {
	CreateSwapchain();
	CreateImageViews();
	CreateRenderPass();
	CreateDepthResources();
	CreateFramebuffers();
	CreateSyncObjects();

	m_OldSwapchain = nullptr;
}

Swapchain::~Swapchain() {
	for(auto imageView : m_PresentableImageViews) { vkDestroyImageView(m_Device.GetDevice(), imageView, nullptr); }
	m_PresentableImageViews.clear();

	if(m_Swapchain != nullptr) {
		vkDestroySwapchainKHR(m_Device.GetDevice(), m_Swapchain, nullptr);
		m_Swapchain = nullptr;
	}

	for(auto framebuffer : m_SwapchainFramebuffers) { vkDestroyFramebuffer(m_Device.GetDevice(), framebuffer, nullptr); }

	vkDestroyRenderPass(m_Device.GetDevice(), m_GeometryRenderPass, nullptr);

	// cleanup synchronization objects
	for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(m_Device.GetDevice(), m_RenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_Device.GetDevice(), m_ImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_Device.GetDevice(), m_InFlightFences[i], nullptr);
	}
}

VkSurfaceFormatKHR Swapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for(const auto& availableFormat : availableFormats) {
		// VK_COLOR_SPACE_SRGB_NONLINEAR_KHR specifes if SRGB color space is used
		// SRGB color space results in more accurate perceived colors* + it is standard for textures
		// * https://stackoverflow.com/questions/12524623/what-are-the-practical-differences-when-working-with-colors-in-a-linear-vs-a-no
		if(availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { return availableFormat; }
	}

	return availableFormats[0];
}

/**
 * @brief Chooses how to present images to Screen
 * @brief Mailbox - Most efficent one, address the screen tearing issue in immediate mode. (not supported by Linux)
 * @brief Immediate - Presents images on screen as fast as possible. Possible screen tearing.
 * @brief V-Sync (FIFO) - Synchronizes presenting images with monitor refresh rate.
*/
VkPresentModeKHR Swapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	// for (const auto &availablePresentMode : availablePresentModes)
	// {
	//     if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
	//     {
	//         std::cout << "Present mode: Mailbox" << std::endl;
	//         return availablePresentMode;
	//     }
	// }
	// for (const auto &availablePresentMode : availablePresentModes)
	// {
	//    if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
	//    {
	//        std::cout << "Present mode: Immediate" << std::endl;
	//        return availablePresentMode;
	//    }
	// }

	std::cout << "Present mode: V-Sync" << std::endl;
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) { return capabilities.currentExtent; }
	else {
		VkExtent2D actualExtent = m_WindowExtent;
		actualExtent.width      = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height     = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

void Swapchain::CreateSwapchain() {
	SwapchainSupportDetails swapChainSupport = m_Device.GetSwapchainSupport();

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode     = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent                = ChooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = MAX_FRAMES_IN_FLIGHT;
	if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) { imageCount = swapChainSupport.capabilities.maxImageCount; }

	VkSwapchainCreateInfoKHR createInfo {};
	createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_Device.GetSurface();

	createInfo.minImageCount    = imageCount;
	createInfo.imageFormat      = surfaceFormat.format;
	createInfo.imageColorSpace  = surfaceFormat.colorSpace;
	createInfo.imageExtent      = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices    = m_Device.FindPhysicalQueueFamilies();
	uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

	// if graphics and present queue are the same which happens on some hardware create images in sharing mode
	if(indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices   = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;          // Optional
		createInfo.pQueueFamilyIndices   = nullptr;    // Optional
	}

	createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped     = VK_TRUE;    // discards pixels that are obscured (for example behind other window)

	createInfo.oldSwapchain = m_OldSwapchain == nullptr ? VK_NULL_HANDLE : m_OldSwapchain->m_Swapchain;

	if(vkCreateSwapchainKHR(m_Device.GetDevice(), &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS) { throw std::runtime_error("failed to create swap chain!"); }

	vkGetSwapchainImagesKHR(m_Device.GetDevice(), m_Swapchain, &imageCount, nullptr);
	m_PresentableImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_Device.GetDevice(), m_Swapchain, &imageCount, m_PresentableImages.data());

	m_SwapchainImageFormat = surfaceFormat.format;
	m_SwapchainExtent      = extent;
}

void Swapchain::CreateImageViews() {
	m_PresentableImageViews.resize(m_PresentableImages.size());

	for(size_t i = 0; i < m_PresentableImages.size(); i++) {
		VkImageViewCreateInfo createInfo {};
		createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image                           = m_PresentableImages[i];
		createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format                          = m_SwapchainImageFormat;
		createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel   = 0;
		createInfo.subresourceRange.levelCount     = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount     = 1;

		if(vkCreateImageView(m_Device.GetDevice(), &createInfo, nullptr, &m_PresentableImageViews[i]) != VK_SUCCESS) { throw std::runtime_error("failed to create texture image view!"); }
	}
}

void Swapchain::CreateRenderPass() {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format                  = GetSwapchainImageFormat();
	colorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment            = 0;
	colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment {};
	depthAttachment.format         = FindDepthFormat();
	depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass    = {};
	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount    = 1;
	subpass.pColorAttachments       = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
	dependency.srcAccessMask       = 0;
	dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstSubpass          = 0;
	dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
	VkRenderPassCreateInfo renderPassInfo              = {};
	renderPassInfo.sType                               = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount                     = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments                        = attachments.data();
	renderPassInfo.subpassCount                        = 1;
	renderPassInfo.pSubpasses                          = &subpass;
	renderPassInfo.dependencyCount                     = 0;
	renderPassInfo.pDependencies                       = nullptr;

	if(vkCreateRenderPass(m_Device.GetDevice(), &renderPassInfo, nullptr, &m_GeometryRenderPass) != VK_SUCCESS) { throw std::runtime_error("failed to create render pass!"); }
}

VkFormat Swapchain::FindDepthFormat() {
	return m_Device.FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void Swapchain::CreateFramebuffers() {
	m_SwapchainFramebuffers.resize(GetImageCount());
	for(size_t i = 0; i < GetImageCount(); i++) {
		std::array<VkImageView, 2> attachments = {m_PresentableImageViews[i], m_PresentableDepthImages[i]->GetImageView()};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass              = m_GeometryRenderPass;
		framebufferInfo.attachmentCount         = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments            = attachments.data();
		framebufferInfo.width                   = m_SwapchainExtent.width;
		framebufferInfo.height                  = m_SwapchainExtent.height;
		framebufferInfo.layers                  = 1;

		if(vkCreateFramebuffer(m_Device.GetDevice(), &framebufferInfo, nullptr, &m_SwapchainFramebuffers[i]) != VK_SUCCESS) { throw std::runtime_error("failed to create framebuffer!"); }
	}
}

void Swapchain::CreateDepthResources() {
	m_SwapchainDepthFormat     = FindDepthFormat();
	VkExtent2D swapChainExtent = GetSwapchainExtent();

	m_PresentableDepthImages.resize(GetImageCount());

	for(int i = 0; i < GetImageCount(); i++) {
		m_PresentableDepthImages[i] = std::make_shared<Image>(m_Device, swapChainExtent.width, swapChainExtent.height, m_SwapchainDepthFormat, VK_IMAGE_TILING_OPTIMAL,
		                                                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    }
}

/**
 * @brief Synchronizes CPU-GPU work, submits command buffer into graphics queue and presents image 
*/
VkResult Swapchain::SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex) {
	VkSubmitInfo submitInfo = {};
	submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores        = m_ImageAvailableSemaphores[m_CurrentFrame];
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount     = 1;
	submitInfo.pWaitSemaphores        = &waitSemaphores;
	submitInfo.pWaitDstStageMask      = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = buffers;

	VkSemaphore signalSemaphores    = m_RenderFinishedSemaphores[m_CurrentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores    = &signalSemaphores;

	vkResetFences(m_Device.GetDevice(), 1, &m_InFlightFences[m_CurrentFrame]);
	if(vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) { throw std::runtime_error("failed to submit draw command buffer!"); }

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = &signalSemaphores;

	VkSwapchainKHR swapChains[] = {m_Swapchain};
	presentInfo.swapchainCount  = 1;
	presentInfo.pSwapchains     = swapChains;

	presentInfo.pImageIndices = imageIndex;

	auto result = vkQueuePresentKHR(m_Device.GetPresentQueue(), &presentInfo);

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return result;
}

/**
 * @brief Acquires next image from swapchain for rendering
 *
 * @param imageIndex Changes current image index to next available image in swapchain
 *
 * @return Returns result of Acquiring image from swapchain
*/
VkResult Swapchain::AcquireNextImage(uint32_t* imageIndex) {
	vkWaitForFences(m_Device.GetDevice(), 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

	VkResult result = vkAcquireNextImageKHR(m_Device.GetDevice(), m_Swapchain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, imageIndex);

	return result;
}

/**
 * @brief Creates objects for explicit synchronization
*/
void Swapchain::CreateSyncObjects() {
	m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

	for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if(vkCreateSemaphore(m_Device.GetDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
		   vkCreateSemaphore(m_Device.GetDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
		   vkCreateFence(m_Device.GetDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}