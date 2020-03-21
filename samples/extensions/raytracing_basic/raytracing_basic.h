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

#pragma once

#define VK_KHR_ray_tracing

#include "api_vulkan_sample.h"

// Indices for the different ray tracing shader types used in this example
#define INDEX_RAYGEN 0
#define INDEX_MISS 1
#define INDEX_CLOSEST_HIT 2

struct ScratchBuffer
{
	uint64_t       device_address = 0;
	VkBuffer       buffer         = VK_NULL_HANDLE;
	VkDeviceMemory memory         = VK_NULL_HANDLE;
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
		VkImage        image;
		VkImageView    view;
		VkFormat       format;
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

	ScratchBuffer create_scratch_buffer(ScratchBuffer &scratch_buffer, VkAccelerationStructureKHR acceleration_structure);
	void          create_storage_image();
	void          create_scene();
	VkDeviceSize  copy_shader_identifier(uint8_t *data, const uint8_t *shader_handle_storage, uint32_t group_index);
	void          create_shader_binding_table();
	void          create_descriptor_sets();
	void          create_ray_tracing_pipeline();
	void          create_uniform_buffer();
	void          build_command_buffers() override;
	void          update_uniform_buffers();
	void          draw();
	bool          prepare(vkb::Platform &platform) override;
	virtual void  render(float delta_time) override;
};

std::unique_ptr<vkb::VulkanSample> create_raytracing_basic();
