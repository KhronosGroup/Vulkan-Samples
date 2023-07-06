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

	struct OceanVertex
	{
		glm::vec3 position;
		glm::vec3 normal;
	};

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
	void create_semaphore();
	void create_descriptor_set_layout();
	void create_descriptor_set();
	void create_pipelines();

	void update_uniform_buffers();
	void update_compute_descriptor();

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

	struct FFTParametersUbo
	{
		alignas(4) float amplitude;
		alignas(4) float length;
		alignas(8) glm::vec2 wind;
		alignas(4) uint32_t grid_size;
	};

	struct GuiConfig
	{
		bool      wireframe = {false};
		float     amplitude = {0.005f};
		float     length    = {16.0f};
		glm::vec2 wind      = {16.0f, 0.0f};
	} ui;

	GridBuffers                        input_grid;        // input buffer for compute shader
	uint32_t                           grid_size = {32u};
	std::unique_ptr<vkb::core::Buffer> camera_ubo;
	std::unique_ptr<vkb::core::Buffer> fft_params_ubo;
	std::unique_ptr<vkb::core::Buffer> fft_input_buffer;        // TODO: change name

	struct
	{
		VkQueue               queue                 = {VK_NULL_HANDLE};
		VkCommandPool         command_pool          = {VK_NULL_HANDLE};
		VkCommandBuffer       command_buffer        = {VK_NULL_HANDLE};
		VkSemaphore           semaphore             = {VK_NULL_HANDLE};
		VkDescriptorSetLayout descriptor_set_layout = {VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set        = {VK_NULL_HANDLE};
		uint32_t              queue_family_index    = {-1u};

		struct
		{
			Pipeline _default;
			Pipeline fft_input;
		} pipelines;
	} compute;

	struct
	{
		GridBuffers           grid;        // output (result) buffer for compute shader
		uint32_t              graphics_queue_family_index = {-1u};
		VkDescriptorSetLayout descriptor_set_layout       = {VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set              = {VK_NULL_HANDLE};
		VkSemaphore           semaphore                   = {VK_NULL_HANDLE};
		struct
		{
			Pipeline _default;
			Pipeline wireframe;
		} pipelines;
	} ocean;

	VkPhysicalDeviceSubgroupProperties subgroups_properties;
};

std::unique_ptr<vkb::VulkanSample> create_subgroups_operations();
