/* Copyright (c) 2025-2026, Arm Limited and Contributors
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

#include "gltf_api_vulkan_sample.h"
#include <vulkan/vulkan_core.h>

class FragmentDensityMap : public GLTFApiVulkanSample
{
  public:
	FragmentDensityMap();
	~FragmentDensityMap() override;

	void prepare_pipelines();

	// Override basic framework functionality
	void build_command_buffers() override;
	void render(float delta_time) override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void setup_render_pass() override;
	void setup_framebuffer() override;
	void setup_color() override;
	void setup_depth_stencil() override;
	void setup_samplers() override;

	void on_update_ui_overlay(vkb::Drawer &drawer) override;

  protected:
	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;

  private:
	struct ConfigOptionsFDM
	{
		bool operator==(const ConfigOptionsFDM &) const = default;
		bool enable_fdm{true};
		bool update_fdm{true};
		bool debug_fdm{false};
		bool show_stats{false};
		bool generate_fdm_compute{false};
		bool show_fdm{false};
	};

	struct FDMUBO
	{
		bool      operator==(const FDMUBO &) const = default;
		glm::vec4 eye_center{glm::vec4(0.0f)};
		glm::vec4 circle_radius{glm::vec4(0.0f)};
	};

	struct SamplersFDM
	{
		VkSampler subsampled_nearest{VK_NULL_HANDLE};
	} samplersFDM{};

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

	ConfigOptionsFDM current_options_fdm;
	ConfigOptionsFDM last_options_fdm;

	void recreate_swapchain_resources() override;
	void update_extents() override;
	void update_uniform_buffer(float delta_time) override;
	void setup_additional_descriptor_pool();
	void setup_descriptor_set_layout_present() override;
	void setup_descriptor_set_present() override;

	void setup_fragment_density_map();

	void write_density_map(VkCommandBuffer cmd_buffer);

	bool is_generate_fdm_compute();
	bool is_fdm_supported();
	bool is_fdm_enabled();
	bool is_show_stats() override;
	bool is_update_fdm_enabled();
	bool is_show_fdm_enabled();
	bool is_debug_fdm_enabled();

	void prepare_uniform_buffers_fdm();

	void setup_descriptor_set_layout_fdm();
	void setup_descriptor_set_fdm();
};

std::unique_ptr<vkb::VulkanSampleC> create_fragment_density_map();
