/* Copyright (c) 2022-2023, Sascha Willems
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
 * Using Vulkan profiles from the LunarG SDK for device and instance setup
 */

#pragma once

#include "common/vk_common.h"

#include "api_vulkan_sample.h"

class Profiles : public ApiVulkanSample
{
  public:
	// Vertex layout for this example
	struct VertexStructure
	{
		float   pos[3];
		float   uv[2];
		int32_t texture_index;
	};

	struct RandomTexture
	{
		VkImage        image{};
		VkImageView    image_view{};
		VkDeviceMemory memory{};
	};
	std::vector<RandomTexture> textures;
	VkSampler                  sampler{};

	std::unique_ptr<vkb::core::Buffer> vertex_buffer;
	std::unique_ptr<vkb::core::Buffer> index_buffer;
	uint32_t                           index_count{0};

	std::unique_ptr<vkb::core::Buffer> uniform_buffer_vs;

	struct
	{
		glm::mat4 projection;
		glm::mat4 view;
	} ubo_vs;

	VkPipeline       pipeline{VK_NULL_HANDLE};
	VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
	VkDescriptorSet  base_descriptor_set{VK_NULL_HANDLE};

	VkDescriptorSetLayout base_descriptor_set_layout{VK_NULL_HANDLE};
	VkDescriptorSetLayout sampler_descriptor_set_layout{VK_NULL_HANDLE};

	Profiles();
	~Profiles() override;
	void         generate_textures();
	void         generate_cubes();
	void         build_command_buffers() override;
	void         draw();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_set();
	void         prepare_pipelines();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	bool         prepare(const vkb::ApplicationOptions &options) override;
	void         render(float delta_time) override;
	void         view_changed() override;
	void         create_device() override;
	void         create_instance() override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
};

std::unique_ptr<vkb::Application> create_profiles();
