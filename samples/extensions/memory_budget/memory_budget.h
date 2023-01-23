/* Copyright (c) 2019-2023, Sascha Willems
 * Copyright (c) 2023, Holochip Corporation
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
 * Instanced mesh rendering, uses a separate vertex buffer for instanced data.
 * Adjust density of instanced meshes, displays hardware memory availability/consumption.
 */

#pragma once

#include "api_vulkan_sample.h"

#define MESH_DENSITY 2048
#define MESH_DENSITY_HALF 1024

class MemoryBudget : public ApiVulkanSample
{
  private:
	// Memory budget extension related variables
	VkPhysicalDeviceMemoryBudgetPropertiesEXT physical_device_memory_budget_properties{};
	VkPhysicalDeviceMemoryProperties2         device_memory_properties{};

	struct ConvertedMemory
	{
		float       data;
		std::string units;
	} converted_memory{};

	const float kilobyte_coefficient = 1024.0f;
	const float megabyte_coefficient = kilobyte_coefficient * 1024.0f;
	const float gigabyte_coefficient = megabyte_coefficient * 1024.0f;

	uint32_t     device_memory_heap_count   = 0;
	VkDeviceSize device_memory_total_usage  = 0;
	VkDeviceSize device_memory_total_budget = 0;

  public:
	struct Textures
	{
		Texture rocks;
		Texture planet;
	} textures;

	struct Models
	{
		std::unique_ptr<vkb::sg::SubMesh> rock;
		std::unique_ptr<vkb::sg::SubMesh> planet;
	} models;

	// Per-instance data block
	struct InstanceData
	{
		glm::vec3 pos;
		glm::vec3 rot;
		float     scale;
		uint32_t  texIndex;
	};

	// Contains the instanced data
	struct InstanceBuffer
	{
		VkBuffer               buffer = VK_NULL_HANDLE;
		VkDeviceMemory         memory = VK_NULL_HANDLE;
		size_t                 size   = 0;
		VkDescriptorBufferInfo descriptor{};
	} instance_buffer;

	struct UBOVS
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec4 light_pos  = glm::vec4(0.0f, -5.0f, 0.0f, 1.0f);
		float     loc_speed  = 0.0f;
		float     glob_speed = 0.0f;
	} ubo_vs;

	struct UniformBuffers
	{
		std::unique_ptr<vkb::core::Buffer> scene;
	} uniform_buffers;

	VkPipelineLayout pipeline_layout{};
	struct Pipelines
	{
		VkPipeline instanced_rocks{};
		VkPipeline planet{};
		VkPipeline starfield{};
	} pipelines;

	VkDescriptorSetLayout descriptor_set_layout{};
	struct DescriptorSets
	{
		VkDescriptorSet instanced_rocks{};
		VkDescriptorSet planet{};
	} descriptor_sets;

  private:
	// memory budget extension related function
	void                                initialize_device_memory_properties();
	const MemoryBudget::ConvertedMemory update_converted_memory(uint64_t input_memory);
	const std::string                   read_memoryHeap_flags(VkMemoryHeapFlags inputVkMemoryFlag);
	void                                update_device_memory_properties();

  public:
	MemoryBudget();
	~MemoryBudget() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;

	void load_assets();
	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	void setup_descriptor_set();
	void prepare_pipelines();
	void prepare_instance_data();
	void prepare_uniform_buffers();
	void update_uniform_buffer(float delta_time);
	void draw();
	bool prepare(vkb::Platform &platform) override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	bool resize(uint32_t width, uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_memory_budget();
