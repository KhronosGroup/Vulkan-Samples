/* Copyright (c) 2024, Google
 * Copyright (c) 2024, NVIDIA
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

#pragma once

#include "hpp_api_vulkan_sample.h"

class HPPOITDepthPeeling : public HPPApiVulkanSample
{
  public:
	HPPOITDepthPeeling();
	~HPPOITDepthPeeling();

  private:
	// from vkb::Application
	bool prepare(const vkb::ApplicationOptions &options) override;
	bool resize(const uint32_t width, const uint32_t height) override;

	// from vkb::VulkanSample
	void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;

	// from HPPApiVulkanSample
	void build_command_buffers() override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void render(float delta_time) override;

	void create_background_pipeline();
	void create_combine_pass();
	void create_combine_pass_pipeline();
	void create_descriptor_pool();
	void create_gather_pass();
	void create_gather_pass_depth_descriptor_sets();
	void create_gather_pass_descriptor_set_layout();
	void create_gather_pass_framebuffers(const uint32_t width, const uint32_t height);
	void create_gather_pass_pipelines();
	void create_gather_pass_render_pass();
	void create_images(const uint32_t width, const uint32_t height);
	void create_point_sampler();
	void create_scene_constants_buffer();
	void load_assets();
	void prepare_camera();
	void update_descriptors();
	void update_scene_constants();

  private:
	static constexpr uint32_t kLayerMaxCount = 8;

	static constexpr uint32_t kDepthCount = 2;

	static constexpr float kBackgroundGrayscaleMin = 0.0f;
	static constexpr float kBackgroundGrayscaleMax = 1.0f;

	static constexpr float kObjectAlphaMin = 0.0f;
	static constexpr float kObjectAlphaMax = 1.0f;

	struct Background
	{
		vk::Pipeline pipeline = {};
		HPPTexture   texture  = {};

		void destroy(vk::Device device)
		{
			device.destroyPipeline(pipeline);
			device.destroySampler(texture.sampler);
		}
	};

	struct CombinePass
	{
		vk::DescriptorSet       descriptor_set        = {};
		vk::DescriptorSetLayout descriptor_set_layout = {};
		vk::Pipeline            pipeline              = {};
		vk::PipelineLayout      pipeline_layout       = {};

		void destroy(vk::Device device)
		{
			// descriptor_set is implicitly destroyed when the managing descriptor_pool is destroyed
			device.destroyDescriptorSetLayout(descriptor_set_layout);
			device.destroyPipeline(pipeline);
			device.destroyPipelineLayout(pipeline_layout);

			descriptor_set_layout = nullptr;
			pipeline              = nullptr;
			pipeline_layout       = nullptr;
		}
	};

	struct Depth
	{
		vk::DescriptorSet                        gather_descriptor_set = {};
		std::unique_ptr<vkb::core::HPPImage>     image                 = {};
		std::unique_ptr<vkb::core::HPPImageView> image_view            = {};

		void destroy()
		{
			image_view.reset();
			image.reset();
		}
	};

	struct GatherPass
	{
		vk::DescriptorSetLayout descriptor_set_layout = {};
		vk::Pipeline            first_pipeline        = {};
		vk::Pipeline            pipeline              = {};
		vk::PipelineLayout      pipeline_layout       = {};
		vk::RenderPass          render_pass           = {};

		void destroy(vk::Device device)
		{
			device.destroyDescriptorSetLayout(descriptor_set_layout);
			device.destroyPipeline(first_pipeline);
			device.destroyPipeline(pipeline);
			device.destroyPipelineLayout(pipeline_layout);
			device.destroyRenderPass(render_pass);

			descriptor_set_layout = nullptr;
			first_pipeline        = nullptr;
			pipeline              = nullptr;
			pipeline_layout       = nullptr;
			render_pass           = nullptr;
		}
	};

	struct GUI
	{
		float   background_grayscale = 0.3f;
		int32_t camera_auto_rotation = false;
		int32_t layer_index_back     = kLayerMaxCount - 1;
		int32_t layer_index_front    = 0;
		float   object_opacity       = 0.5f;
	};

	struct Layer
	{
		vk::Framebuffer                          gather_framebuffer = {};
		std::unique_ptr<vkb::core::HPPImage>     image              = {};
		std::unique_ptr<vkb::core::HPPImageView> image_view         = {};

		void destroy(vk::Device device)
		{
			device.destroyFramebuffer(gather_framebuffer);
			image_view.reset();
			image.reset();

			gather_framebuffer = nullptr;
		}
	};

	struct SceneConstants
	{
		glm::mat4  model_view_projection = {};
		glm::f32   background_grayscale  = {};
		glm::f32   object_opacity        = {};
		glm::int32 front_layer_index     = {};
		glm::int32 back_layer_index      = {};
	};

  private:
	Background                                                background      = {};
	CombinePass                                               combinePass     = {};
	std::array<Depth, kDepthCount>                            depths          = {};
	vk::DescriptorPool                                        descriptor_pool = {};
	GatherPass                                                gatherPass      = {};
	GUI                                                       gui             = {};
	std::array<Layer, kLayerMaxCount>                         layers          = {};
	std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> model           = {};
	vk::Sampler                                               point_sampler   = {};
	std::unique_ptr<vkb::core::HPPBuffer>                     scene_constants = {};
};

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::Cpp>> create_hpp_oit_depth_peeling();
