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
#include "descriptor_manager.h"
#include "camera_component.h"
#include "transform_component.h"
#include <array>
#include <cstring>
#include <iostream>

// Constructor
DescriptorManager::DescriptorManager(VulkanDevice &device) :
    device(device)
{
}

// Destructor
DescriptorManager::~DescriptorManager() = default;

// Create descriptor pool
bool DescriptorManager::createDescriptorPool(uint32_t maxSets)
{
	try
	{
		// Create descriptor pool sizes
		std::array<vk::DescriptorPoolSize, 2> poolSizes = {
		    vk::DescriptorPoolSize{
		        .type            = vk::DescriptorType::eUniformBuffer,
		        .descriptorCount = maxSets},
		    vk::DescriptorPoolSize{
		        .type            = vk::DescriptorType::eCombinedImageSampler,
		        .descriptorCount = maxSets}};

		// Create descriptor pool
		vk::DescriptorPoolCreateInfo poolInfo{
		    .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		    .maxSets       = maxSets,
		    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		    .pPoolSizes    = poolSizes.data()};

		descriptorPool = vk::raii::DescriptorPool(device.getDevice(), poolInfo);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create descriptor pool: " << e.what() << std::endl;
		return false;
	}
}

// Create uniform buffers for an entity
bool DescriptorManager::createUniformBuffers(Entity *entity, uint32_t maxFramesInFlight)
{
	try
	{
		// Create uniform buffers for each frame in flight
		vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

		// Create entity resources if they don't exist
		auto entityResourcesIt = entityResources.try_emplace(entity).first;

		// Clear existing uniform buffers
		entityResourcesIt->second.uniformBuffers.clear();
		entityResourcesIt->second.uniformBuffersMemory.clear();
		entityResourcesIt->second.uniformBuffersMapped.clear();

		// Create uniform buffers
		for (size_t i = 0; i < maxFramesInFlight; i++)
		{
			// Create buffer
			auto [buffer, bufferMemory] = createBuffer(
			    bufferSize,
			    vk::BufferUsageFlagBits::eUniformBuffer,
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

			// Map memory
			void *data = bufferMemory.mapMemory(0, bufferSize);

			// Store resources
			entityResourcesIt->second.uniformBuffers.push_back(std::move(buffer));
			entityResourcesIt->second.uniformBuffersMemory.push_back(std::move(bufferMemory));
			entityResourcesIt->second.uniformBuffersMapped.push_back(data);
		}

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create uniform buffers: " << e.what() << std::endl;
		return false;
	}
}

bool DescriptorManager::update_descriptor_sets(Entity *entity, uint32_t maxFramesInFlight, bool &value1)
{
	assert(entityResources[entity].uniformBuffers.size() == maxFramesInFlight);
	// Update descriptor sets
	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		// Create descriptor buffer info
		vk::DescriptorBufferInfo bufferInfo{
		    .buffer = *entityResources[entity].uniformBuffers[i],
		    .offset = 0,
		    .range  = sizeof(UniformBufferObject)};

		// Create descriptor image info
		vk::DescriptorImageInfo imageInfo{
		    // These would be set based on the texture resources
		    // .sampler = textureSampler,
		    // .imageView = textureImageView,
		    .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

		// Create descriptor writes
		std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {
		    vk::WriteDescriptorSet{
		        .dstSet           = entityResources[entity].descriptorSets[i],
		        .dstBinding       = 0,
		        .dstArrayElement  = 0,
		        .descriptorCount  = 1,
		        .descriptorType   = vk::DescriptorType::eUniformBuffer,
		        .pImageInfo       = nullptr,
		        .pBufferInfo      = &bufferInfo,
		        .pTexelBufferView = nullptr},
		    vk::WriteDescriptorSet{
		        .dstSet           = entityResources[entity].descriptorSets[i],
		        .dstBinding       = 1,
		        .dstArrayElement  = 0,
		        .descriptorCount  = 1,
		        .descriptorType   = vk::DescriptorType::eCombinedImageSampler,
		        .pImageInfo       = &imageInfo,
		        .pBufferInfo      = nullptr,
		        .pTexelBufferView = nullptr}};

		// Update descriptor sets
		device.getDevice().updateDescriptorSets(descriptorWrites, nullptr);
	}
	return false;
}
// Create descriptor sets for an entity
bool DescriptorManager::createDescriptorSets(Entity *entity, const std::string &texturePath, vk::DescriptorSetLayout descriptorSetLayout, uint32_t maxFramesInFlight)
{
	try
	{
		assert(entityResources.find(entity) != entityResources.end());
		// Create descriptor sets for each frame in flight
		std::vector<vk::DescriptorSetLayout> layouts(maxFramesInFlight, descriptorSetLayout);

		// Allocate descriptor sets
		vk::DescriptorSetAllocateInfo allocInfo{
		    .descriptorPool     = *descriptorPool,
		    .descriptorSetCount = static_cast<uint32_t>(maxFramesInFlight),
		    .pSetLayouts        = layouts.data()};

		entityResources[entity].descriptorSets = device.getDevice().allocateDescriptorSets(allocInfo);

		bool value1;
		if (update_descriptor_sets(entity, maxFramesInFlight, value1))
			return value1;

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create descriptor sets: " << e.what() << std::endl;
		return false;
	}
}

// Update uniform buffer for an entity
void DescriptorManager::updateUniformBuffer(uint32_t currentImage, Entity *entity, CameraComponent *camera)
{
	// Update uniform buffer with the latest data
	UniformBufferObject ubo{};

	// Get entity transform
	auto transform = entity->GetComponent<TransformComponent>();
	if (transform)
	{
		ubo.model = transform->GetModelMatrix();
	}
	else
	{
		ubo.model = glm::mat4(1.0f);
	}

	// Get camera view and projection
	if (camera)
	{
		ubo.view    = camera->GetViewMatrix();
		ubo.proj    = camera->GetProjectionMatrix();
		ubo.viewPos = glm::vec4(camera->GetPosition(), 1.0f);
	}
	else
	{
		ubo.view    = glm::mat4(1.0f);
		ubo.proj    = glm::mat4(1.0f);
		ubo.viewPos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	// Set light position and color
	ubo.lightPos   = glm::vec4(0.0f, 5.0f, 0.0f, 1.0f);
	ubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	assert(entityResources.find(entity) != entityResources.end());
	assert(entityResources[entity].uniformBuffers.size() > currentImage);
	// Copy data to uniform buffer
	memcpy(entityResources[entity].uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

// Create buffer
std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> DescriptorManager::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	// Create buffer
	vk::BufferCreateInfo bufferInfo{
	    .size        = size,
	    .usage       = usage,
	    .sharingMode = vk::SharingMode::eExclusive};

	vk::raii::Buffer buffer(device.getDevice(), bufferInfo);

	// Allocate memory
	vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

	vk::MemoryAllocateInfo allocInfo{
	    .allocationSize  = memRequirements.size,
	    .memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties)};

	vk::raii::DeviceMemory bufferMemory(device.getDevice(), allocInfo);

	// Bind memory
	buffer.bindMemory(*bufferMemory, 0);

	return {std::move(buffer), std::move(bufferMemory)};
}
