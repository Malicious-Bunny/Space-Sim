#pragma once
#include "camera.h"
#include "input.h"
#include "object.h"
#include "renderer.h"
#include "vulkan/descriptors.h"
#include "vulkan/device.h"
#include "vulkan/skybox.h"
#include "vulkan/uniform.h"
#include "vulkan/window.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

struct Sync {
	std::condition_variable conditionVar;
	bool isGameLogicFinished = false;
	bool isRenderingFinished = false;
	bool stop                = false;
	bool canClose            = false;
	int cpuFramesAhead       = 0;
	std::mutex mutex;
};

class Application {
public:
	Application();
	~Application();

	Window m_Window {1600, 900, "Space Sim"};
	Device m_Device {m_Window};
	std::unique_ptr<Renderer> m_Renderer;
	FrameInfo m_FrameInfo {};
	FrameInfo m_FrameInfoCopy {};
	void Start();

private:
	void LoadGameObjects();
	void Update(const FrameInfo& frameInfo);

	void Run(Sync& syncObj);
	void Render(Sync& syncObj);

	void RenderImGui(VkCommandBuffer& commandBuffer);

	Camera m_Camera {};

	std::unique_ptr<DescriptorPool> m_GlobalPool {};
	Map m_GameObjects;
	Map m_Stars;

	Sampler m_Sampler {m_Device};

	std::shared_ptr<Object> m_Spaceship;
	std::shared_ptr<Object> m_LightSphere;
	Skybox m_Skybox {m_Device, "../../assets/textures/stars"};

	float m_SpaceshipRotationX = 0;
	float m_SpaceshipRotationY = 0;
	float m_SpaceshipRotationZ = 180;
};