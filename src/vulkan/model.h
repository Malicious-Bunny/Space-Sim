#pragma once

#include "../vulkan/buffer.h"
#include "../vulkan/device.h"
#include "../vulkan/image.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Model {
public:
	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;

		static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

		bool operator==(const Vertex& other) const { return position == other.position && normal == other.normal && texCoord == other.texCoord; }
	};

	struct Builder {
		std::vector<Vertex> vertices {};
		std::vector<uint32_t> indices;

		void LoadModel(const std::string& modelFilepath);
	};

	Model(Device& device, const Model::Builder& builder);
	~Model();

	Model(const Model&)            = delete;
	Model& operator=(const Model&) = delete;

	static std::unique_ptr<Model> CreateModelFromFile(Device& device, const std::string& modelFilepath);

	void Bind(VkCommandBuffer commandBuffer);
	void Draw(VkCommandBuffer commandBuffer);

	void UpdateVertexBuffer(VkCommandBuffer cmd, Buffer* buffer, const std::vector<Vertex>& vertices);

	inline Buffer* GetVertexBuffer() { return m_VertexBuffer.get(); }

private:
	void CreateVertexBuffer(const std::vector<Vertex>& vertices);
	void CreateIndexBuffer(const std::vector<uint32_t>& indices);

	Device& m_Device;

	std::unique_ptr<Buffer> m_VertexBuffer;
	uint32_t m_VertexCount;

	bool m_HasIndexBuffer = false;
	std::unique_ptr<Buffer> m_IndexBuffer;
	uint32_t m_IndexCount;
};