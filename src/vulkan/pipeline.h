#pragma once

#include "device.h"

#include <string>
#include <vector>

struct PipelineConfigInfo {
	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	VkPipelineLayout pipelineLayout = nullptr;
	VkRenderPass renderPass         = nullptr;
	uint32_t subpass                = 0;
};

class Pipeline {
public:
	Pipeline(Device& device);
	~Pipeline();

	Pipeline(const Pipeline&)            = delete;
	Pipeline& operator=(const Pipeline&) = delete;

	void Bind(VkCommandBuffer commandBuffer);

	static PipelineConfigInfo CreatePipelineConfigInfo(PipelineConfigInfo& configInfo, uint32_t width, uint32_t height, VkPrimitiveTopology topology, VkCullModeFlags cullMode, bool depthTestEnable, bool blendingEnable);
	void CreatePipeline(const std::string& vertexPath, const std::string& fragmentPath, const PipelineConfigInfo& configInfo,
	                    std::vector<VkVertexInputBindingDescription> bindingDesc     = std::vector<VkVertexInputBindingDescription>(),
	                    std::vector<VkVertexInputAttributeDescription> attributeDesc = std::vector<VkVertexInputAttributeDescription>());
	static void CreatePipelineLayout(Device& device, std::vector<VkDescriptorSetLayout>& descriptorSetsLayouts, VkPipelineLayout& pipelineLayout, VkPushConstantRange* pushConstants);

private:
	static std::vector<char> ReadFile(const std::string& filepath);

	void CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

	Device& m_Device;
	VkPipeline m_Pipeline;
};