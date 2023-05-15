/* Copyright (c) 2023, Sascha Willems
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
 * Graphics pipeline libraries
 *
 * Note: Requires a device that supports the VK_EXT_graphics_pipeline_library
 *
 * Creates a pipeline library for shared pipeline parts like vertex input and fragment output interfaces. These pre-built pipeline
 * "building blocks" are then used for runtime pipeline creation, which will be faster than always creating a full pipeline
 */

#pragma once

#include "api_vulkan_sample.h"

class GraphicsPipelineLibrary : public ApiVulkanSample
{
  public:
	bool link_time_optimization = true;

	std::unique_ptr<vkb::sg::SubMesh> scene;

	struct UBOVS
	{
		glm::mat4 projection = glm::mat4(1.0f);
		glm::mat4 modelview  = glm::mat4(1.0f);
		glm::vec4 lightPos   = glm::vec4(-10.0f, -5.0f, 15.0f, 0.0f);
	} ubo_vs;

	std::unique_ptr<vkb::core::Buffer> uniform_buffer{nullptr};

	struct
	{
		VkPipeline vertex_input_interface{VK_NULL_HANDLE};
		VkPipeline pre_rasterization_shaders{VK_NULL_HANDLE};
		VkPipeline fragment_output_interface{VK_NULL_HANDLE};
		std::vector<VkPipeline> fragment_shaders;
	} pipeline_library;

	// Will be dynamically created at runtime from pipeline library
	std::vector<VkPipeline> pipelines{};

	VkPipelineLayout      pipeline_layout{VK_NULL_HANDLE};
	VkDescriptorSet       descriptor_set{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};

	std::mutex      mutex;
	VkPipelineCache thread_pipeline_cache{VK_NULL_HANDLE};

	bool  new_pipeline_created = false;
	float accumulated_time{};

	uint32_t split_x = 3;
	uint32_t split_y = 3;

	std::vector<glm::vec3> colors{};

	GraphicsPipelineLibrary();
	~GraphicsPipelineLibrary();
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         build_command_buffers() override;
	void         load_assets();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_sets();
	void         compile_shader(const std::string filename, VkShaderStageFlagBits shader_stage, std::vector<uint32_t> &spirv);
	void         prepare_pipeline_library();
	void         prepare_new_pipeline();
	void         pipeline_creation_threadfn();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	void         draw();
	bool         prepare(vkb::Platform &platform) override;
	virtual void render(float delta_time) override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
	virtual bool resize(const uint32_t width, const uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_graphics_pipeline_library();
