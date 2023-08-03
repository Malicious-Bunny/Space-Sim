#include "application.h"

#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "stbimage/stb_image.h"

#include <array>
#include <chrono>
#include <future>
#include <glm/glm.hpp>

// Timer for benchmarking
class Timer {
public:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_Start, m_End;

	Timer() { m_Start = std::chrono::high_resolution_clock::now(); }

	~Timer() {
		m_End           = std::chrono::high_resolution_clock::now();
		double duration = (m_End - m_Start).count() / 1000.0;
		std::cout << "Timer Took " << duration << "ms" << std::endl;
	}
};

struct GlobalUbo {
	glm::mat4 projectionView {1.0f};
	glm::mat4 lightMatrix {1.0f};
};

static const uint32_t MAX_LIGHTS = 2;

struct LightsUbo {
	glm::vec3 lightPositions[MAX_LIGHTS];
	alignas(16) glm::vec4 lightColors[MAX_LIGHTS];
	float numberOfLights;
};

Application::Application() {
	m_GlobalPool = DescriptorPool::Builder(m_Device)
	                   .SetMaxSets((Swapchain::MAX_FRAMES_IN_FLIGHT) *100)
	                   .SetPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
	                   .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (Swapchain::MAX_FRAMES_IN_FLIGHT) *100)
	                   .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, (Swapchain::MAX_FRAMES_IN_FLIGHT) *100)    // one for each figure
	                   .Build();
	LoadGameObjects();

	WindowInfo winInfo;
	winInfo.windowPtr  = m_Window.GetGLFWwindow();
	winInfo.windowSize = {m_Window.GetExtent().height, m_Window.GetExtent().width};
	Input::Instantiate(winInfo);
	Input::SetCallbacks();
	glfwSetInputMode(m_Window.GetGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//Renderer Creation
	m_Renderer = std::make_unique<Renderer>(m_Window, m_Device, m_GlobalPool->GetDescriptorPool());
}

Application::~Application() {
	
}

void Application::Start() {
	Sync syncObj;

	std::thread gameThread {[&]() { Run(syncObj); }};

	std::thread renderThread {[&]() { Render(syncObj); }};

	renderThread.join();
	gameThread.join();
}

void Application::Render(Sync& syncObj) {
	while(!syncObj.stop) {
		std::unique_lock<std::mutex> lock(syncObj.mutex);
		syncObj.conditionVar.wait(lock, [&]() { return syncObj.isGameLogicFinished; });
		syncObj.isRenderingFinished = false;
        // Pass RenderImgui function pointer because we need to render ImGui with valid command buffer and we don't have access to that from application
		m_Renderer->Render(m_FrameInfoCopy, [this](VkCommandBuffer& commandBuffer) { RenderImGui(commandBuffer); });
		syncObj.isGameLogicFinished = false;
		syncObj.cpuFramesAhead--;
		syncObj.conditionVar.notify_all();
		syncObj.isRenderingFinished = true;
	}
	std::unique_lock<std::mutex> lock(syncObj.mutex);
	syncObj.canClose = true;
	syncObj.conditionVar.notify_all();
}

void Application::Run(Sync& syncObj) {
	// GLOBAL UBO
	std::vector<Binding> globalUboBindings;
	globalUboBindings.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(GlobalUbo), 0, 0, VK_IMAGE_LAYOUT_UNDEFINED});
	std::vector<std::shared_ptr<Uniform>> globalUniforms;
	for(int i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++) { globalUniforms.push_back(std::make_shared<Uniform>(m_Device, globalUboBindings, *m_GlobalPool)); }

    // We could move that to renderer but maybe we'll want to change skybox from application side in the future
	// SKYBOX UBO
	std::vector<Binding> skyboxBindings;
	skyboxBindings.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, m_Skybox.GetCubemap().GetCubeMapImageSampler(), m_Skybox.GetCubemap().GetCubeMapImageView(),
	                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
	Uniform skyboxUniform(m_Device, skyboxBindings, *m_GlobalPool);

	// LIGHTS UBO
	std::vector<Binding> LightBindings;
	LightBindings.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(LightsUbo), 0, 0, VK_IMAGE_LAYOUT_UNDEFINED});
	Uniform LightsUniform(m_Device, LightBindings, *m_GlobalPool);

	// Frame Info struct creation
	m_FrameInfo.skybox              = &m_Skybox;
	m_FrameInfo.skyboxDescriptorSet = skyboxUniform.GetDescriptorSet();
	m_FrameInfo.lightsDescriptorSet = LightsUniform.GetDescriptorSet();

	m_Camera.SetPerspective(25.0f, m_Renderer->GetAspectRatio(), 1.0f, 100.0f);

	// Main Loop
	while(!m_Window.ShouldClose()) {
		m_Spaceship->GetObjectTransform().rotation.x = m_SpaceshipRotationX;
		m_Spaceship->GetObjectTransform().rotation.y = m_SpaceshipRotationY;
		m_Spaceship->GetObjectTransform().rotation.z = m_SpaceshipRotationZ;
		glfwPollEvents();
		Input::ProcessInput();

		// Fill FrameInfo struct
		int frameIndex                  = m_Renderer->GetFrameIndex();
		m_FrameInfo.camera              = m_Camera;
		m_FrameInfo.globalDescriptorSet = globalUniforms[frameIndex]->GetDescriptorSet();
		m_FrameInfo.gameObjects         = m_GameObjects;
		m_FrameInfo.stars 				= m_Stars;

		Update(m_FrameInfo);

		// Camera Update
		Input::GetInput(m_Camera);
		m_Camera.MoveCamera(Input::mouseX - m_Window.GetExtent().width / 2.0, Input::mouseY - m_Window.GetExtent().height / 2.0);

		// UBO update
		{
            {
                // global ubo
		        GlobalUbo ubo {};
                ubo.projectionView = m_Camera.GetProj() * m_Camera.GetView();
                ubo.lightMatrix    = glm::mat4(1.0f);    // for now we don't need that
                globalUniforms[frameIndex]->GetUboBuffer(0)->WriteToBuffer(&ubo);
                globalUniforms[frameIndex]->GetUboBuffer(0)->Flush();
            }
            {
                // lights ubo
                LightsUbo ubo;
                ubo.numberOfLights    = 1;
                ubo.lightColors[0]    = {1.0, 1.0, 1.0, 5.0f};
                ubo.lightPositions[0] = m_LightSphere->GetObjectTransform().translation - m_Camera.m_Translation;
                LightsUniform.GetUboBuffer(0)->WriteToBuffer(&ubo);
                LightsUniform.GetUboBuffer(0)->Flush();
            }
        }

        {
            std::unique_lock<std::mutex> lock(syncObj.mutex);
            syncObj.conditionVar.wait(lock, [&]() { return syncObj.cpuFramesAhead < 1; });
            syncObj.isGameLogicFinished = true;
            m_FrameInfoCopy             = m_FrameInfo;
            syncObj.conditionVar.notify_all();
            syncObj.cpuFramesAhead++;
        }
    }

    std::unique_lock<std::mutex> lock(syncObj.mutex);
    syncObj.conditionVar.wait(lock, [&]() { return syncObj.isRenderingFinished; });
    syncObj.stop                = true;
    syncObj.isGameLogicFinished = true;
    syncObj.conditionVar.notify_all();

    syncObj.conditionVar.wait(lock, [&]() { return syncObj.canClose; });
    vkDeviceWaitIdle(m_Device.GetDevice());
}

/**
 * @brief Updates game objects
 */
void Application::Update(const FrameInfo& frameInfo) {}

void Application::LoadGameObjects() {
	m_Sampler.CreateSimpleSampler();

	ObjectInfo objInfo;
	objInfo.descriptorPool = m_GlobalPool.get();
	objInfo.device         = &m_Device;
	objInfo.sampler        = &m_Sampler;

	// ----------------- Object Creation -----------------------

	{
		Transform objTransform {};
		objTransform.translation = { 0.0, 5.0, -10.0 };
		objTransform.rotation = { m_SpaceshipRotationX, m_SpaceshipRotationY, m_SpaceshipRotationZ };
		objTransform.scale       = glm::dvec3(1.0);

		m_Spaceship = std::make_shared<Object>(objInfo, objTransform, "../../assets/models/spaceship.obj", "../../assets/textures/spaceship_albedo.png", "../../assets/textures/spaceship_normal.png",
											"../../assets/textures/spaceship_metalic.png", "../../assets/textures/spaceship_roughness.png");
		m_GameObjects.emplace(m_Spaceship->GetObjectID(), m_Spaceship);
	}
	{
		Transform objTransform {};
		objTransform.translation = { 0.0, -1.0, -10.0 };
		objTransform.rotation = { 0.0, 0.0, 0.0 };
		objTransform.scale       = glm::dvec3(0.2);
		m_LightSphere = std::make_shared<Object>(objInfo, objTransform, "../../assets/models/sphere.obj", "../../assets/textures/empty_roughness.jpg");
		m_Stars.emplace(m_LightSphere->GetObjectID(), m_LightSphere);
	}
}

void Application::RenderImGui(VkCommandBuffer& commandBuffer) {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings", (bool*) false, 0);

	ImGui::Text("Spaceship rotation");
	ImGui::SliderFloat("Rotation X", &m_SpaceshipRotationX, 0.0f, 360.0f);
	ImGui::SliderFloat("Rotation y", &m_SpaceshipRotationY, 0.0f, 360.0f);
	ImGui::SliderFloat("Rotation z", &m_SpaceshipRotationZ, 0.0f, 360.0f);

	ImGui::End();

	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}