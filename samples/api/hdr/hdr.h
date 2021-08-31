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
 * High dynamic range rendering
 */

#pragma once

#include "api_vulkan_sample.h"

class HDR : public ApiVulkanSample
{
  public:
	bool bloom          = true;
	bool display_skybox = true;

	struct
	{
		Texture envmap;
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

	struct UBOParams
	{
		float exposure = 1.0f;
	} ubo_params;

	struct
	{
		VkPipeline skybox;
		VkPipeline reflect;
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
		VkDescriptorSet object;
		VkDescriptorSet skybox;
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
		void           destroy(VkDevice device) const
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

	std::vector<std::string> object_names;

	HDR();
	~HDR() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
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
	void         update_params() const;
	void         draw();
	bool         prepare(vkb::Platform &platform) override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void resize(uint32_t width, uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_hdr();
