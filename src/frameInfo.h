#pragma once

#include "camera.h"
#include "object.h"
#include "vulkan/descriptors.h"
#include "vulkan/sampler.h"
#include "vulkan/skybox.h"

#include <memory>
#include <unordered_map>
#include <vulkan/vulkan.h>

using Map = std::unordered_map<int, std::shared_ptr<Object>>;

struct FrameInfo {
	VkCommandBuffer commandBuffer;
	Camera camera;
	VkDescriptorSet globalDescriptorSet;
	VkDescriptorSet lightsDescriptorSet;
	Skybox* skybox;
	VkDescriptorSet skyboxDescriptorSet;
	Map gameObjects;
};