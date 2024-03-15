/* Copyright (c) 2021-2024, Holochip
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
 * This sample is demonstrates using a dynamic shading rate map generated each frame based on the
 * frequency content of the previous frame.
 */

#pragma once

#include "api_vulkan_sample.h"

class FragmentShadingRateDynamic : public ApiVulkanSample
{
  public:
	FragmentShadingRateDynamic();
	~FragmentShadingRateDynamic() override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	bool resize(uint32_t new_width, uint32_t new_height) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void render(float delta_time) override;
	void build_command_buffers() override;
	void setup_framebuffer() override;
	void setup_render_pass() override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

  private:
	void create_shading_rate_attachment();
	void invalidate_shading_rate_attachment();
	void load_assets();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void setup_descriptor_set_layout();
	void prepare_pipelines();
	void setup_descriptor_pool();
	void setup_descriptor_sets();
	void create_compute_pipeline();
	void update_compute_pipeline();
	void draw();

	bool enable_attachment_shading_rate = true;
	bool display_sky_sphere             = true;
	bool debug_utils_supported          = false;

	VkPhysicalDeviceFragmentShadingRatePropertiesKHR    physical_device_fragment_shading_rate_properties{};
	std::vector<VkPhysicalDeviceFragmentShadingRateKHR> fragment_shading_rates{};
	VkRenderPass                                        fragment_render_pass{VK_NULL_HANDLE};
	std::vector<VkFramebuffer>                          fragment_framebuffers;
	VkCommandPool                                       command_pool{VK_NULL_HANDLE};

	// Shading rate image is an input to the graphics pipeline
	// and is produced by the "compute shader."
	// It has a lower resolution than the framebuffer
	struct ComputeBuffers
	{
		std::unique_ptr<vkb::core::Image>     shading_rate_image;
		std::unique_ptr<vkb::core::ImageView> shading_rate_image_view;

		// Frequency content image is an output of the graphics pipeline
		// and is consumed by the "compute shader" to produce the shading rate image.
		// It has the same resolution as the framebuffer
		std::unique_ptr<vkb::core::Image>     frequency_content_image;
		std::unique_ptr<vkb::core::ImageView> frequency_content_image_view;

		std::unique_ptr<vkb::core::Image>     shading_rate_image_compute;
		std::unique_ptr<vkb::core::ImageView> shading_rate_image_compute_view;

		VkCommandBuffer command_buffer = VK_NULL_HANDLE;
		VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	};
	std::vector<ComputeBuffers>  compute_buffers;
	std::vector<VkCommandBuffer> small_command_buffers;
	VkExtent2D                   subpass_extent;
	uint32_t                     subpass_extent_ratio = 4;

	VkFence compute_fence{VK_NULL_HANDLE};

	struct FrequencyInformation
	{
		glm::uvec2 frame_dimensions;
		glm::uvec2 shading_rate_dimensions;
		glm::uvec2 max_rates;
		uint32_t   n_rates;
		uint32_t   _pad;
	};
	std::unique_ptr<vkb::core::Buffer> frequency_information_params;

	struct
	{
		VkPipelineLayout      pipeline_layout       = VK_NULL_HANDLE;
		VkPipeline            pipeline              = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
		VkDescriptorPool      descriptor_pool       = VK_NULL_HANDLE;
	} compute;

	struct
	{
		Texture skysphere;
		Texture scene;
	} textures;

	struct
	{
		std::unique_ptr<vkb::sg::SubMesh> skysphere;
		std::unique_ptr<vkb::sg::SubMesh> scene;
	} models;

	struct
	{
		std::unique_ptr<vkb::core::Buffer> scene;
	} uniform_buffers;

	struct UBOScene
	{
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::mat4 skysphere_modelview;
		int32_t   color_shading_rate;
	} ubo_scene;

	VkPipelineLayout pipeline_layout;
	struct Pipelines
	{
		VkPipeline skysphere;
		VkPipeline sphere;
	} pipelines;

	VkDescriptorSetLayout        descriptor_set_layout;
	std::vector<VkDescriptorSet> render_descriptor_sets;

	struct
	{
		glm::vec4 offset;
		uint32_t  object_type;
	} push_const_block;
};

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_fragment_shading_rate_dynamic();
