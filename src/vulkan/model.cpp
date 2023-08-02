#include "model.h"

#include "../utilities.h"

#include <cstring>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <iostream>

namespace std {
	template <> struct hash<Model::Vertex> {
		size_t operator()(Model::Vertex const& vertex) const {
			size_t seed = 0;
			HashCombine(seed, vertex.position);
			return seed;
		}
	};
}    // namespace std

Model::Model(Device& device, const Model::Builder& builder): m_Device(device) {
	CreateVertexBuffer(builder.vertices);
	CreateIndexBuffer(builder.indices);
}

Model::~Model() {}

void Model::CreateVertexBuffer(const std::vector<Vertex>& vertices) {
	m_VertexCount           = static_cast<uint32_t>(vertices.size());
	VkDeviceSize bufferSize = sizeof(vertices[0]) * m_VertexCount;
	uint32_t vertexSize     = sizeof(vertices[0]);

	/*
        We need to be able to write our vertex data to memory.
        This property is indicated with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property.
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT is Used to get memory heap that is host coherent. 
        We use this to copy the data into the buffer memory immediately.
    */
	Buffer stagingBuffer(m_Device, vertexSize, m_VertexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	/*
        When buffer is created It is time to copy the vertex data to the buffer. 
        This is done by mapping the buffer memory into CPU accessible memory with vkMapMemory.
    */
	stagingBuffer.Map();
	stagingBuffer.WriteToBuffer((void*) vertices.data());

	/*
        The vertexBuffer is now allocated from a memory type that is device 
        local, which generally means that we're not able to use vkMapMemory. 
        However, we can copy data from the stagingBuffer to the vertexBuffer. 
        We have to indicate that we intend to do that by specifying the transfer 
        source flag(VK_BUFFER_USAGE_TRANSFER_SRC_BIT) for the stagingBuffer 
        and the transfer destination flag(VK_BUFFER_USAGE_TRANSFER_DST_BIT)
        for the vertexBuffer, along with the vertex buffer usage flag.
    */

	m_VertexBuffer = std::make_unique<Buffer>(m_Device, vertexSize, m_VertexCount, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_Device.CopyBuffer(stagingBuffer.GetBuffer(), m_VertexBuffer->GetBuffer(), bufferSize);
}

void Model::CreateIndexBuffer(const std::vector<uint32_t>& indices) {
	m_IndexCount     = static_cast<uint32_t>(indices.size());
	m_HasIndexBuffer = m_IndexCount > 0;
	if(!m_HasIndexBuffer) { return; }

	VkDeviceSize bufferSize = sizeof(indices[0]) * m_IndexCount;
	uint32_t indexSize      = sizeof(indices[0]);

	/*
        We need to be able to write our index data to memory.
        This property is indicated with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property.
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT is Used to get memory heap that is host coherent. 
        We use this to copy the data into the buffer memory immediately.
    */
	Buffer stagingBuffer(m_Device, indexSize, m_IndexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	/*
        When buffer is created It is time to copy the index data to the buffer. 
        This is done by mapping the buffer memory into CPU accessible memory with vkMapMemory.
    */
	stagingBuffer.Map();
	stagingBuffer.WriteToBuffer((void*) indices.data());

	/*
        The IndexBuffer is now allocated from a memory type that is device 
        local, which generally means that we're not able to use vkMapMemory. 
        However, we can copy data from the stagingBuffer to the IndexBuffer. 
        We have to indicate that we intend to do that by specifying the transfer 
        source flag(VK_BUFFER_USAGE_TRANSFER_SRC_BIT) for the stagingBuffer 
        and the transfer destination flag(VK_BUFFER_USAGE_TRANSFER_DST_BIT)
        for the IndexBuffer, along with the IndexBuffer usage flag.
    */
	m_IndexBuffer = std::make_unique<Buffer>(m_Device, indexSize, m_IndexCount, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_Device.CopyBuffer(stagingBuffer.GetBuffer(), m_IndexBuffer->GetBuffer(), bufferSize);
}

/**
    @brief Binds vertex and index buffers
*/
void Model::Bind(VkCommandBuffer commandBuffer) {
	VkBuffer buffers[]     = {m_VertexBuffer->GetBuffer()};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

	if(m_HasIndexBuffer) { vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32); }
}

void Model::Draw(VkCommandBuffer commandBuffer) {
	if(m_HasIndexBuffer) { vkCmdDrawIndexed(commandBuffer, m_IndexCount, 1, 0, 0, 0); }
	else { vkCmdDraw(commandBuffer, m_VertexCount, 1, 0, 0); }
}

/**
 * @brief Specifies how many vertex buffers we wish to bind to our pipeline. In this case there is only one with all data packed inside it
*/
std::vector<VkVertexInputBindingDescription> Model::Vertex::GetBindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescription(1);
	bindingDescription[0].binding   = 0;
	bindingDescription[0].stride    = sizeof(Vertex);
	bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

/**
 * @brief Specifies layout of data inside vertex buffer
*/
std::vector<VkVertexInputAttributeDescription> Model::Vertex::GetAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions {};
	attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
	attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
	attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)});

	return attributeDescriptions;
}

std::unique_ptr<Model> Model::CreateModelFromFile(Device& device, const std::string& modelFilepath) {
	Builder builder {};
	builder.LoadModel(modelFilepath);

	return std::make_unique<Model>(device, builder);
}

void Model::Builder::LoadModel(const std::string& modelFilepath) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	std::string mtlPath = modelFilepath;
	size_t lastSlashPos = mtlPath.find_last_of('/');
	if(lastSlashPos != std::string::npos) {
		// Erase everything after the last '/'
		mtlPath.erase(lastSlashPos);
	}
	if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelFilepath.c_str(), mtlPath.c_str())) { throw std::runtime_error(warn + err); }
	std::cout << warn << std::endl;

	vertices.clear();
	indices.clear();

	std::unordered_map<Vertex, uint32_t> uniqueVertices {};
	for(const auto& shape : shapes) {
		for(const auto& index : shape.mesh.indices) {
			Vertex vertex {};

			if(index.vertex_index >= 0) {
				vertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
				};
			}

			if(index.texcoord_index >= 0) { vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]}; }

			if(index.normal_index >= 0) {
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
				};
			}

			// Check if vertex is in hashmap already, if yes just add it do indices. We're saving a lot of memory this way
			if(uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

void Model::UpdateVertexBuffer(VkCommandBuffer cmd, Buffer* buffer, const std::vector<Vertex>& vertices) {
	vkCmdUpdateBuffer(cmd, buffer->GetBuffer(), 0, sizeof(vertices[0]) * vertices.size(), vertices.data());
}