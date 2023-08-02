#include "device.h"

#include "GLFW/glfw3.h"

#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <unordered_set>

/*
   *  @brief Callback function for Vulkan to be called by validation layers when needed
*/
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	if(messageType == 0x00000001)
		std::cerr << "Validation Layer: Info"
				  << "\n\t" << pCallbackData->pMessage << std::endl;
	if(messageType == 0x00000002)
		std::cerr << "Validation Layer: Validation Error"
				  << "\n\t" << pCallbackData->pMessage << std::endl;
	if(messageType == 0x00000004)
		std::cerr << "Validation Layer: Performance Issue (Not Optimal)"
				  << "\n\t" << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

/**
   *  @brief This is a proxy function. 
   *  It loads vkCreateDebugUtilsMessengerEXT from memory and then calls it.
   *  This is necessary because this function is an extension function, it is not automatically loaded to memory
*/
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if(func != nullptr) { return func(instance, pCreateInfo, pAllocator, pDebugMessenger); }
	else { return VK_ERROR_EXTENSION_NOT_PRESENT; }
}

/**
   *  @brief This is a proxy function. 
   *  It loads vkDestroyDebugUtilsMessengerEXT from memory and then calls it.
   *  This is necessary because this function is an extension function, it is not automatically loaded to memory
*/
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if(func != nullptr) { func(instance, debugMessenger, pAllocator); }
}

Device::Device(Window& window): m_Window {window} {
	CreateInstance();
	SetupDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateCommandPool();
}

Device::~Device() {
	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
	vkDestroyDevice(m_Device, nullptr);
	if(m_EnableValidationLayers) { DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr); }

	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
	vkDestroyInstance(m_Instance, nullptr);
}

bool Device::CheckValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for(const char* layerName : m_ValidationLayers) {
		bool layerFound = false;

		for(const auto& layerProperties : availableLayers) {
			if(strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if(!layerFound) { return false; }
	}

	return true;
}

std::vector<const char*> Device::GetRequiredGlfwExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if(m_EnableValidationLayers) { extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }

	return extensions;
}

void Device::CheckRequiredGlfwExtensions() {
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	//std::cout << "Available Extensions:" << std::endl;
	std::unordered_set<std::string> available;
	for(const auto& extension : extensions) {
		//std::cout << "\t" << extension.extensionName << std::endl;
		available.insert(extension.extensionName);
	}

	//std::cout << "Required Extensions:" << std::endl;
	auto requiredExtensions = GetRequiredGlfwExtensions();
	for(const auto& required : requiredExtensions) {
		//std::cout << "\t" << required << std::endl;
		if(available.find(required) == available.end()) { throw std::runtime_error("Missing required GLFW extension"); }
	}
}

void Device::SetupDebugMessenger() {
	if(!m_EnableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo {};
	PopulateDebugMessenger(createInfo);

	if(CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) { throw std::runtime_error("Failed to setup debug messenger!"); }
}

/**
   *  @brief Sets which messages to show by validation layers
   *
   *  @param createInfo struct to fill
*/
void Device::PopulateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo                 = {};
	createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
}

void Device::CreateInstance() {
	if(m_EnableValidationLayers && !CheckValidationLayerSupport()) { throw std::runtime_error("Validation layers requested but not avaiable!"); }

	VkApplicationInfo appInfo {};
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = "SpaceSim";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = "No Engine";
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion         = VK_API_VERSION_1_2;

	VkInstanceCreateInfo createInfo {};
	createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};    // placed outside to ensure that it is not destroyed before vkCreateInstance call
	if(m_EnableValidationLayers) {
		createInfo.enabledLayerCount   = (uint32_t) m_ValidationLayers.size();
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

		PopulateDebugMessenger(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateFlagsEXT*) &debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext             = nullptr;
	}

	auto glfwExtensions                = GetRequiredGlfwExtensions();
	createInfo.enabledExtensionCount   = (uint32_t) glfwExtensions.size();
	createInfo.ppEnabledExtensionNames = glfwExtensions.data();

	if(vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) { throw std::runtime_error("failed to create instance!"); }

	CheckRequiredGlfwExtensions();
}

QueueFamilyIndices Device::FindQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for(const auto& queueFamily : queueFamilies) {
		if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily         = i;
			indices.graphicsFamilyHasValue = true;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
		if(presentSupport) {
			indices.presentFamily         = i;
			indices.presentFamilyHasValue = true;
		}
		if(indices.IsComplete()) { break; }

		i++;
	}

	return indices;
}

bool Device::IsDeviceSuitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = FindQueueFamilies(device);

	bool extensionSupported = CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if(extensionSupported) {
		SwapchainSupportDetails swapChainSupport = QuerySwapchainSupport(device);
		swapChainAdequate                        = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.IsComplete() && extensionSupported && swapChainAdequate;
}

void Device::PickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
	if(deviceCount == 0) { throw std::runtime_error("failed to find GPUs with Vulkan support!"); }
	std::cout << "Number of devices: " << deviceCount << std::endl;
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

	for(const auto& device : devices) {
		if(IsDeviceSuitable(device)) {
			m_PhysicalDevice = device;
			break;
		}
	}

	if(m_PhysicalDevice == VK_NULL_HANDLE) { throw std::runtime_error("failed to find a suitable GPU!"); }

	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_Properties);
	std::cout << "physical device: " << m_Properties.deviceName << std::endl;
}

void Device::CreateLogicalDevice() {
	QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

	float queuePriority = 1.0f;
	for(uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex        = queueFamily;
		queueCreateInfo.queueCount              = 1;
		queueCreateInfo.pQueuePriorities        = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy        = VK_TRUE;

	VkDeviceCreateInfo createInfo      = {};
	createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos       = queueCreateInfos.data();
	createInfo.pEnabledFeatures        = &deviceFeatures;
	createInfo.enabledExtensionCount   = (uint32_t) m_DeviceExtensions.size();
	createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

	if(m_EnableValidationLayers) {
		createInfo.enabledLayerCount   = static_cast<uint32_t>(m_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
	}
	else { createInfo.enabledLayerCount = 0; }

	if(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS) { throw std::runtime_error("failed to create logical device!"); }

	vkGetDeviceQueue(m_Device, indices.graphicsFamily, 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, indices.graphicsFamily, 0, &m_PresentQueue);
}

void Device::CreateSurface() { m_Window.CreateWindowSurface(m_Instance, &m_Surface); }

bool Device::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

	for(const auto& extension : availableExtensions) { requiredExtensions.erase(extension.extensionName); }

	return requiredExtensions.empty();
}

SwapchainSupportDetails Device::QuerySwapchainSupport(VkPhysicalDevice device) {
	SwapchainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
	if(formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
	if(presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkFormat Device::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for(VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

		if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) { return format; }
		else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) { return format; }
	}
	throw std::runtime_error("failed to find supported format!");
}

void Device::CreateCommandPool() {
	QueueFamilyIndices queueFamilyIndices = FindPhysicalQueueFamilies();

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex        = queueFamilyIndices.graphicsFamily;
	poolInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) { throw std::runtime_error("failed to create command pool!"); }
}

uint32_t Device::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	/*
        The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps. 
        Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM 
        for when VRAM runs out. The different types of memory exist within these heaps. 
        Right now we'll only concern ourselves with the type of memory and not the heap it comes 
        from, but you can imagine that this can affect performance.
    */
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
	for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		/*
            The typeFilter parameter will be used to specify the bit field of memory 
            types that are suitable. That means that we can find the index of a suitable 
            memory type by simply iterating over them and checking if the corresponding bit is set to 1.
        */
		/*
            We also need to be able to write our vertex data to that memory. 
            The memoryTypes array consists of VkMemoryType structs that specify 
            the heap and properties of each type of memory. The properties define 
            special features of the memory, like being able to map it so we can write to it from the CPU.
            So we should check if the result of the bitwise AND is equal to the desired properties bit field.
        */
		if((typeFilter & (1 << i)) && ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)) { return i; }
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void Device::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferInfo {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size  = size;
	bufferInfo.usage = usage;
	// Just like the images in the swap chain, buffers can also be owned
	// by a specific queue family or be shared between multiple at the same time.
	// The buffer will only be used from the graphics queue, so we can stick to exclusive access.
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) { throw std::runtime_error("failed to create vertex buffer!"); }

	/**  
     * The VkMemoryRequirements struct has three fields:
            size: The size of the required amount of memory in bytes, may differ from bufferInfo.size.
            alignment: The offset in bytes where the buffer begins in the allocated region of memory, depends on bufferInfo.usage and bufferInfo.flags.
            memoryTypeBits: Bit field of the memory types that are suitable for the buffer.

        Graphics cards can offer different types of memory to allocate from. 
        Each type of memory varies in terms of allowed operations and performance 
        characteristics. We need to combine the requirements of the buffer and
        our own application requirements to find the right type of memory to use.
    */

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo {};
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if(vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) { throw std::runtime_error("failed to allocate vertex buffer memory!"); }

	/*
        If memory allocation was successful, then we can now associate this memory with the buffer using vkBindBufferMemory
    */
	vkBindBufferMemory(m_Device, buffer, bufferMemory, 0);
}

void Device::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer;
	BeginSingleTimeCommands(commandBuffer);

	VkBufferCopy copyRegion {};
	copyRegion.srcOffset = 0;    // Optional
	copyRegion.dstOffset = 0;    // Optional
	copyRegion.size      = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

void Device::BeginSingleTimeCommands(VkCommandBuffer& buffer) {
	VkCommandBufferAllocateInfo allocInfo {};
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool        = m_CommandPool;
	allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(m_Device, &allocInfo, &buffer);

	VkCommandBufferBeginInfo beginInfo {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(buffer, &beginInfo);
}

void Device::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo {};
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &commandBuffer;

	vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_GraphicsQueue);

	vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
}