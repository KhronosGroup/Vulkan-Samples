/* Copyright (c) 2019-2020, Sascha Willems
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

/*
 * Basic example for ray tracing using VK_KHR_ray_tracing
 */

/*
 * Important note:
 *
 * The VK_KHR_ray_tracing extension is currently considered a BETA extension.
 *
 * This means that it is not yet production ready and subject to change until it's finalized.
 *
 * In order to use this sample you may also need special developer drivers.
 */

#pragma once

#include "api_vulkan_sample.h"

// Indices for the different ray tracing shader types used in this example
#define INDEX_RAYGEN 0
#define INDEX_CLOSEST_HIT 1
#define INDEX_MISS 2

// Holds data for a ray tracing scratch buffer that is used as a temporary storage
struct RayTracingScratchBuffer
{
	vkb::Device &  device;
	uint64_t       device_address = 0;
	VkBuffer       buffer         = VK_NULL_HANDLE;
	VkDeviceMemory memory         = VK_NULL_HANDLE;
	RayTracingScratchBuffer(vkb::Device &device, VkAccelerationStructureKHR acceleration_structure);
	~RayTracingScratchBuffer();
};

// Holds data for a memory object bound to an acceleration structure
struct RayTracingObjectMemory
{
	vkb::Device &  device;
	uint64_t       device_address = 0;
	VkDeviceMemory memory         = VK_NULL_HANDLE;
	RayTracingObjectMemory(vkb::Device &device, VkAccelerationStructureKHR acceleration_structure);
};

class RaytracingBasic : public ApiVulkanSample
{
  public:
	VkPhysicalDeviceRayTracingPropertiesKHR ray_tracing_properties{};
	VkPhysicalDeviceRayTracingFeaturesKHR   ray_tracing_features{};

	VkAccelerationStructureKHR bottom_level_acceleration_structure;
	uint64_t                   bottom_level_acceleration_structure_handle = 0;
	VkAccelerationStructureKHR top_level_acceleration_structure;
	uint64_t                   top_level_acceleration_structure_handle = 0;

	std::unique_ptr<vkb::core::Buffer> vertex_buffer;
	std::unique_ptr<vkb::core::Buffer> index_buffer;
	uint32_t                           index_count;
	std::unique_ptr<vkb::core::Buffer> shader_binding_table;

	struct StorageImage
	{
		VkDeviceMemory memory;
		VkImage        image = VK_NULL_HANDLE;
		VkImageView    view;
		VkFormat       format;
		uint32_t       width;
		uint32_t       height;
	} storage_image;

	struct UniformData
	{
		glm::mat4 view_inverse;
		glm::mat4 proj_inverse;
	} uniform_data;
	std::unique_ptr<vkb::core::Buffer> ubo;

	VkPipeline            pipeline;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;

	RaytracingBasic();
	~RaytracingBasic();

	void         request_gpu_features(vkb::PhysicalDevice &gpu) override;
	uint64_t     get_buffer_device_address(VkBuffer buffer);
	void         create_storage_image();
	void         create_scene();
	void         create_shader_binding_table();
	void         create_descriptor_sets();
	void         create_ray_tracing_pipeline();
	void         create_uniform_buffer();
	void         build_command_buffers() override;
	void         update_uniform_buffers();
	void         draw();
	bool         prepare(vkb::Platform &platform) override;
	virtual void render(float delta_time) override;
};

std::unique_ptr<vkb::VulkanSample> create_raytracing_basic();
