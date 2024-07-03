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

#include "api_vulkan_sample.h"

class DynamicLineRasterization : public ApiVulkanSample
{
  public:
	DynamicLineRasterization();
	virtual ~DynamicLineRasterization();

	void render(float delta_time) override;
	void build_command_buffers() override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	bool resize(const uint32_t width, const uint32_t height) override;

  private:
	struct CameraUbo
	{
		alignas(16) glm::mat4 projection;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 viewProjectionInverse;
	};

	struct
	{
		VkPipeline grid;
		VkPipeline object;
	} pipelines;

	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorPool      descriptor_pool;

	std::unique_ptr<vkb::core::Buffer> camera_ubo;
	std::unique_ptr<vkb::core::Buffer> vertex_buffer;
	std::unique_ptr<vkb::core::Buffer> cube_index_buffer, edges_index_buffer;

	glm::vec4 fill_color, edge_color;

	uint32_t cube_index_count, edges_index_count;

	struct
	{
		bool fill_enabled = true;
		bool grid_enabled = true;

		int                      selected_rasterization_mode = 0;
		std::vector<std::string> rasterization_mode_names    = {"DEFAULT", "RECT", "BRESENHAM", "SMOOTH"};

		bool  stipple_enabled = true;
		float line_width      = 1.0f;

		int32_t  stipple_factor = 1;
		uint16_t stipple_pattern;

		std::array<bool, 16> stipple_pattern_arr = {};
	} gui_settings;

	void draw();
	void prepare_scene();
	void create_pipelines();
	void create_descriptor_set();
	void create_descriptor_set_layout();
	void setup_descriptor_pool();
	void prepare_uniform_buffers();
	void update_uniform_buffers();

	static uint16_t array_to_uint16(const std::array<bool, 16> &array);
};

std::unique_ptr<vkb::VulkanSampleC> create_dynamic_line_rasterization();
