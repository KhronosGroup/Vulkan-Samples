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

#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "entity.h"
#include "vulkan_device.h"

/**
 * @brief Structure for uniform buffer object.
 */
struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec4 lightPos;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec4 viewPos;
};

class CameraComponent;

/**
 * @brief Class for managing Vulkan descriptor sets and layouts.
 */
class DescriptorManager
{
  public:
	// Entity resources
	struct EntityResources
	{
		std::vector<vk::raii::Buffer>        uniformBuffers;
		std::vector<vk::raii::DeviceMemory>  uniformBuffersMemory;
		std::vector<void *>                  uniformBuffersMapped;
		std::vector<vk::raii::DescriptorSet> descriptorSets;
	};

	/**
	 * @brief Constructor.
	 * @param device The Vulkan device.
	 */
	DescriptorManager(VulkanDevice &device);

	/**
	 * @brief Destructor.
	 */
	~DescriptorManager();

	/**
	 * @brief Create the descriptor pool.
	 * @param maxSets The maximum number of descriptor sets.
	 * @return True if the descriptor pool was created successfully, false otherwise.
	 */
	bool createDescriptorPool(uint32_t maxSets);

	/**
	 * @brief Create uniform buffers for an entity.
	 * @param entity The entity.
	 * @param maxFramesInFlight The maximum number of frames in flight.
	 * @return True if the uniform buffers were created successfully, false otherwise.
	 */
	bool createUniformBuffers(Entity *entity, uint32_t maxFramesInFlight);
	bool update_descriptor_sets(Entity *entity, uint32_t maxFramesInFlight, bool &value1);

	/**
	 * @brief Create descriptor sets for an entity.
	 * @param entity The entity.
	 * @param texturePath The texture path.
	 * @param descriptorSetLayout The descriptor set layout.
	 * @param maxFramesInFlight The maximum number of frames in flight.
	 * @return True if the descriptor sets were created successfully, false otherwise.
	 */
	bool createDescriptorSets(Entity *entity, const std::string &texturePath, vk::DescriptorSetLayout descriptorSetLayout, uint32_t maxFramesInFlight);

	/**
	 * @brief Update uniform buffer for an entity.
	 * @param currentImage The current image index.
	 * @param entity The entity.
	 * @param camera The camera.
	 */
	void updateUniformBuffer(uint32_t currentImage, Entity *entity, CameraComponent *camera);

	/**
	 * @brief Get the descriptor pool.
	 * @return The descriptor pool.
	 */
	vk::raii::DescriptorPool &getDescriptorPool()
	{
		return descriptorPool;
	}

	/**
	 * @brief Get the entity resources.
	 * @return The entity resources.
	 */
	const std::unordered_map<Entity *, EntityResources> &getEntityResources()
	{
		return entityResources;
	}

	/**
	 * @brief Get the resources for an entity.
	 * @param entity The entity.
	 * @return The entity resources.
	 */
	const EntityResources &getEntityResources(Entity *entity)
	{
		return entityResources[entity];
	}

  private:
	// Vulkan device
	VulkanDevice &device;

	// Descriptor pool
	vk::raii::DescriptorPool                      descriptorPool = nullptr;
	std::unordered_map<Entity *, EntityResources> entityResources;

	// Helper functions
	std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
};
