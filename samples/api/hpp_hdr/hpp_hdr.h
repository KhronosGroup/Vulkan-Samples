/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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
	struct Bloom
	{
		bool                    enabled               = true;
		vk::DescriptorSetLayout descriptor_set_layout = {};
		vk::DescriptorSet       descriptor_set;
		vk::PipelineLayout      pipeline_layout;
		vk::Pipeline            pipelines[2];

		void destroy(vk::Device device)
		{
			device.destroyPipeline(pipelines[0]);
			device.destroyPipeline(pipelines[1]);
			device.destroyPipelineLayout(pipeline_layout);
			device.destroyDescriptorSetLayout(descriptor_set_layout);
			// no need to free the descriptor_set, as it's implicitly free'd with the descriptor_pool
		}
	};

	struct Composition
	{
		vk::DescriptorSetLayout descriptor_set_layout = {};
		vk::DescriptorSet       descriptor_set        = {};
		vk::PipelineLayout      pipeline_layout       = {};
		vk::Pipeline            pipeline              = {};

		void destroy(vk::Device device)
		{
			device.destroyPipeline(pipeline);
			device.destroyPipelineLayout(pipeline_layout);
			device.destroyDescriptorSetLayout(descriptor_set_layout);
			// no need to free the descriptor_set, as it's implicitly free'd with the descriptor_pool
		}
	};

	// Framebuffer for offscreen rendering
	struct FrameBufferAttachment
	{
		vk::Format       format = {};
		vk::Image        image  = {};
		vk::DeviceMemory mem    = {};
		vk::ImageView    view   = {};

		void destroy(vk::Device device)
		{
			device.destroyImageView(view);
			device.destroyImage(image);
			device.freeMemory(mem);
		}
	};

	struct FilterPass
	{
		vk::Extent2D          extent      = {};
		vk::Framebuffer       framebuffer = {};
		FrameBufferAttachment color       = {};
		vk::RenderPass        render_pass = {};
		vk::Sampler           sampler     = {};

		void destroy(vk::Device device)
		{
			device.destroySampler(sampler);
			device.destroyFramebuffer(framebuffer);
			device.destroyRenderPass(render_pass);
			color.destroy(device);
		}
	};

	struct Geometry
	{
		vk::DescriptorSet                                                      descriptor_set = {};
		vk::Pipeline                                                           pipeline       = {};
		std::vector<std::unique_ptr<vkb::scene_graph::components::HPPSubMesh>> meshes         = {};

		void destroy(vk::Device device, vk::DescriptorPool descriptor_pool)
		{
			// no need to free the descriptor_set, as it's implicitly free'd with the descriptor_pool
			device.destroyPipeline(pipeline);
		}
	};

	struct Models
	{
		vk::DescriptorSetLayout descriptor_set_layout = {};
		vk::PipelineLayout      pipeline_layout       = {};
		Geometry                objects               = {};
		Geometry                skybox                = {};
		std::vector<glm::mat4>  transforms            = {};
		int32_t                 object_index          = 0;

		void destroy(vk::Device device, vk::DescriptorPool descriptor_pool)
		{
			objects.destroy(device, descriptor_pool);
			skybox.destroy(device, descriptor_pool);
			device.destroyPipelineLayout(pipeline_layout);
			device.destroyDescriptorSetLayout(descriptor_set_layout);
		}
	};

	struct Offscreen
	{
		vk::Extent2D          extent      = {};
		vk::Framebuffer       framebuffer = {};
		FrameBufferAttachment color[2]    = {};
		FrameBufferAttachment depth       = {};
		vk::RenderPass        render_pass = {};
		vk::Sampler           sampler     = {};

		void destroy(vk::Device device)
		{
			device.destroySampler(sampler);
			device.destroyFramebuffer(framebuffer);
			device.destroyRenderPass(render_pass);
			color[0].destroy(device);
			color[1].destroy(device);
			depth.destroy(device);
		}
	};

	struct Textures
	{
		HPPTexture envmap;

		void destroy(vk::Device device)
		{
			device.destroySampler(envmap.sampler);
		}
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
	// from vkb::Application
	virtual bool prepare(const vkb::ApplicationOptions &options) override;
	virtual bool resize(const uint32_t width, const uint32_t height) override;

	// from HPPVulkanSample
	virtual void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;

	// from HPPApiVulkanSample
	virtual void build_command_buffers() override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
	virtual void render(float delta_time) override;

	vk::DeviceMemory      allocate_memory(vk::Image image);
	FrameBufferAttachment create_attachment(vk::Format format, vk::ImageUsageFlagBits usage);
	vk::DescriptorPool    create_descriptor_pool();
	vk::Pipeline          create_bloom_pipeline(uint32_t direction);
	vk::Pipeline          create_composition_pipeline();
	vk::RenderPass        create_filter_render_pass();
	vk::Image             create_image(vk::Format format, vk::ImageUsageFlagBits usage);
	vk::ImageView         create_image_view(vk::Format format, vk::ImageUsageFlagBits usage, vk::Image image);
	vk::Pipeline          create_models_pipeline(uint32_t shaderType, vk::CullModeFlagBits cullMode, bool depthTestAndWrite);
	vk::RenderPass        create_offscreen_render_pass();
	vk::RenderPass        create_render_pass(std::vector<vk::AttachmentDescription> const &attachment_descriptions, vk::SubpassDescription const &subpass_description);
	vk::Sampler           create_sampler();
	void                  draw();
	void                  load_assets();
	void                  prepare_camera();
	void                  prepare_offscreen_buffer();
	void                  prepare_uniform_buffers();
	void                  setup_bloom();
	void                  setup_composition();
	void                  setup_models();
	void                  update_composition_descriptor_set();
	void                  update_bloom_descriptor_set();
	void                  update_model_descriptor_set(vk::DescriptorSet descriptor_set);
	void                  update_params();
	void                  update_uniform_buffers();

  private:
	Bloom                    bloom;
	Composition              composition;
	bool                     display_skybox = true;
	FilterPass               filter_pass;
	Models                   models;
	std::vector<std::string> object_names;
	Offscreen                offscreen;
	Textures                 textures;
	UBOMatrices              ubo_matrices;
	UBOParams                ubo_params;
	UniformBuffers           uniform_buffers;
};

std::unique_ptr<vkb::Application> create_hpp_hdr();
