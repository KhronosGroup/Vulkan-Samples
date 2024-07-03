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
#include "scene_graph/components/pbr_material.h"

class PatchControlPoints : public ApiVulkanSample
{
  public:
	struct
	{
		bool  tessellation = true;
		float tess_level   = 3.0f;
	} gui_settings;

	/* Buffer used in both pipelines */
	struct UBOCOMM
	{
		glm::mat4 projection;
		glm::mat4 view;
	} ubo_common;

	struct UBOTESS
	{
		float tessellation_level = 3.0f;
	} ubo_tess;

	VkDescriptorPool descriptor_pool{VK_NULL_HANDLE};

	struct
	{
		VkDescriptorSetLayout static_tessellation{VK_NULL_HANDLE};
		VkDescriptorSetLayout dynamic_tessellation{VK_NULL_HANDLE};
	} descriptor_set_layouts;

	struct
	{
		VkPipelineLayout static_tessellation{VK_NULL_HANDLE};
		VkPipelineLayout dynamic_tessellation{VK_NULL_HANDLE};
	} pipeline_layouts;

	struct
	{
		VkDescriptorSet static_tessellation{VK_NULL_HANDLE};
		VkDescriptorSet dynamic_tessellation{VK_NULL_HANDLE};
	} descriptor_sets;

	struct
	{
		VkPipeline static_tessellation{VK_NULL_HANDLE};
		VkPipeline dynamic_tessellation{VK_NULL_HANDLE};
	} pipeline;

	struct
	{
		std::unique_ptr<vkb::core::Buffer> common;
		std::unique_ptr<vkb::core::Buffer> static_tessellation;
		std::unique_ptr<vkb::core::Buffer> dynamic_tessellation;
	} uniform_buffers;

	struct
	{
		glm::vec3 direction;
	} push_const_block;

	std::unique_ptr<vkb::sg::SubMesh> model;

	PatchControlPoints();
	~PatchControlPoints() override;

	void render(float delta_time) override;
	void build_command_buffers() override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void create_pipelines();
	void draw();

	void load_assets();
	void create_descriptor_pool();
	void setup_descriptor_set_layout();
	void create_descriptor_sets();
};

std::unique_ptr<vkb::VulkanSampleC> create_patch_control_points();
