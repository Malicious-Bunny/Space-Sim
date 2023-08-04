#pragma once
#include "camera.h"
#include "condition_variable"
#include "frameInfo.h"
#include "object.h"
#include "utilities.h"
#include "vulkan/device.h"
#include "vulkan/pipeline.h"
#include "vulkan/skybox.h"
#include "vulkan/swapchain.h"
#include "vulkan/window.h"

#include <cassert>
#include <functional>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

struct PushConstants {
	glm::mat4 modelMatrix {1.0f};
};

struct PushConstantsPBR {
	glm::mat4 modelMatrix {1.0f};
	glm::mat4 normalMatrix {1.0f};
};

class Renderer {
public:
	Renderer(Window& window, Device& device, VkDescriptorPool pool);
	~Renderer();

	Renderer(const Renderer&)            = delete;
	Renderer& operator=(const Renderer&) = delete;

	inline uint32_t GetSwapchainImageCount() { return m_Swapchain->GetImageCount(); }

	inline VkRenderPass GetGeometryRenderPass() { return m_Swapchain->GetGeometryRenderPass(); }

	inline float GetAspectRatio() { return m_Swapchain->GetExtentAspectRatio(); }

	inline bool IsFrameInProgress() const { return m_IsFrameStarted; }

	VkCommandBuffer GetCurrentCommandBuffer() const {
		ASSERT(m_IsFrameStarted);    // Cannot get command buffer when frame is not in progress
		return m_CommandBuffers[m_CurrentImageIndex];
	}

	int GetFrameIndex() const {
		if(m_IsFrameStarted)    // Cannot get frame index when frameis not in progress
			return m_CurrentFrameIndex;
		else return 0;
	}

	VkCommandBuffer BeginFrame();
	void EndFrame();
	void BeginSwapchainRenderPass(VkCommandBuffer commandBuffer, const glm::vec3& clearColor);
	void EndSwapchainRenderPass(VkCommandBuffer commandBuffer);
	void RenderGameObjects(FrameInfo& frameInfo);
	void RenderSkybox(FrameInfo& frameInfo);

	void Render(FrameInfo frameInfo, std::function<void(VkCommandBuffer& commandBuffer)> renderImGui);

private:
	void CreateCommandBuffers();
	void FreeCommandBuffers();
	void RecreateSwapchain();
    void ImGuiInit();

	void CreatePipelines();

	void CreatePipelineLayouts();

	Window& m_Window;
	Device& m_Device;
    VkDescriptorPool m_Pool;
	std::unique_ptr<Swapchain> m_Swapchain;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	std::unique_ptr<Pipeline> m_DefaultPipeline;
	VkPipelineLayout m_DefaultPipelineLayout;

	std::unique_ptr<Pipeline> m_SkyboxPipeline;
	VkPipelineLayout m_SkyboxPipelineLayout;

	uint32_t m_CurrentImageIndex = 0;
	int m_CurrentFrameIndex      = 0;
	bool m_IsFrameStarted        = false;
};