#include "renderer.h"

#include "vulkan/model.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#include <array>
#include <cassert>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vulkan/vulkan_core.h>

Renderer::Renderer(Window& window, Device& device, VkDescriptorPool pool): m_Window(window), m_Device(device), m_Pool(pool) {
	CreatePipelineLayouts();
	RecreateSwapchain();
	CreateCommandBuffers();

    ImGuiInit();
}

Renderer::~Renderer() {
	FreeCommandBuffers();
	vkDestroyPipelineLayout(m_Device.GetDevice(), m_DefaultPipelineLayout, nullptr);
	vkDestroyPipelineLayout(m_Device.GetDevice(), m_SkyboxPipelineLayout, nullptr);
	m_CommandBuffers.clear();

    ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

static void CheckVkResult(VkResult err) {
	if(err == 0) return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if(err < 0) abort();
}

void Renderer::ImGuiInit()
{
    // ImGui Creation
	ImGui::CreateContext();
	auto io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplGlfw_InitForVulkan(m_Window.GetGLFWwindow(), true);
	ImGui_ImplVulkan_InitInfo info {};
	info.Instance        = m_Device.GetInstance();
	info.PhysicalDevice  = m_Device.GetPhysicalDevice();
	info.Device          = m_Device.GetDevice();
	info.Queue           = m_Device.GetGraphicsQueue();
	info.DescriptorPool  = m_Pool;
	info.Subpass         = 0;
	info.MinImageCount   = 2;
	info.ImageCount      = GetSwapchainImageCount();
	info.CheckVkResultFn = CheckVkResult;
	ImGui_ImplVulkan_Init(&info, GetGeometryRenderPass());

	VkCommandBuffer cmdBuffer;
	m_Device.BeginSingleTimeCommands(cmdBuffer);
	ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);
	m_Device.EndSingleTimeCommands(cmdBuffer);

	vkDeviceWaitIdle(m_Device.GetDevice());
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void Renderer::RecreateSwapchain() {
	auto extent = m_Window.GetExtent();
	while(extent.width == 0 || extent.height == 0) {
		extent = m_Window.GetExtent();
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(m_Device.GetDevice());

	if(m_Swapchain == nullptr) { m_Swapchain = std::make_unique<Swapchain>(m_Device, extent); }
	else {
		std::shared_ptr<Swapchain> oldSwapchain = std::move(m_Swapchain);
		m_Swapchain                             = std::make_unique<Swapchain>(m_Device, extent, oldSwapchain);
		if(!oldSwapchain->CompareSwapFormats(*m_Swapchain.get())) { throw std::runtime_error("Swap chain image or depth formats have changed!"); }
	}

	CreatePipelines();
}

void Renderer::CreateCommandBuffers() {
	m_CommandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo {};
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool        = m_Device.GetCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());
	if(vkAllocateCommandBuffers(m_Device.GetDevice(), &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) { throw std::runtime_error("failed to allocate command buffers!"); }
}

void Renderer::FreeCommandBuffers() {
	vkFreeCommandBuffers(m_Device.GetDevice(), m_Device.GetCommandPool(), static_cast<float>(m_CommandBuffers.size()), m_CommandBuffers.data());
	m_CommandBuffers.clear();
}

VkCommandBuffer Renderer::BeginFrame() {
	ASSERT(!m_IsFrameStarted);    // Can't call BeginFrame while already in progress!

	auto result = m_Swapchain->AcquireNextImage(&m_CurrentImageIndex);
	if(result == VK_ERROR_OUT_OF_DATE_KHR) {
		RecreateSwapchain();
		return nullptr;
	}
	if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) { throw std::runtime_error("failed to acquire swap chain image!"); }

	m_IsFrameStarted   = true;
	auto commandBuffer = GetCurrentCommandBuffer();

	VkCommandBufferBeginInfo beginInfo {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) { throw std::runtime_error("failed to begin recording command buffer!"); }
	return commandBuffer;
}

void Renderer::EndFrame() {
	auto commandBuffer = GetCurrentCommandBuffer();
	ASSERT(m_IsFrameStarted);    // Can't call EndFrame while frame is not in progress

	if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) { throw std::runtime_error("failed to record command buffer!"); }

	auto result = m_Swapchain->SubmitCommandBuffers(&commandBuffer, &m_CurrentImageIndex);
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window.WasWindowResized()) {
		m_Window.ResetWindowResizedFlag();
		RecreateSwapchain();
	}
	else if(result != VK_SUCCESS) { throw std::runtime_error("failed to present swap chain image!"); }

	m_IsFrameStarted    = false;
	m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::BeginSwapchainRenderPass(VkCommandBuffer commandBuffer, const glm::vec3& clearColor) {
	ASSERT(m_IsFrameStarted);                              // Can't call BeginSwapchainRenderPass while frame is not in progress
	ASSERT(commandBuffer == GetCurrentCommandBuffer());    // Can't Begin Render pass on command buffer from different frame

	VkRenderPassBeginInfo renderPassInfo {};
	renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass        = m_Swapchain->GetGeometryRenderPass();
	renderPassInfo.framebuffer       = m_Swapchain->GetFrameBuffer(m_CurrentImageIndex);
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = m_Swapchain->GetSwapchainExtent();
	std::array<VkClearValue, 2> clearValues {};
	clearValues[0].color           = {clearColor.r, clearColor.g, clearColor.b};
	clearValues[1].depthStencil    = {1.0f, 0};
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues    = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport {};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = static_cast<float>(m_Swapchain->GetSwapchainExtent().width);
	viewport.height   = static_cast<float>(m_Swapchain->GetSwapchainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor {
		{0, 0},
		m_Swapchain->GetSwapchainExtent()
    };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::EndSwapchainRenderPass(VkCommandBuffer commandBuffer) {
	ASSERT(m_IsFrameStarted);                              // Can't call EndSwapchainRenderPass while frame is not in progress
	ASSERT(commandBuffer == GetCurrentCommandBuffer());    // Can't end render pass on command buffer from different frame

	vkCmdEndRenderPass(commandBuffer);
}

void Renderer::Render(FrameInfo frameInfo, std::function<void(VkCommandBuffer& commandBuffer)> renderImGui) {
	if(auto commandBuffer = BeginFrame()) {
		frameInfo.commandBuffer = commandBuffer;
		// ------------------- RENDER PASS -----------------
		BeginSwapchainRenderPass(commandBuffer, {0.01f, 0.01f, 0.01f});

		RenderSkybox(frameInfo);

		RenderGameObjects(frameInfo);

		renderImGui(commandBuffer);

		EndSwapchainRenderPass(commandBuffer);
		EndFrame();
	}
}

void Renderer::RenderGameObjects(FrameInfo& frameInfo) {
	vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DefaultPipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

	vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DefaultPipelineLayout, 1, 1, &frameInfo.lightsDescriptorSet, 0, nullptr);

	for(std::pair<int, std::shared_ptr<Object>> obj : frameInfo.gameObjects) {
		m_DefaultPipeline->Bind(frameInfo.commandBuffer);

		PushConstantsPBR push {};
		push.modelMatrix  = obj.second->GetObjectTransform().mat4(frameInfo.camera.m_Translation);
		push.normalMatrix = glm::transpose(glm::inverse(glm::mat3(obj.second->GetObjectTransform().mat4(frameInfo.camera.m_Translation))));

		vkCmdPushConstants(frameInfo.commandBuffer, m_DefaultPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantsPBR), &push);

		obj.second->Draw(m_DefaultPipelineLayout, frameInfo.commandBuffer);
	}
}

void Renderer::RenderSkybox(FrameInfo& frameInfo) {
	m_SkyboxPipeline->Bind(frameInfo.commandBuffer);

	vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyboxPipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

	vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyboxPipelineLayout, 1, 1, &frameInfo.skyboxDescriptorSet, 0, nullptr);

	glm::dvec3 translation = {0.0, 0.0, 0.0};
	glm::dvec3 scale       = glm::vec3 {5.0f};
	auto transform         = glm::translate(glm::dmat4 {1.0f}, translation);
	transform              = glm::scale(transform, scale);

	PushConstants push {};
	push.modelMatrix = transform;

	vkCmdPushConstants(frameInfo.commandBuffer, m_SkyboxPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push);

	frameInfo.skybox->GetSkyboxModel()->Bind(frameInfo.commandBuffer);
	frameInfo.skybox->GetSkyboxModel()->Draw(frameInfo.commandBuffer);
}

void Renderer::CreatePipelineLayouts() {
	//
	// PBR Pipline layout
	//
	{
		auto globalLayoutBuilder = DescriptorSetLayout::Builder(m_Device);
		globalLayoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
		auto globalLayout = globalLayoutBuilder.Build();

		auto texturesLayoutBuilder = DescriptorSetLayout::Builder(m_Device);
		texturesLayoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		texturesLayoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		texturesLayoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		texturesLayoutBuilder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		auto textureLayout = texturesLayoutBuilder.Build();

		auto lightsLayoutBuilder = DescriptorSetLayout::Builder(m_Device);
		lightsLayoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
		auto lightsLayout = lightsLayoutBuilder.Build();

		VkPushConstantRange pushConstantRange {};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset     = 0;
		pushConstantRange.size       = sizeof(PushConstantsPBR);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts {globalLayout->GetDescriptorSetLayout(), lightsLayout->GetDescriptorSetLayout(), textureLayout->GetDescriptorSetLayout()};
		Pipeline::CreatePipelineLayout(m_Device, descriptorSetLayouts, m_DefaultPipelineLayout, &pushConstantRange);
	}

	//
	// Skybox Pipeline layout
	//
	{
		auto globalLayoutBuilder = DescriptorSetLayout::Builder(m_Device);
		globalLayoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
		auto globalLayout = globalLayoutBuilder.Build();

		auto skyboxLayoutBuilder = DescriptorSetLayout::Builder(m_Device);
		skyboxLayoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		auto skyboxLayout = skyboxLayoutBuilder.Build();

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts {globalLayout->GetDescriptorSetLayout(), skyboxLayout->GetDescriptorSetLayout()};

		VkPushConstantRange pushConstantRange {};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset     = 0;
		pushConstantRange.size       = sizeof(PushConstants);

		Pipeline::CreatePipelineLayout(m_Device, descriptorSetLayouts, m_SkyboxPipelineLayout, &pushConstantRange);
	}
}

void Renderer::CreatePipelines() {
	//
	// PBR Pipline
	//
	{
		PipelineConfigInfo pipelineConfig{};
		Pipeline::CreatePipelineConfigInfo(pipelineConfig, m_Swapchain->GetWidth(), m_Swapchain->GetHeight(),
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, true, true
		);
		pipelineConfig.renderPass     = m_Swapchain->GetGeometryRenderPass();
		pipelineConfig.pipelineLayout = m_DefaultPipelineLayout;
		m_DefaultPipeline             = std::make_unique<Pipeline>(m_Device);
		m_DefaultPipeline->CreatePipeline("../../shaders/spv/PBR.vert.spv", "../../shaders/spv/PBR.frag.spv", pipelineConfig, Model::Vertex::GetBindingDescriptions(),
		                                  Model::Vertex::GetAttributeDescriptions());
	}

	//
	// Skybox Pipeline
	//
	{
		PipelineConfigInfo pipelineConfig{};
		Pipeline::CreatePipelineConfigInfo(pipelineConfig, m_Swapchain->GetWidth(), m_Swapchain->GetHeight(),
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, true, true
		);
		pipelineConfig.renderPass = m_Swapchain->GetGeometryRenderPass();
		pipelineConfig.pipelineLayout = m_SkyboxPipelineLayout;
		m_SkyboxPipeline              = std::make_unique<Pipeline>(m_Device);
		m_SkyboxPipeline->CreatePipeline("../../shaders/spv/skybox.vert.spv", "../../shaders/spv/skybox.frag.spv", pipelineConfig, Model::Vertex::GetBindingDescriptions(),
		                                 Model::Vertex::GetAttributeDescriptions());
	}
}