/* Copyright (c) 2022, Holochip Corporation
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
 * Demonstrates and showcases full_screen_exclusive related functionalities
 */

#pragma once

#include "api_vulkan_sample.h"

class FullScreenExclusive : public ApiVulkanSample
{
  public:
	struct
	{
		Texture skybox_map;
	} textures;

	struct Models
	{
		std::unique_ptr<vkb::sg::SubMesh>              skybox;
		std::vector<std::unique_ptr<vkb::sg::SubMesh>> objects;
		std::vector<glm::mat4>                         transforms;
		int32_t                                        object_index = 0;
	} models;

	struct
	{
		std::unique_ptr<vkb::core::Buffer> matrices;
		std::unique_ptr<vkb::core::Buffer> params;
	} uniform_buffers;

	struct UBOVS
	{
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::mat4 skybox_modelview;
		float     modelscale = 0.05f;
	} ubo_vs;

	// It is the binding 2 from gBuffer.frag, and defined using the same structure,
	// I will most likely change everything back to base.vert/.frag
	// However, the reflection looks real nice
	// probably I will come up with my own shaders.vert and .frag to set exposure as a const
	// so this uniform buffer param and this entire binding 2 will be no longer needed
	// But right now, to just save me a ton of time, I have to make everything look exactly the same
	// corresponding to the gBuffer.frag bindings
	const struct UBOParams
	{
		float exposure = 1.0f;
	} ubo_params;

	struct Pipelines
	{
		VkPipeline skybox;
		VkPipeline reflect;
		VkPipeline composition;
	} pipelines;

	struct PipelineLayouts
	{
		VkPipelineLayout models;
		VkPipelineLayout composition;
	} pipeline_layouts;

	struct DescriptorSets
	{
		VkDescriptorSet object;
		// Add models here:
		VkDescriptorSet object2;
		VkDescriptorSet skybox;
		VkDescriptorSet composition;
	} descriptor_sets;

	struct DescriptorSetLayouts
	{
		VkDescriptorSetLayout models;
		VkDescriptorSetLayout composition;
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

	// ui overlay sample
	float input_box_sample = 1.0f;
	bool  checkbox_sample  = true;

	FullScreenExclusive();
	~FullScreenExclusive();
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;
	void create_attachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment);
	void prepare_offscreen_buffer();
	void load_assets();
	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	void setup_descriptor_sets();
	void prepare_pipelines();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void update_params();
	void draw();
	bool prepare(vkb::Platform &platform) override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void resize(const uint32_t width, const uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_full_screen_exclusive();
