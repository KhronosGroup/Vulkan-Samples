/* Copyright (c) 2024, Sascha Willems
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

#include "api_vulkan_sample.h"
#include "glsl_compiler.h"
#include <core/acceleration_structure.h>

class RayTracingPositionFetch : public ApiVulkanSample
{
  public:
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  ray_tracing_pipeline_properties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{};

	std::unique_ptr<vkb::core::AccelerationStructure> bottom_level_acceleration_structure{nullptr};
	std::unique_ptr<vkb::core::AccelerationStructure> top_level_acceleration_structure{nullptr};

	std::unique_ptr<vkb::core::Buffer>                vertex_buffer;
	std::unique_ptr<vkb::core::Buffer>                index_buffer;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups{};

	std::unique_ptr<vkb::core::Buffer> raygen_shader_binding_table;
	std::unique_ptr<vkb::core::Buffer> miss_shader_binding_table;
	std::unique_ptr<vkb::core::Buffer> hit_shader_binding_table;

	struct StorageImage
	{
		VkDeviceMemory memory;
		VkImage        image;
		VkImageView    view;
		VkFormat       format;
		uint32_t       width;
		uint32_t       height;
	} storage_image{};

	struct UniformData
	{
		glm::mat4 view_inverse{1.0f};
		glm::mat4 proj_inverse{1.0f};
		int32_t   display_mode{0};
	} uniform_data;
	std::unique_ptr<vkb::core::Buffer> ubo;

	VkPipeline            pipeline{VK_NULL_HANDLE};
	VkPipelineLayout      pipeline_layout{VK_NULL_HANDLE};
	VkDescriptorSet       descriptor_set{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};

	RayTracingPositionFetch();
	virtual ~RayTracingPositionFetch();

	void         request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         create_storage_image();
	void         create_bottom_level_acceleration_structure();
	void         create_top_level_acceleration_structure();
	void         create_scene();
	void         create_shader_binding_tables();
	void         create_descriptor_sets();
	void         create_ray_tracing_pipeline();
	void         create_uniform_buffer();
	void         build_command_buffers() override;
	void         update_uniform_buffers();
	void         draw();
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
	bool         prepare(const vkb::ApplicationOptions &options) override;
	virtual void render(float delta_time) override;
};

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_ray_tracing_position_fetch();
