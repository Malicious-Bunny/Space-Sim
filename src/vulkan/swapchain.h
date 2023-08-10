#pragma once

#include "device.h"
#include "image.h"
#include "framebuffer.h"

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

class Swapchain {
public:
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
	Swapchain(Device& deviceRef, VkExtent2D windowExtent);
	Swapchain(Device& deviceRef, VkExtent2D windowExtent, std::shared_ptr<Swapchain> previousSwapchain);
	~Swapchain();

	Swapchain(const Swapchain&)            = delete;
	Swapchain& operator=(const Swapchain&) = delete;

	VkRenderPass GetGeometryRenderPass() { return m_GeometryRenderPass; }
	VkRenderPass GetShadowMapRenderPass() { return m_ShadowMapRenderPass; }

	VkFramebuffer GetGeometryFrameBuffer(int index);
	VkFramebuffer GetShadowMapFrameBuffer(int index);

	uint32_t GetWidth() { return m_SwapchainExtent.width; }

	uint32_t GetHeight() { return m_SwapchainExtent.height; }

	VkFormat GetSwapchainImageFormat() { return m_SwapchainImageFormat; }

	size_t GetImageCount() { return m_PresentableImageViews.size(); }

	VkExtent2D GetSwapchainExtent() { return m_SwapchainExtent; }

	VkResult SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);
	VkResult AcquireNextImage(uint32_t* imageIndex);

	bool CompareSwapFormats(const Swapchain& swapChain) const { return swapChain.m_SwapchainDepthFormat == m_SwapchainDepthFormat && swapChain.m_SwapchainImageFormat == m_SwapchainImageFormat; }

	float GetExtentAspectRatio() { return float(m_SwapchainExtent.width) / float(m_SwapchainExtent.height); }

private:
	void CreateSwapchain();
	void CreateImageViews();
	void CreateDepthResources();
	void CreateRenderPass();
	void CreateFramebuffers();
	void CreateSyncObjects();

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	VkFormat FindDepthFormat();

	std::shared_ptr<Swapchain> m_OldSwapchain;
	VkSwapchainKHR m_Swapchain;
	Device& m_Device;
	VkExtent2D m_WindowExtent;

	size_t m_CurrentFrame = 0;
	std::vector<VkFramebuffer> m_SwapchainFramebuffers;

	std::vector<std::shared_ptr<Image>> m_PresentableDepthImages;
	std::vector<VkImage> m_PresentableImages;
	std::vector<VkImageView> m_PresentableImageViews;

	std::vector<std::shared_ptr<Framebuffer>> m_ShadowMapFramebuffer; // this has to be a pointer for some unknown to mankind reason...

	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;

	VkFormat m_SwapchainImageFormat;
	VkFormat m_SwapchainDepthFormat;
	VkExtent2D m_SwapchainExtent;

	VkRenderPass m_GeometryRenderPass;
	VkRenderPass m_ShadowMapRenderPass;
};