/* Copyright (c) 2023-2024, Mobica Limited
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

#include <memory>
#include <string>
#include <vector>

#include "api_vulkan_sample.h"

class LogicOpDynamicState : public ApiVulkanSample
{
  public:
	typedef struct ModelDynamicParam
	{
		bool depth_bias         = false;
		bool rasterizer_discard = false;
	} ModelDynamicParam;

	/* Buffer used in all pipelines */
	struct UBOCOMM
	{
		glm::mat4 projection;
		glm::mat4 view;
	} ubo_common;

	struct UBOBAS
	{
		glm::vec4 ambientLightColor = glm::vec4(1.f, 1.f, 1.f, 0.1f);
		glm::vec4 lightPosition     = glm::vec4(-3.0f, -8.0f, 6.0f, -1.0f);
		glm::vec4 lightColor        = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		float     lightIntensity    = 50.0f;
	} ubo_baseline;

	struct
	{
		VkDescriptorSetLayout baseline{VK_NULL_HANDLE};
		VkDescriptorSetLayout background{VK_NULL_HANDLE};
	} descriptor_set_layouts;

	struct
	{
		VkPipelineLayout baseline{VK_NULL_HANDLE};
		VkPipelineLayout background{VK_NULL_HANDLE};
	} pipeline_layouts;

	struct
	{
		VkDescriptorSet baseline{VK_NULL_HANDLE};
		VkDescriptorSet background{VK_NULL_HANDLE};
	} descriptor_sets;

	struct
	{
		VkPipeline baseline{VK_NULL_HANDLE};
		VkPipeline background{VK_NULL_HANDLE};
	} pipeline;

	struct
	{
		std::unique_ptr<vkb::core::BufferC> common;
		std::unique_ptr<vkb::core::BufferC> baseline;
	} uniform_buffers;

	struct
	{
		Texture envmap;
	} textures;

	struct
	{
		glm::mat4 model_matrix;
		glm::vec4 color;
	} push_const_block;

	std::unique_ptr<vkb::sg::SubMesh> background_model;

	struct Cube
	{
		std::unique_ptr<vkb::core::BufferC> vertices_pos;
		std::unique_ptr<vkb::core::BufferC> vertices_norm;
		std::unique_ptr<vkb::core::BufferC> indices;
		uint32_t                            index_count{};
	} cube;

	struct GUI_settings
	{
		bool                            logic_op_enable    = false;
		int                             selected_operation = 3; /* VK_LOGIC_OP_COPY */
		static std::vector<std::string> logic_op_names;

		/* Method used to initialize logic_op_names */
		static std::vector<std::string> init_logic_op_names()
		{
			std::vector<std::string> names;
			for (int i = 0; i <= VK_LOGIC_OP_SET; ++i) /* VK_LOGIC_OP_SET is last operation in enum VkLogicOp */
			{
				names.push_back(vkb::to_string(static_cast<VkLogicOp>(i)));
			}
			return names;
		}
	} gui_settings;

	LogicOpDynamicState();
	~LogicOpDynamicState() override;

	bool prepare(const vkb::ApplicationOptions &options) override;
	void render(float delta_time) override;
	void build_command_buffers() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void create_pipeline();
	void draw();
	void load_assets();
	void create_descriptor_pool();
	void setup_descriptor_set_layout();
	void create_descriptor_sets();
	void model_data_creation();
	void draw_created_model(VkCommandBuffer commandBuffer);

  protected:
	void create_render_context() override;
};

std::unique_ptr<vkb::VulkanSampleC> create_logic_op_dynamic_state();
