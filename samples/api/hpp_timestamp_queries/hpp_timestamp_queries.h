/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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
 * Timestamp queries (based on the HDR sample), using vulkan.hpp
 */

#pragma once

#include "hpp_api_vulkan_sample.h"

class HPPTimestampQueries : public HPPApiVulkanSample
{
  private:
	struct DescriptorSetLayouts
	{
		vk::DescriptorSetLayout bloom_filter;
		vk::DescriptorSetLayout composition;
		vk::DescriptorSetLayout models;
	};

	struct DescriptorSets
	{
		vk::DescriptorSet bloom_filter;
		vk::DescriptorSet composition;
		vk::DescriptorSet object;
		vk::DescriptorSet skybox;
	};

	// Framebuffer for offscreen rendering
	struct FramebufferAttachment
	{
		vk::Format       format;
		vk::Image        image;
		vk::DeviceMemory mem;
		vk::ImageView    view;

		void destroy(vk::Device device)
		{
			device.destroyImageView(view);
			device.destroyImage(image);
			device.freeMemory(mem);
		}
	};

	struct FilterPassData
	{
		FramebufferAttachment color;
		vk::Extent2D          extent;
		vk::Framebuffer       framebuffer;
		vk::RenderPass        render_pass;
		vk::Sampler           sampler;
	};

	struct Models
	{
		int32_t                                                                object_index = 0;
		std::vector<std::unique_ptr<vkb::scene_graph::components::HPPSubMesh>> objects;
		std::unique_ptr<vkb::scene_graph::components::HPPSubMesh>              skybox;
		std::vector<glm::mat4>                                                 transforms;
	};

	struct OffscreenData
	{
		FramebufferAttachment color[2];
		FramebufferAttachment depth;
		vk::Extent2D          extent;
		vk::Framebuffer       framebuffer;
		vk::RenderPass        render_pass;
		vk::Sampler           sampler;
	};

	struct PipelineLayouts
	{
		vk::PipelineLayout bloom_filter;
		vk::PipelineLayout composition;
		vk::PipelineLayout models;
	};

	struct Pipelines
	{
		vk::Pipeline bloom[2];
		vk::Pipeline composition;
		vk::Pipeline reflect;
		vk::Pipeline skybox;
	};

	struct Textures
	{
		HPPTexture envmap;
	};

	struct UBOParams
	{
		float exposure = 1.0f;
	};

	struct UBOs
	{
		std::unique_ptr<vkb::core::HPPBuffer> matrices;
		std::unique_ptr<vkb::core::HPPBuffer> params;
	};

	struct UBOVS
	{
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::mat4 skybox_modelview;
		float     modelscale = 0.05f;
	};

  public:
	HPPTimestampQueries();
	~HPPTimestampQueries();

  private:

	bool prepare(const vkb::ApplicationOptions &options) override;
	bool resize(const uint32_t width, const uint32_t height) override;

	// from HPPVulkanSample
	void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;

	// from HPPApiVulkanSample
	void build_command_buffers() override;
	void on_update_ui_overlay(vkb::HPPDrawer &drawer) override;
	void render(float delta_time) override;

	void create_attachment(vk::Format format, vk::ImageUsageFlagBits usage, FramebufferAttachment *attachment);
	void draw();
	void get_time_stamp_results();
	void load_assets();
	void prepare_descriptor_pool();
	void prepare_descriptor_set_layout();
	void prepare_descriptor_sets();
	void prepare_offscreen_buffer();
	void prepare_pipelines();
	void prepare_uniform_buffers();
	void prepare_time_stamp_queries();
	void update_params();
	void update_uniform_buffers();

  private:
	bool                     bloom = true;
	DescriptorSetLayouts     descriptor_set_layouts;
	DescriptorSets           descriptor_sets;
	bool                     display_skybox = true;
	FilterPassData           filter_pass;
	Models                   models;
	std::vector<std::string> object_names;
	OffscreenData            offscreen;
	PipelineLayouts          pipeline_layouts;
	Pipelines                pipelines;
	Textures                 textures;
	std::array<uint64_t, 6>  time_stamps;                   // GPU time stamps will be stored in an array
	vk::QueryPool            time_stamps_query_pool;        // A query pool is required to use GPU time stamps
	UBOParams                ubo_params;
	UBOVS                    ubo_vs;
	UBOs                     uniform_buffers;
};

std::unique_ptr<vkb::Application> create_hpp_timestamp_queries();
