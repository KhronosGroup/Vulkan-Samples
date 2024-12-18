/* Copyright (c) 2023-2024, Sascha Willems
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
 * Using VK_EXT_conditional_rendering, which executes or discards draw commands based on values sourced from a buffer
 */

#pragma once

#include "api_vulkan_sample.h"

class ConditionalRendering : public ApiVulkanSample
{
  public:
	std::unique_ptr<vkb::core::BufferC> vertex_buffer = nullptr;
	std::unique_ptr<vkb::core::BufferC> index_buffer  = nullptr;

	std::unique_ptr<vkb::sg::Scene> scene;
	struct SceneNode
	{
		std::string       name;
		vkb::sg::Node    *node;
		vkb::sg::SubMesh *sub_mesh;
	};
	std::vector<SceneNode> linear_scene_nodes;

	struct UniformData
	{
		glm::mat4 projection;
		glm::mat4 view;
	} uniform_data;
	std::unique_ptr<vkb::core::BufferC> uniform_buffer;

	VkPipeline            pipeline;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;

	struct
	{
		glm::mat4 model_matrix;
		glm::vec4 color;
	} push_const_block;

	std::vector<int32_t>                conditional_visibility_list;
	std::unique_ptr<vkb::core::BufferC> conditional_visibility_buffer;

	ConditionalRendering();
	~ConditionalRendering();
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         build_command_buffers() override;
	void         load_assets();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_sets();
	void         prepare_pipelines();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	void         prepare_visibility_buffer();
	void         update_visibility_buffer();
	void         draw();
	bool         prepare(const vkb::ApplicationOptions &options) override;
	virtual void render(float delta_time) override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
	virtual bool resize(const uint32_t width, const uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_conditional_rendering();
