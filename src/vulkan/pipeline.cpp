#include "pipeline.h"

#include "../utilities.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>

Pipeline::Pipeline(Device& device): m_Device(device) {}

Pipeline::~Pipeline() { vkDestroyPipeline(m_Device.GetDevice(), m_Pipeline, nullptr); }

std::vector<char> Pipeline::ReadFile(const std::string& filepath) {
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);    // ate goes to the end of the file so reading filesize is easier and binary avoids text transformation
	if(!file.is_open()) { throw std::runtime_error("failed to open file: " + filepath); }

	size_t fileSize = (size_t) (file.tellg());    // tellg gets current position in file
	std::vector<char> buffer(fileSize);
	file.seekg(0);    // return to the begining of the file
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}

void Pipeline::CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) {
	VkShaderModuleCreateInfo createInfo {};

	createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

	if(vkCreateShaderModule(m_Device.GetDevice(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) { throw std::runtime_error("failed to create shader module"); }
}

void Pipeline::Bind(VkCommandBuffer commandBuffer) { vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline); }

PipelineConfigInfo Pipeline::CreatePipelineConfigInfo(uint32_t width, uint32_t height, VkPrimitiveTopology topology, VkCullModeFlags cullMode, bool depthTestEnable, bool blendingEnable) {
	PipelineConfigInfo configInfo {};

	configInfo.inputAssemblyInfo.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	configInfo.inputAssemblyInfo.topology = topology;
	if(topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP || topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP) configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_TRUE;
	else configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	configInfo.viewport.x        = 0.0f;
	configInfo.viewport.y        = 0.0f;
	configInfo.viewport.width    = static_cast<float>(width);
	configInfo.viewport.height   = static_cast<float>(height);
	configInfo.viewport.minDepth = 0.0f;
	configInfo.viewport.maxDepth = 1.0f;

	configInfo.scissor.offset = {0, 0};
	configInfo.scissor.extent = {width, height};

	configInfo.rasterizationInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	configInfo.rasterizationInfo.depthClampEnable        = VK_FALSE;
	configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	configInfo.rasterizationInfo.polygonMode             = VK_POLYGON_MODE_FILL;
	configInfo.rasterizationInfo.lineWidth               = 1.0f;
	configInfo.rasterizationInfo.cullMode                = cullMode;
	configInfo.rasterizationInfo.frontFace               = VK_FRONT_FACE_CLOCKWISE;
	configInfo.rasterizationInfo.depthBiasEnable         = VK_FALSE;

	configInfo.multisampleInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	configInfo.multisampleInfo.sampleShadingEnable   = VK_FALSE;
	configInfo.multisampleInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	configInfo.multisampleInfo.minSampleShading      = 1.0f;
	configInfo.multisampleInfo.pSampleMask           = nullptr;
	configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
	configInfo.multisampleInfo.alphaToOneEnable      = VK_FALSE;

	if(blendingEnable) {
		configInfo.colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		configInfo.colorBlendAttachment.blendEnable         = VK_TRUE;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		configInfo.colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		configInfo.colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
	}
	else {
		configInfo.colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		configInfo.colorBlendAttachment.blendEnable         = VK_FALSE;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		configInfo.colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		configInfo.colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
	}

	configInfo.colorBlendInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	configInfo.colorBlendInfo.logicOpEnable     = VK_FALSE;
	configInfo.colorBlendInfo.logicOp           = VK_LOGIC_OP_COPY;
	configInfo.colorBlendInfo.attachmentCount   = 1;
	configInfo.colorBlendInfo.pAttachments      = &configInfo.colorBlendAttachment;
	configInfo.colorBlendInfo.blendConstants[0] = 0.0f;
	configInfo.colorBlendInfo.blendConstants[1] = 0.0f;
	configInfo.colorBlendInfo.blendConstants[2] = 0.0f;
	configInfo.colorBlendInfo.blendConstants[3] = 0.0f;

	configInfo.depthStencilInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	configInfo.depthStencilInfo.depthTestEnable       = depthTestEnable;
	configInfo.depthStencilInfo.depthWriteEnable      = VK_TRUE;
	configInfo.depthStencilInfo.depthCompareOp        = VK_COMPARE_OP_LESS;
	configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	configInfo.depthStencilInfo.minDepthBounds        = 0.0f;
	configInfo.depthStencilInfo.maxDepthBounds        = 1.0f;
	configInfo.depthStencilInfo.stencilTestEnable     = VK_FALSE;
	configInfo.depthStencilInfo.front                 = {};
	configInfo.depthStencilInfo.back                  = {};

	return configInfo;
}

void Pipeline::CreatePipeline(const std::string& vertexPath, const std::string& fragmentPath, const PipelineConfigInfo& configInfo, std::vector<VkVertexInputBindingDescription> bindingDesc,
                              std::vector<VkVertexInputAttributeDescription> attributeDesc) {
	ASSERT(configInfo.pipelineLayout != nullptr);    // Cannot create graphics pipeline: no pipelineLayout provided in config info
	ASSERT(configInfo.renderPass != nullptr);        // Cannot create graphics pipeline: no renderPass provided in config info

	auto vertCode = ReadFile(vertexPath);
	auto fragCode = ReadFile(fragmentPath);

	VkShaderModule vertexShaderModule;
	VkShaderModule fragmentShaderModule;

	CreateShaderModule(vertCode, &vertexShaderModule);
	CreateShaderModule(fragCode, &fragmentShaderModule);

	VkPipelineShaderStageCreateInfo shaderStages[2];
	shaderStages[0].sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage               = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module              = vertexShaderModule;
	shaderStages[0].pName               = "main";
	shaderStages[0].flags               = 0;
	shaderStages[0].pNext               = nullptr;
	shaderStages[0].pSpecializationInfo = nullptr;

	shaderStages[1].sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage               = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module              = fragmentShaderModule;
	shaderStages[1].pName               = "main";
	shaderStages[1].flags               = 0;
	shaderStages[1].pNext               = nullptr;
	shaderStages[1].pSpecializationInfo = nullptr;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
	vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t) attributeDesc.size();
	vertexInputInfo.pVertexAttributeDescriptions    = attributeDesc.data();
	vertexInputInfo.vertexBindingDescriptionCount   = (uint32_t) bindingDesc.size();
	vertexInputInfo.pVertexBindingDescriptions      = bindingDesc.data();

	VkPipelineViewportStateCreateInfo viewportInfo {};
	viewportInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.viewportCount = 1;
	viewportInfo.pViewports    = &configInfo.viewport;
	viewportInfo.scissorCount  = 1;
	viewportInfo.pScissors     = &configInfo.scissor;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount                   = 2;
	pipelineInfo.pStages                      = shaderStages;
	pipelineInfo.pVertexInputState            = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState          = &configInfo.inputAssemblyInfo;
	pipelineInfo.pViewportState               = &viewportInfo;
	pipelineInfo.pRasterizationState          = &configInfo.rasterizationInfo;
	pipelineInfo.pMultisampleState            = &configInfo.multisampleInfo;
	pipelineInfo.pColorBlendState             = &configInfo.colorBlendInfo;
	pipelineInfo.pDynamicState                = nullptr;
	pipelineInfo.pDepthStencilState           = &configInfo.depthStencilInfo;

	pipelineInfo.layout     = configInfo.pipelineLayout;
	pipelineInfo.renderPass = configInfo.renderPass;
	pipelineInfo.subpass    = configInfo.subpass;

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex  = -1;

	if(vkCreateGraphicsPipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) { throw std::runtime_error("failed to create graphics pipeline!"); }
	vkDestroyShaderModule(m_Device.GetDevice(), vertexShaderModule, nullptr);
	vkDestroyShaderModule(m_Device.GetDevice(), fragmentShaderModule, nullptr);
}

void Pipeline::CreatePipelineLayout(Device& device, std::vector<VkDescriptorSetLayout>& descriptorSetsLayouts, VkPipelineLayout& pipelineLayout, VkPushConstantRange* pushConstants) {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
	pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount         = (uint32_t) descriptorSetsLayouts.size();
	pipelineLayoutInfo.pSetLayouts            = descriptorSetsLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = (pushConstants == nullptr) ? 0 : 1;
	pipelineLayoutInfo.pPushConstantRanges    = pushConstants;
	if(vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) { throw std::runtime_error("failed to create pipeline layout!"); }
}