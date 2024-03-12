/* Copyright (c) 2020-2024, Sascha Willems
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
 * Using variable fragment shading rates from a subpass attachment with VK_KHR_fragment_shading_rate
 * This sample creates an image that contains different shading rates, which are then sampled during rendering
 */

#pragma once

#include "api_vulkan_sample.h"

class FragmentShadingRate : public ApiVulkanSample
{
  public:
	bool enable_attachment_shading_rate = true;
	bool color_shading_rate             = false;
	bool display_skysphere              = true;

	VkPhysicalDeviceFragmentShadingRatePropertiesKHR physical_device_fragment_shading_rate_properties{};
	VkPhysicalDeviceFragmentShadingRateFeaturesKHR   enabled_physical_device_fragment_shading_rate_features{};

	struct ShadingRateImage
	{
		VkImage        image  = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkImageView    view   = VK_NULL_HANDLE;
	} shading_rate_image;

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

	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorSet       descriptor_set;

	struct
	{
		glm::vec4 offset;
		uint32_t  object_type;
	} push_const_block;

	FragmentShadingRate();
	~FragmentShadingRate();
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         create_shading_rate_attachment();
	void         invalidate_shading_rate_attachment();
	void         setup_render_pass() override;
	void         setup_framebuffer() override;
	void         build_command_buffers() override;
	void         load_assets();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_sets();
	void         prepare_pipelines();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	void         draw();
	bool         prepare(const vkb::ApplicationOptions &options) override;
	virtual void render(float delta_time) override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
	virtual bool resize(const uint32_t width, const uint32_t height) override;
};

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_fragment_shading_rate();
