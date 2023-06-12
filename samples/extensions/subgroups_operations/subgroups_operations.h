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

struct TextureQuadVertex
{
	glm::vec3 pos;
	glm::vec2 uv;
};

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

	void prepare_graphics();
	void generate_quad();
	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	void setup_descriptor_set();
	void setup_pipelines();
	void create_semaphore();
	void prepare_uniform_buffers();
	void update_uniform_buffers();

	void prepare_compute();
	void create_compute_queue();
	void create_compute_command_pool();
	void create_compute_command_buffer();
	void create_compute_descriptor_set_layout();
	void create_compute_descriptor_set();

	void preapre_compute_pipeline_layout();
	void prepare_compute_pipeline();

	void build_compute_command_buffer();

	struct GuiSettings
	{
		int32_t                         selected_filtr = 0;
		static std::vector<std::string> init_filters_name()
		{
			std::vector<std::string> filtrs = {
			    {"Blur"}, {"Sharpen"}, {"Edge detection"}, {"Custom"}};
			return filtrs;
		}
	} gui_settings;

	struct Pipeline
	{
		void destroy(VkDevice device);

		VkPipeline       pipeline;
		VkPipelineLayout pipeline_layout;
	};

	struct
	{
		VkSemaphore           semaphore;
		VkDescriptorSetLayout descriptor_set_layout;
		VkDescriptorSet       descriptor_set;
		Pipeline              pipeline;
		Texture               texture;

		struct
		{
			std::unique_ptr<vkb::core::Buffer> vertex;
			std::unique_ptr<vkb::core::Buffer> index;
			uint32_t                           index_count = {0u};
		} buffers;
	} texture_object;

	struct
	{
		glm::mat3 kernel;
	} kernel_ubo;

	std::unique_ptr<vkb::core::Buffer> texture_uniform_buffer;

	struct
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
	} texture_ubo;

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

	VkPhysicalDeviceSubgroupProperties subgroups_properties;
};

std::unique_ptr<vkb::VulkanSample> create_subgroups_operations();
