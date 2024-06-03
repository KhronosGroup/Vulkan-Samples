/* Copyright (c) 2022-2024, Sascha Willems
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
 * Timestamp queries (based on the HDR sample)
 */

#pragma once

#include "api_vulkan_sample.h"

class TimestampQueries : public ApiVulkanSample
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
		glm::mat4 inverse_modelview;
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

	std::vector<std::string> object_names;

	// A query pool is required to use GPU time stamps
	VkQueryPool query_pool_timestamps = VK_NULL_HANDLE;
	// GPU time stamps will be stored in a vector
	std::vector<uint64_t> time_stamps{};

	TimestampQueries();
	~TimestampQueries();
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         build_command_buffers() override;
	void         create_attachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment);
	void         prepare_offscreen_buffer();
	void         load_assets();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_sets();
	void         prepare_pipelines();
	void         prepare_uniform_buffers();
	void         prepare_time_stamp_queries();
	void         get_time_stamp_results();
	void         update_uniform_buffers();
	void         update_params();
	void         draw();
	bool         prepare(const vkb::ApplicationOptions &options) override;
	virtual void render(float delta_time) override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
	virtual bool resize(const uint32_t width, const uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_timestamp_queries();
