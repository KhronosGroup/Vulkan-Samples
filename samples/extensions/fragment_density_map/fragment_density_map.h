/* Copyright (c) 2025, Arm Limited and Contributors
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
#include <vulkan/vulkan_core.h>

class FragmentDensityMap : public ApiVulkanSample
{
  public:
	FragmentDensityMap();
	~FragmentDensityMap() override;

	void prepare_pipelines();

	// Override basic framework functionality
	void build_command_buffers() override;
	void render(float delta_time) override;
	bool resize(const uint32_t width, const uint32_t height) override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void setup_render_pass() override;
	void setup_framebuffer() override;
	void setup_color();
	void setup_depth_stencil() override;
	void setup_samplers();
	void prepare_gui() override;

	void on_update_ui_overlay(vkb::Drawer &drawer) override;

  protected:
	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;

  private:
	struct PipelineData
	{
		VkPipeline            pipeline{VK_NULL_HANDLE};
		VkPipelineLayout      pipeline_layout{VK_NULL_HANDLE};
		VkDescriptorSetLayout set_layout{VK_NULL_HANDLE};
	};

	struct SubmeshData
	{
		const vkb::sg::SubMesh             *submesh;
		glm::mat4                           world_matrix;
		std::unique_ptr<vkb::core::BufferC> vertex_ubo;
		vkb::sg::Texture                   *base_color_texture;
	};

	struct ConfigOptions
	{
		bool operator==(const ConfigOptions &) const = default;
		bool enable_fdm{true};
		bool update_fdm{true};
		bool debug_fdm{false};
		bool show_stats{false};
		bool generate_fdm_compute{false};
		bool show_fdm{false};
	};

	struct UBOVS
	{
		glm::mat4 projection;
		glm::mat4 modelview;
	};

	struct FDMUBO
	{
		bool      operator==(const FDMUBO &) const = default;
		glm::vec4 eye_center{glm::vec4(0.0f)};
		glm::vec4 circle_radius{glm::vec4(0.0f)};
	};

	struct
	{
		VkSampler subsampled_nearest{VK_NULL_HANDLE};
		VkSampler nearest{VK_NULL_HANDLE};
	} samplers{};

	struct
	{
		bool supports_dynamic_fdm{true};
		bool supports_fdm{false};
	} available_options{};

	struct
	{
		ImageData image{};
		struct
		{
			PipelineData    pipeline{};
			VkDescriptorSet set{VK_NULL_HANDLE};
			VkRenderPass    render_pass = VK_NULL_HANDLE;
			VkFramebuffer   framebuffer = VK_NULL_HANDLE;
		} generate{};

		FDMUBO                              ubo_data{};
		std::unique_ptr<vkb::core::BufferC> ubo;
		VkExtent2D                          texel_size{32, 32};
		VkExtent3D                          extend{};
	} fdm{};

	struct
	{
		ImageData        image{};
		VkExtent2D       extend{};
		VkFramebuffer    framebuffer{VK_NULL_HANDLE};
		VkDescriptorPool descriptor_pool{VK_NULL_HANDLE};

		PipelineData sky_pipeline{};

		struct
		{
			PipelineData                 pipeline{};
			std::vector<VkDescriptorSet> descriptor_sets;
		} meshes;
	} main_pass{};

	struct
	{
		PipelineData pipeline{};

		VkRenderPass    render_pass = VK_NULL_HANDLE;
		VkDescriptorSet set{VK_NULL_HANDLE};
	} present{};

	vkb::DebugUtilsExtDebugUtils debug_utils;

	std::unique_ptr<vkb::sg::Scene> sg_scene;

	std::vector<SubmeshData> scene_data;

	ConfigOptions current_options;
	ConfigOptions last_options;

	uint32_t frame_idx{0};

	void reset_fdm_gpu_data();
	void setup_fragment_density_map();

	void write_density_map(VkCommandBuffer cmd_buffer);

	bool is_generate_fdm_compute();
	bool is_fdm_supported();
	bool is_fdm_enabled();
	bool is_update_fdm_enabled();
	bool is_show_fdm_enabled();
	bool is_show_stats();
	bool is_debug_fdm_enabled();

	void update_extents();

	void load_assets();

	void update_uniform_buffer(float delta_time);

	void prepare_uniform_buffers_main_pass();
	void setup_descriptor_pool_main_pass();
	void setup_descriptor_set_layout_main_pass();
	void setup_descriptor_set_main_pass();

	void prepare_uniform_buffers_fdm();
	void setup_additional_descriptor_pool();

	void setup_descriptor_set_layout_fdm();
	void setup_descriptor_set_fdm();

	void setup_descriptor_set_layout_present();
	void setup_descriptor_set_present();

	void destroy_pipeline(PipelineData &pipeline_data);
	void destroy_image(ApiVulkanSample::ImageData &image_data);
};

std::unique_ptr<vkb::VulkanSampleC> create_fragment_density_map();
