/* Copyright (c) 2025 Holochip Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <array>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "component.h"

/**
 * @brief Structure representing per-instance data for instanced rendering.
 * Using explicit float vectors instead of matrices for better control over GPU data layout.
 */
struct InstanceData
{
	// Model matrix as glm::mat4 (4x4)
	glm::mat4 modelMatrix{};

	// Normal matrix as glm::mat3x4 (3 columns of vec4: xyz = normal matrix columns, w unused)
	glm::mat3x4 normalMatrix{};

	InstanceData()
	{
		// Initialize as identity matrices
		modelMatrix     = glm::mat4(1.0f);
		normalMatrix[0] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		normalMatrix[1] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
		normalMatrix[2] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
	}

	explicit InstanceData(const glm::mat4 &transform, uint32_t matIndex = 0)
	{
		// Store model matrix directly
		modelMatrix = transform;

		// Calculate normal matrix (inverse transpose of upper-left 3x3)
		glm::mat3 normalMat3 = glm::transpose(glm::inverse(glm::mat3(transform)));
		normalMatrix[0]      = glm::vec4(normalMat3[0], 0.0f);
		normalMatrix[1]      = glm::vec4(normalMat3[1], 0.0f);
		normalMatrix[2]      = glm::vec4(normalMat3[2], 0.0f);

		// Note: matIndex parameter ignored since materialIndex field was removed
	}

	// Helper methods for backward compatibility
	[[nodiscard]] glm::mat4 getModelMatrix() const
	{
		return modelMatrix;
	}

	void setModelMatrix(const glm::mat4 &matrix)
	{
		modelMatrix = matrix;

		// Also update normal matrix when model matrix changes
		glm::mat3 normalMat3 = glm::transpose(glm::inverse(glm::mat3(matrix)));
		normalMatrix[0]      = glm::vec4(normalMat3[0], 0.0f);
		normalMatrix[1]      = glm::vec4(normalMat3[1], 0.0f);
		normalMatrix[2]      = glm::vec4(normalMat3[2], 0.0f);
	}

	[[nodiscard]] glm::mat3 getNormalMatrix() const
	{
		return {
		    glm::vec3(normalMatrix[0]),
		    glm::vec3(normalMatrix[1]),
		    glm::vec3(normalMatrix[2])};
	}

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		constexpr vk::VertexInputBindingDescription bindingDescription(
		    1,                                    // binding (binding 1 for instance data)
		    sizeof(InstanceData),                 // stride
		    vk::VertexInputRate::eInstance        // inputRate
		);
		return bindingDescription;
	}

	static std::array<vk::VertexInputAttributeDescription, 7> getAttributeDescriptions()
	{
		constexpr uint32_t                                           modelBase             = offsetof(InstanceData, modelMatrix);
		constexpr uint32_t                                           normalBase            = offsetof(InstanceData, normalMatrix);
		constexpr uint32_t                                           vec4Size              = sizeof(glm::vec4);
		constexpr std::array<vk::VertexInputAttributeDescription, 7> attributeDescriptions = {
		    // Model matrix columns (locations 4-7)
		    vk::VertexInputAttributeDescription{
		        .location = 4,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = modelBase + 0u * vec4Size},
		    vk::VertexInputAttributeDescription{
		        .location = 5,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = modelBase + 1u * vec4Size},
		    vk::VertexInputAttributeDescription{
		        .location = 6,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = modelBase + 2u * vec4Size},
		    vk::VertexInputAttributeDescription{
		        .location = 7,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = modelBase + 3u * vec4Size},
		    // Normal matrix columns (locations 8-10)
		    vk::VertexInputAttributeDescription{
		        .location = 8,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = normalBase + 0u * vec4Size},
		    vk::VertexInputAttributeDescription{
		        .location = 9,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = normalBase + 1u * vec4Size},
		    vk::VertexInputAttributeDescription{
		        .location = 10,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = normalBase + 2u * vec4Size}};
		return attributeDescriptions;
	}

	// Get all attribute descriptions for model matrix (4 vec4s)
	static std::array<vk::VertexInputAttributeDescription, 4> getModelMatrixAttributeDescriptions()
	{
		constexpr uint32_t                                           modelBase             = offsetof(InstanceData, modelMatrix);
		constexpr uint32_t                                           vec4Size              = sizeof(glm::vec4);
		constexpr std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions = {
		    vk::VertexInputAttributeDescription{
		        .location = 4,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = modelBase + 0u * vec4Size},
		    vk::VertexInputAttributeDescription{
		        .location = 5,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = modelBase + 1u * vec4Size},
		    vk::VertexInputAttributeDescription{
		        .location = 6,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = modelBase + 2u * vec4Size},
		    vk::VertexInputAttributeDescription{
		        .location = 7,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = modelBase + 3u * vec4Size}};
		return attributeDescriptions;
	}

	// Get all attribute descriptions for normal matrix (3 vec4s)
	static std::array<vk::VertexInputAttributeDescription, 3> getNormalMatrixAttributeDescriptions()
	{
		constexpr uint32_t                                           normalBase            = offsetof(InstanceData, normalMatrix);
		constexpr uint32_t                                           vec4Size              = sizeof(glm::vec4);
		constexpr std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = {
		    vk::VertexInputAttributeDescription{
		        .location = 8,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = normalBase + 0u * vec4Size},
		    vk::VertexInputAttributeDescription{
		        .location = 9,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = normalBase + 1u * vec4Size},
		    vk::VertexInputAttributeDescription{
		        .location = 10,
		        .binding  = 1,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = normalBase + 2u * vec4Size}};
		return attributeDescriptions;
	}
};

/**
 * @brief Structure representing a vertex in a mesh.
 */
struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 tangent;

	bool operator==(const Vertex &other) const
	{
		return position == other.position &&
		       normal == other.normal &&
		       texCoord == other.texCoord &&
		       tangent == other.tangent;
	}

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		constexpr vk::VertexInputBindingDescription bindingDescription(
		    0,                                  // binding
		    sizeof(Vertex),                     // stride
		    vk::VertexInputRate::eVertex        // inputRate
		);
		return bindingDescription;
	}

	static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions()
	{
		constexpr std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions = {
		    vk::VertexInputAttributeDescription{
		        .location = 0,
		        .binding  = 0,
		        .format   = vk::Format::eR32G32B32Sfloat,
		        .offset   = offsetof(Vertex, position)},
		    vk::VertexInputAttributeDescription{
		        .location = 1,
		        .binding  = 0,
		        .format   = vk::Format::eR32G32B32Sfloat,
		        .offset   = offsetof(Vertex, normal)},
		    vk::VertexInputAttributeDescription{
		        .location = 2,
		        .binding  = 0,
		        .format   = vk::Format::eR32G32Sfloat,
		        .offset   = offsetof(Vertex, texCoord)},
		    vk::VertexInputAttributeDescription{
		        .location = 3,
		        .binding  = 0,
		        .format   = vk::Format::eR32G32B32A32Sfloat,
		        .offset   = offsetof(Vertex, tangent)}};
		return attributeDescriptions;
	}
};

/**
 * @brief Component that handles the mesh data for rendering.
 */
class MeshComponent final : public Component
{
  private:
	std::vector<Vertex>   vertices;
	std::vector<uint32_t> indices;

	// Cached local-space AABB
	glm::vec3 localAABBMin{0.0f};
	glm::vec3 localAABBMax{0.0f};
	bool      localAABBValid = false;

	// All PBR texture paths for this mesh
	std::string texturePath;                         // Primary texture path (baseColor) - kept for backward compatibility
	std::string baseColorTexturePath;                // Base color (albedo) texture
	std::string normalTexturePath;                   // Normal map texture
	std::string metallicRoughnessTexturePath;        // Metallic-roughness texture
	std::string occlusionTexturePath;                // Ambient occlusion texture
	std::string emissiveTexturePath;                 // Emissive texture

	// Instancing support
	std::vector<InstanceData> instances;                  // Instance data for instanced rendering
	bool                      isInstanced = false;        // Flag to indicate if this mesh uses instancing

	// The renderer will manage Vulkan resources
	// This component only stores the data

  public:
	/**
	 * @brief Constructor with an optional name.
	 * @param componentName The name of the component.
	 */
	explicit MeshComponent(const std::string &componentName = "MeshComponent") :
	    Component(componentName)
	{}

	// Local AABB utilities
	void RecomputeLocalAABB()
	{
		if (vertices.empty())
		{
			localAABBMin   = glm::vec3(0.0f);
			localAABBMax   = glm::vec3(0.0f);
			localAABBValid = false;
			return;
		}
		glm::vec3 minB = vertices[0].position;
		glm::vec3 maxB = vertices[0].position;
		for (const auto &v : vertices)
		{
			minB = glm::min(minB, v.position);
			maxB = glm::max(maxB, v.position);
		}
		localAABBMin   = minB;
		localAABBMax   = maxB;
		localAABBValid = true;
	}
	[[nodiscard]] bool HasLocalAABB() const
	{
		return localAABBValid;
	}
	[[nodiscard]] glm::vec3 GetLocalAABBMin() const
	{
		return localAABBMin;
	}
	[[nodiscard]] glm::vec3 GetLocalAABBMax() const
	{
		return localAABBMax;
	}

	/**
	 * @brief Set the vertices of the mesh.
	 * @param newVertices The new vertices.
	 */
	void SetVertices(const std::vector<Vertex> &newVertices)
	{
		vertices = newVertices;
		RecomputeLocalAABB();
	}

	/**
	 * @brief Get the vertices of the mesh.
	 * @return The vertices.
	 */
	[[nodiscard]] const std::vector<Vertex> &GetVertices() const
	{
		return vertices;
	}

	/**
	 * @brief Set the indices of the mesh.
	 * @param newIndices The new indices.
	 */
	void SetIndices(const std::vector<uint32_t> &newIndices)
	{
		indices = newIndices;
	}

	/**
	 * @brief Get the indices of the mesh.
	 * @return The indices.
	 */
	[[nodiscard]] const std::vector<uint32_t> &GetIndices() const
	{
		return indices;
	}

	/**
	 * @brief Set the texture path for the mesh.
	 * @param path The path to the texture file.
	 */
	void SetTexturePath(const std::string &path)
	{
		texturePath          = path;
		baseColorTexturePath = path;        // Keep baseColor in sync for backward compatibility
	}

	/**
	 * @brief Get the texture path for the mesh.
	 * @return The path to the texture file.
	 */
	[[nodiscard]] const std::string &GetTexturePath() const
	{
		return texturePath;
	}

	// PBR texture path setters
	void SetBaseColorTexturePath(const std::string &path)
	{
		baseColorTexturePath = path;
	}
	void SetNormalTexturePath(const std::string &path)
	{
		normalTexturePath = path;
	}
	void SetMetallicRoughnessTexturePath(const std::string &path)
	{
		metallicRoughnessTexturePath = path;
	}
	void SetOcclusionTexturePath(const std::string &path)
	{
		occlusionTexturePath = path;
	}
	void SetEmissiveTexturePath(const std::string &path)
	{
		emissiveTexturePath = path;
	}

	// PBR texture path getters
	[[nodiscard]] const std::string &GetBaseColorTexturePath() const
	{
		return baseColorTexturePath;
	}
	[[nodiscard]] const std::string &GetNormalTexturePath() const
	{
		return normalTexturePath;
	}
	[[nodiscard]] const std::string &GetMetallicRoughnessTexturePath() const
	{
		return metallicRoughnessTexturePath;
	}
	[[nodiscard]] const std::string &GetOcclusionTexturePath() const
	{
		return occlusionTexturePath;
	}
	[[nodiscard]] const std::string &GetEmissiveTexturePath() const
	{
		return emissiveTexturePath;
	}

	/**
	 * @brief Create a simple sphere mesh.
	 * @param radius The radius of the sphere.
	 * @param color The color of the sphere.
	 * @param segments The number of segments (resolution).
	 */
	void CreateSphere(float radius = 1.0f, const glm::vec3 &color = glm::vec3(1.0f), int segments = 16);

	/**
	 * @brief Load mesh data from a Model.
	 * @param model Pointer to the model to load from.
	 */
	void LoadFromModel(const class Model *model);

	// Instancing methods

	/**
	 * @brief Add an instance with the given transform matrix.
	 * @param transform The transform matrix for this instance.
	 * @param materialIndex The material index for this instance (default: 0).
	 */
	void AddInstance(const glm::mat4 &transform, uint32_t materialIndex = 0)
	{
		instances.emplace_back(transform, materialIndex);
		isInstanced = instances.size() > 1;
	}

	/**
	 * @brief Set all instances at once.
	 * @param newInstances Vector of instance data.
	 */
	void SetInstances(const std::vector<InstanceData> &newInstances)
	{
		instances   = newInstances;
		isInstanced = instances.size() > 1;
	}

	/**
	 * @brief Get all instance data.
	 * @return Reference to the instances vector.
	 */
	[[nodiscard]] const std::vector<InstanceData> &GetInstances() const
	{
		return instances;
	}

	/**
	 * @brief Get the number of instances.
	 * @return Number of instances (0 if not instanced, >= 1 if instanced).
	 */
	[[nodiscard]] size_t GetInstanceCount() const
	{
		return instances.size();
	}

	/**
	 * @brief Check if this mesh uses instancing.
	 * @return True if instanced (more than 1 instance), false otherwise.
	 */
	[[nodiscard]] bool IsInstanced() const
	{
		return isInstanced;
	}

	/**
	 * @brief Clear all instances and disable instancing.
	 */
	void ClearInstances()
	{
		instances.clear();
		isInstanced = false;
	}

	/**
	 * @brief Update a specific instance's transform.
	 * @param index The index of the instance to update.
	 * @param transform The new transform matrix.
	 * @param materialIndex The new material index (optional).
	 */
	void UpdateInstance(size_t index, const glm::mat4 &transform, uint32_t materialIndex = 0)
	{
		if (index < instances.size())
		{
			instances[index] = InstanceData(transform, materialIndex);
		}
	}

	/**
	 * @brief Get a specific instance's data.
	 * @param index The index of the instance.
	 * @return Reference to the instance data, or first instance if the index is out of bounds.
	 */
	[[nodiscard]] const InstanceData &GetInstance(size_t index) const
	{
		if (index < instances.size())
		{
			return instances[index];
		}
		// Return the first instance or default if empty
		static const InstanceData defaultInstance;
		return instances.empty() ? defaultInstance : instances[0];
	}
};
