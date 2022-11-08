/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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
 * High dynamic range rendering, using vulkan.hpp
 */

#pragma once

#include <hpp_api_vulkan_sample.h>

class HPPHDR : public HPPApiVulkanSample
{
  public:
	HPPHDR();
	~HPPHDR();

  private:
	struct DescriptorSetLayouts
	{
		vk::DescriptorSetLayout models;
		vk::DescriptorSetLayout composition;
		vk::DescriptorSetLayout bloom_filter;
	};

	struct DescriptorSets
	{
		vk::DescriptorSet object;
		vk::DescriptorSet skybox;
		vk::DescriptorSet composition;
		vk::DescriptorSet bloom_filter;
	};

	// Framebuffer for offscreen rendering
	struct FrameBufferAttachment
	{
		vk::Image        image;
		vk::DeviceMemory mem;
		vk::ImageView    view;
		vk::Format       format;

		FrameBufferAttachment() = default;

		FrameBufferAttachment(vk::Image image_, vk::DeviceMemory mem_, vk::ImageView view_, vk::Format format_) :
		    image(image_), mem(mem_), view(view_), format(format_)
		{}

		void destroy(vk::Device device)
		{
			device.destroyImageView(view);
			device.destroyImage(image);
			device.freeMemory(mem);
		}
	};

	struct FilterPass
	{
		vk::Extent2D          extent;
		vk::Framebuffer       framebuffer;
		FrameBufferAttachment color[1];
		vk::RenderPass        render_pass;
		vk::Sampler           sampler;
	};

	struct Offscreen
	{
		vk::Extent2D          extent;
		vk::Framebuffer       framebuffer;
		FrameBufferAttachment color[2];
		FrameBufferAttachment depth;
		vk::RenderPass        render_pass;
		vk::Sampler           sampler;
	};

	struct Models
	{
		std::unique_ptr<vkb::scene_graph::components::HPPSubMesh>              skybox;
		std::vector<std::unique_ptr<vkb::scene_graph::components::HPPSubMesh>> objects;
		std::vector<glm::mat4>                                                 transforms;
		int32_t                                                                object_index = 0;
	};

	struct PipelineLayouts
	{
		vk::PipelineLayout models;
		vk::PipelineLayout composition;
		vk::PipelineLayout bloom_filter;
	};

	struct Pipelines
	{
		vk::Pipeline skybox;
		vk::Pipeline reflect;
		vk::Pipeline composition;
		vk::Pipeline bloom[2];
	};

	struct Textures
	{
		HPPTexture envmap;
	};

	struct UBOMatrices
	{
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::mat4 skybox_modelview;
		float     modelscale = 0.05f;
	};

	struct UBOParams
	{
		float exposure = 1.0f;
	};

	struct UniformBuffers
	{
		std::unique_ptr<vkb::core::HPPBuffer> matrices;
		std::unique_ptr<vkb::core::HPPBuffer> params;
	};

  private:
	// from platform::HPPApplication
	virtual bool prepare(vkb::platform::HPPPlatform &platform) override;
	virtual bool resize(const uint32_t width, const uint32_t height) override;

	// from HPPVulkanSample
	virtual void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;

	// from HPPApiVulkanSample
	virtual void build_command_buffers() override;
	virtual void on_update_ui_overlay(vkb::HPPDrawer &drawer) override;
	virtual void render(float delta_time) override;

	FrameBufferAttachment create_attachment(vk::Format format, vk::ImageUsageFlagBits usage);
	void                  draw();
	void                  load_assets();
	void                  prepare_offscreen_buffer();
	void                  prepare_pipelines();
	void                  prepare_uniform_buffers();
	void                  setup_descriptor_pool();
	void                  setup_descriptor_set_layout();
	void                  setup_descriptor_sets();
	void                  update_params();
	void                  update_uniform_buffers();

  private:
	bool                     bloom = true;
	DescriptorSetLayouts     descriptor_set_layouts;
	DescriptorSets           descriptor_sets;
	bool                     display_skybox = true;
	FilterPass               filter_pass;
	Models                   models;
	std::vector<std::string> object_names;
	Offscreen                offscreen;
	PipelineLayouts          pipeline_layouts;
	Pipelines                pipelines;
	Textures                 textures;
	UBOMatrices              ubo_matrices;
	UBOParams                ubo_params;
	UniformBuffers           uniform_buffers;
};

std::unique_ptr<vkb::Application> create_hpp_hdr();
