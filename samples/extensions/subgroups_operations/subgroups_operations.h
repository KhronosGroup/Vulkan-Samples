/* Copyright (c) 2023, Mobica Limited
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

class SubgroupsOperations : public ApiVulkanSample
{
  public:
	SubgroupsOperations();
	~SubgroupsOperations();

	bool prepare(vkb::Platform &platform) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;
	void render(float delta_time) override;
	bool resize(const uint32_t width, const uint32_t height) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

	void draw();
	void load_assets();

	void prepare_compute();
	void create_compute_queue();
	void create_compute_command_pool();
	void create_compute_command_buffer();
	void create_compute_descriptor_set_layout();
	void create_compute_descriptor_set();

	void preapre_compute_pipeline_layout();
	void prepare_compute_pipeline();

	void build_compute_command_buffer();

	void generate_grid();
	void prepare_uniform_buffers();
	void setup_descriptor_pool();
	void create_descriptor_set_layout();
	void create_descriptor_set();
	void create_pipelines();

	void update_uniform_buffers();

	struct Pipeline
	{
		void destroy(VkDevice device);

		VkPipeline       pipeline        = {VK_NULL_HANDLE};
		VkPipelineLayout pipeline_layout = {VK_NULL_HANDLE};
	};

	struct GridBuffers
	{
		std::unique_ptr<vkb::core::Buffer> vertex;
		std::unique_ptr<vkb::core::Buffer> index;
		uint32_t                           index_count = {0u};
	};

	struct CameraUbo
	{
		alignas(16) glm::mat4 projection;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 model;
	};

	struct ComputeUbo
	{
		alignas(4) uint32_t grid_size;
		alignas(4) float time;
	};

	struct GuiConfig
	{
		GuiConfig()
		{
			grid_sizes = {
			    "16", "32", "64", "128", "256"};
		}
		bool    wireframe     = {false};
		int32_t grid_size_idx = {0};

		std::vector<std::string> grid_sizes;
	} ui;

	GridBuffers                        input_grid;        // input buffer for compute shader
	uint32_t                           grid_size;
	std::unique_ptr<vkb::core::Buffer> camera_ubo;
	std::unique_ptr<vkb::core::Buffer> compute_ubo;

	struct
	{
		VkQueue               queue;
		VkCommandPool         command_pool;
		VkCommandBuffer       command_buffer;
		VkSemaphore           semaphore;
		VkDescriptorSetLayout descriptor_set_layout;
		VkDescriptorSet       descriptor_set;
		uint32_t              queue_family_index;

		struct
		{
			Pipeline _default;
		} pipelines;
	} compute;

	struct
	{
		GridBuffers grid;        // output (result) buffer for compute shader

		VkDescriptorSetLayout descriptor_set_layout;
		VkDescriptorSet       descriptor_set;
		struct
		{
			Pipeline _default;
			Pipeline wireframe;
		} pipelines;
	} ocean;

	VkPhysicalDeviceSubgroupProperties subgroups_properties;
};

std::unique_ptr<vkb::VulkanSample> create_subgroups_operations();
