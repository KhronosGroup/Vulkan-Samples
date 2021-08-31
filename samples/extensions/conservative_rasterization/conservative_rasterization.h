/* Copyright (c) 2019-2020, Sascha Willems
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
 * Conservative rasterization
 *
 * Note: Requires a device that supports the VK_EXT_conservative_rasterization extension
 *
 * Uses an offscreen buffer with lower resolution to demonstrate the effect of conservative rasterization
 */

#pragma once

#include "api_vulkan_sample.h"

#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM
#define ZOOM_FACTOR 16

class ConservativeRasterization : public ApiVulkanSample
{
  public:
	// Fetch and store conservative rasterization state props for display purposes
	VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservative_raster_properties{};

	bool conservative_raster_enabled = true;

	struct Vertex
	{
		float position[3];
		float color[3];
	};

	struct Triangle
	{
		std::unique_ptr<vkb::core::Buffer> vertices;
		std::unique_ptr<vkb::core::Buffer> indices;
		uint32_t                           index_count;
	} triangle;

	std::unique_ptr<vkb::core::Buffer> uniform_buffer;

	struct UniformBuffers
	{
		std::unique_ptr<vkb::core::Buffer> scene;
	} uniform_buffers;

	struct UboScene
	{
		glm::mat4 projection;
		glm::mat4 model;
	} ubo_scene;

	struct PipelineLayouts
	{
		VkPipelineLayout scene;
		VkPipelineLayout fullscreen;
	} pipeline_layouts;

	struct Pipelines
	{
		VkPipeline triangle;
		VkPipeline triangle_conservative_raster;
		VkPipeline triangle_overlay;
		VkPipeline fullscreen;
	} pipelines;

	struct DescriptorSetLayouts
	{
		VkDescriptorSetLayout scene;
		VkDescriptorSetLayout fullscreen;
	} descriptor_set_layouts;

	struct DescriptorSets
	{
		VkDescriptorSet scene;
		VkDescriptorSet fullscreen;
	} descriptor_sets;

	// Framebuffer for offscreen rendering
	struct FrameBufferAttachment
	{
		VkImage        image;
		VkDeviceMemory mem;
		VkImageView    view;
	};
	struct OffscreenPass
	{
		int32_t               width, height;
		VkFramebuffer         framebuffer;
		FrameBufferAttachment color, depth;
		VkRenderPass          render_pass;
		VkSampler             sampler;
		VkDescriptorImageInfo descriptor;
	} offscreen_pass;

	ConservativeRasterization();
	~ConservativeRasterization() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         build_command_buffers() override;
	void         prepare_offscreen();
	void         load_assets();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_set();
	void         prepare_pipelines();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers_scene();
	void         draw();
	bool         prepare(vkb::Platform &platform) override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
};

std::unique_ptr<vkb::VulkanSample> create_conservative_rasterization();
