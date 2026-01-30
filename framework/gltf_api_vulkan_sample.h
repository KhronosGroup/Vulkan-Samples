/* Copyright (c) 2026, Arm Limited and Contributors
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

template <typename THandle>
static uint64_t get_object_handle(THandle object)
{
	using UintHandle = typename std::conditional<sizeof(THandle) == sizeof(uint32_t), uint32_t, uint64_t>::type;
	return static_cast<uint64_t>(reinterpret_cast<UintHandle>(object));
}

class GLTFApiVulkanSample : public ApiVulkanSample
{
  public:
	GLTFApiVulkanSample();
	~GLTFApiVulkanSample() override;

	// Override basic framework functionality
	bool resize(const uint32_t width, const uint32_t height) override;
	void setup_render_pass() override;
	void setup_framebuffer() override;
	void prepare_gui() override;

	void on_update_ui_overlay(vkb::Drawer &drawer) override;

	virtual void prepare_pipelines();
	virtual void setup_color();
	virtual void setup_depth_stencil() override;
	virtual void setup_samplers();

  protected:
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
		bool show_stats{true};
	};

	struct UBOVS
	{
		glm::mat4 projection;
		glm::mat4 modelview;
	};

	struct Samplers
	{
		VkSampler nearest{VK_NULL_HANDLE};
	} samplers{};

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

	void         request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;
	virtual void reset_gpu_data();
	virtual bool is_show_stats();

	virtual void update_extents();

	virtual void load_assets(const std::string &scene_file = "scenes/bonza/Bonza.gltf");

	virtual void update_uniform_buffer(float delta_time);

	virtual void prepare_uniform_buffers_main_pass();
	virtual void setup_descriptor_pool_main_pass();
	virtual void setup_additional_descriptor_pool();

	virtual void setup_descriptor_set_layout_main_pass();
	virtual void setup_descriptor_set_main_pass();

	virtual void setup_descriptor_set_layout_present();
	virtual void setup_descriptor_set_present();

	virtual void destroy_pipeline(PipelineData &pipeline_data);
	virtual void destroy_image(ApiVulkanSample::ImageData &image_data);
};