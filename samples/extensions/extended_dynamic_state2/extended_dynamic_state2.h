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

#include <string>
#include <vector>
#include <memory>

#include "api_vulkan_sample.h"
#include "scene_graph/components/pbr_material.h"

class ExtendedDynamicState2 : public ApiVulkanSample
{
 public:
	typedef struct ModelDynamicParam
	{
		bool depth_bias         = false;
		bool rasterizer_discard = false;
	} ModelDynamicParam;

	struct
	{
		bool                           tessellation = false;
		float                          tess_factor  = 1.0f;
		std::vector<ModelDynamicParam> objects;
		int                            selected_obj     = 0;
		bool                           selection_active = true;
		bool                           time_tick        = false;
	} gui_settings;

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

	struct UBOTESS
	{
		float tessellation_factor = 1.0f;
	} ubo_tess;

	VkDescriptorPool descriptor_pool{VK_NULL_HANDLE};

	struct
	{
		VkDescriptorSetLayout baseline{VK_NULL_HANDLE};
		VkDescriptorSetLayout tesselation{VK_NULL_HANDLE};
		VkDescriptorSetLayout background{VK_NULL_HANDLE};
	} descriptor_set_layouts;

	struct
	{
		VkPipelineLayout baseline{VK_NULL_HANDLE};
		VkPipelineLayout tesselation{VK_NULL_HANDLE};
		VkPipelineLayout background{VK_NULL_HANDLE};
	} pipeline_layouts;

	struct
	{
		VkDescriptorSet baseline{VK_NULL_HANDLE};
		VkDescriptorSet tesselation{VK_NULL_HANDLE};
		VkDescriptorSet background{VK_NULL_HANDLE};
	} descriptor_sets;

	struct
	{
		VkPipeline baseline{VK_NULL_HANDLE};
		VkPipeline tesselation{VK_NULL_HANDLE};
		VkPipeline background{VK_NULL_HANDLE};
	} pipeline;

	struct
	{
		std::unique_ptr<vkb::core::Buffer> common;
		std::unique_ptr<vkb::core::Buffer> baseline;
		std::unique_ptr<vkb::core::Buffer> tesselation;
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

	struct SceneNode
	{
		std::string       name;
		vkb::sg::Node    *node;
		vkb::sg::SubMesh *sub_mesh;
	};
	std::vector<SceneNode> scene_elements_baseline;
	std::vector<SceneNode> scene_elements_tess;

	std::unique_ptr<vkb::sg::SubMesh> background_model;

	struct Cube
	{
		std::unique_ptr<vkb::core::Buffer> vertices_pos;
		std::unique_ptr<vkb::core::Buffer> vertices_norm;
		std::unique_ptr<vkb::core::Buffer> indices;
		uint32_t                           index_count;
	} cube;

	ExtendedDynamicState2();
	~ExtendedDynamicState2();

	void render(float delta_time) override;
	void build_command_buffers() override;
	bool prepare(vkb::Platform &platform) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void update(float delta_time) override;

	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void create_pipeline();
	void draw();

	void      load_assets();
	void      create_descriptor_pool();
	void      setup_descriptor_set_layout();
	void      create_descriptor_sets();
	glm::vec4 get_changed_alpha(const vkb::sg::PBRMaterial *original_mat);
	void      scene_pipeline_divide(std::vector<SceneNode> const &scene_node);
	void      draw_from_scene(VkCommandBuffer command_buffer, std::vector<SceneNode> const &scene_node);
	void      draw_created_model(VkCommandBuffer commandBuffer);
	void      model_data_creation();
	void      cube_animation(float delta_time);
};

std::unique_ptr<vkb::VulkanSample> create_extended_dynamic_state2();
