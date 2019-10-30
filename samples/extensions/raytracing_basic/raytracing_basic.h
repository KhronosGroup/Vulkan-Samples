/* Copyright (c) 2019, Sascha Willems
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
 * Basic example for ray tracing using VK_NV_ray_tracing
 */

#pragma once

#include "api_vulkan_sample.h"

// Ray tracing acceleration structure
struct AccelerationStructure
{
	VkDeviceMemory            memory;
	VkAccelerationStructureNV acceleration_structure;
	uint64_t                  handle;
};

// Ray tracing geometry instance
struct GeometryInstance
{
	glm::mat3x4 transform;
	uint32_t    instance_id : 24;
	uint32_t    mask : 8;
	uint32_t    instance_offset : 24;
	uint32_t    flags : 8;
	uint64_t    acceleration_structure_handle;
};

// Indices for the different ray tracing shader types used in this example
#define INDEX_RAYGEN 0
#define INDEX_MISS 1
#define INDEX_CLOSEST_HIT 2

class RaytracingBasic : public ApiVulkanSample
{
  public:
	PFN_vkCreateAccelerationStructureNV                vkCreateAccelerationStructureNV;
	PFN_vkDestroyAccelerationStructureNV               vkDestroyAccelerationStructureNV;
	PFN_vkBindAccelerationStructureMemoryNV            vkBindAccelerationStructureMemoryNV;
	PFN_vkGetAccelerationStructureHandleNV             vkGetAccelerationStructureHandleNV;
	PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
	PFN_vkCmdBuildAccelerationStructureNV              vkCmdBuildAccelerationStructureNV;
	PFN_vkCreateRayTracingPipelinesNV                  vkCreateRayTracingPipelinesNV;
	PFN_vkGetRayTracingShaderGroupHandlesNV            vkGetRayTracingShaderGroupHandlesNV;
	PFN_vkCmdTraceRaysNV                               vkCmdTraceRaysNV;

	VkPhysicalDeviceRayTracingPropertiesNV ray_tracing_properties{};

	AccelerationStructure bottom_level_acceleration_structure;
	AccelerationStructure top_level_acceleration_structure;

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

	void         create_storage_image();
	void         create_bottom_level_acceleration_structure(const VkGeometryNV *geometries);
	void         create_top_level_acceleration_structure();
	void         create_scene();
	VkDeviceSize copy_shader_identifier(uint8_t *data, const uint8_t *shader_handle_storage, uint32_t group_index);
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
