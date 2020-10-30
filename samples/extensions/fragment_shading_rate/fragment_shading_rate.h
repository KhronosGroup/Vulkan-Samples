/* Copyright (c) 2020, Sascha Willems
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
 * Using variable fragment shading rates with VK_KHR_fragment_shading_rate
 */

#pragma once

#include "api_vulkan_sample.h"

class FragmentShadingRate : public ApiVulkanSample
{
  public:
	bool enable_shading_rate = true;
	bool color_shading_rate  = false;
	bool bloom               = true;
	bool display_skysphere   = true;

	VkPhysicalDeviceFragmentShadingRatePropertiesKHR physical_device_fragment_shading_rate_properties{};
	VkPhysicalDeviceFragmentShadingRateFeaturesKHR   enabled_physical_device_fragment_shading_rate_features{};

	std::vector<VkPhysicalDeviceFragmentShadingRateKHR> fragment_shading_rates{};

	struct ShadingRateImage
	{
		VkImage        image  = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkImageView    view   = VK_NULL_HANDLE;
	} shading_rate_image;

	struct
	{
		Texture skysphere;
	} textures;

	struct
	{
		std::unique_ptr<vkb::sg::SubMesh> skysphere;
		std::unique_ptr<vkb::sg::SubMesh> scene;
	} models;

	struct
	{
		std::unique_ptr<vkb::core::Buffer> matrices;
	} uniform_buffers;

	struct UBOVS
	{
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::mat4 skysphere_modelview;
		float     modelscale = 0.05f;
	} ubo_vs;

	struct
	{
		VkPipeline skysphere;
		VkPipeline sphere;
		VkPipeline composition;
		VkPipeline bloom[2];
	} pipelines;

	struct
	{
		VkPipelineLayout models;
		VkPipelineLayout composition;
		VkPipelineLayout bloom_filter;
	} pipeline_layouts;

	struct
	{
		VkDescriptorSet skysphere;
		VkDescriptorSet sphere;
		VkDescriptorSet composition;
		VkDescriptorSet bloom_filter;
	} descriptor_sets;

	struct
	{
		VkDescriptorSetLayout models;
		VkDescriptorSetLayout composition;
		VkDescriptorSetLayout bloom_filter;
	} descriptor_set_layouts;

	// Framebuffer for offscreen rendering
	struct FrameBufferAttachment
	{
		VkImage        image;
		VkDeviceMemory mem;
		VkImageView    view;
		VkFormat       format;
		void           destroy(VkDevice device)
		{
			vkDestroyImageView(device, view, nullptr);
			vkDestroyImage(device, image, nullptr);
			vkFreeMemory(device, mem, nullptr);
		}
	};
	struct FrameBuffer
	{
		int32_t               width, height;
		VkFramebuffer         framebuffer;
		FrameBufferAttachment color[2];
		FrameBufferAttachment depth;
		VkRenderPass          render_pass;
		VkSampler             sampler;
	} offscreen;

	struct
	{
		int32_t               width, height;
		VkFramebuffer         framebuffer;
		FrameBufferAttachment color[1];
		VkRenderPass          render_pass;
		VkSampler             sampler;
	} filter_pass;

	struct
	{
		glm::vec4 offset;
		glm::vec4 color;
		uint32_t  object_type;
	} push_const_block;

	FragmentShadingRate();
	~FragmentShadingRate();
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         create_shading_rate_attachment();
	void         setup_render_pass() override;
	void         setup_framebuffer() override;
	void         build_command_buffers() override;
	void         create_attachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment);
	void         prepare_offscreen_buffer();
	void         load_assets();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_sets();
	void         prepare_pipelines();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	void         draw();
	bool         prepare(vkb::Platform &platform) override;
	virtual void render(float delta_time) override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
	virtual void resize(const uint32_t width, const uint32_t height) override;
};

std::unique_ptr<vkb::VulkanSample> create_fragment_shading_rate();
