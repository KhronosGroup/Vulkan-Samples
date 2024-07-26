/* Copyright (c) 2024, Google
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

#include "api_vulkan_sample.h"
#include "rendering/render_pipeline.h"
#include "scene_graph/components/camera.h"

class OITDepthPeeling : public ApiVulkanSample
{
  public:
	OITDepthPeeling();
	~OITDepthPeeling();

	bool prepare(const vkb::ApplicationOptions &options) override;
	bool resize(const uint32_t width, const uint32_t height) override;
	void render(float delta_time) override;
	void build_command_buffers() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

  private:
	void create_sized_objects(const uint32_t width, const uint32_t height);
	void destroy_sized_objects();
	void create_gather_pass_objects(const uint32_t width, const uint32_t height);
	void create_images(const uint32_t width, const uint32_t height);

	void load_assets();
	void create_samplers();
	void create_constant_buffers();
	void create_descriptors();
	void create_pipelines();

	void update_descriptors();
	void update_scene_constants();

  private:
	static constexpr uint32_t kLayerMaxCount = 8;

	static constexpr uint32_t kDepthCount = 2;

	static constexpr float kBackgroundGrayscaleMin = 0.0f;
	static constexpr float kBackgroundGrayscaleMax = 1.0f;

	static constexpr float kObjectAlphaMin = 0.0f;
	static constexpr float kObjectAlphaMax = 1.0f;

	struct SceneConstants
	{
		glm::mat4  model_view_projection;
		glm::f32   background_grayscale;
		glm::f32   object_alpha;
		glm::int32 front_layer_index;
		glm::int32 back_layer_index;
	};

  private:
	std::unique_ptr<vkb::sg::SubMesh> object;
	Texture                           background_texture;

	std::unique_ptr<vkb::core::Buffer> scene_constants;

	VkSampler point_sampler = VK_NULL_HANDLE;

	std::unique_ptr<vkb::core::Image>     layer_image[kLayerMaxCount];
	std::unique_ptr<vkb::core::ImageView> layer_image_view[kLayerMaxCount];

	std::unique_ptr<vkb::core::Image>     depth_image[kDepthCount];
	std::unique_ptr<vkb::core::ImageView> depth_image_view[kDepthCount];

	VkRenderPass  gather_render_pass                 = VK_NULL_HANDLE;
	VkFramebuffer gather_framebuffer[kLayerMaxCount] = {};

	VkDescriptorSetLayout gather_descriptor_set_layout       = VK_NULL_HANDLE;
	VkDescriptorSet       gather_descriptor_set[kDepthCount] = {};
	VkDescriptorSetLayout combine_descriptor_set_layout      = VK_NULL_HANDLE;
	VkDescriptorSet       combine_descriptor_set             = VK_NULL_HANDLE;

	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

	VkPipelineLayout gather_pipeline_layout;
	VkPipeline       gather_first_pipeline;
	VkPipeline       gather_pipeline;
	VkPipelineLayout combine_pipeline_layout;
	VkPipeline       combine_pipeline;
	VkPipeline       background_pipeline;

	int32_t camera_auto_rotation = false;
	float   background_grayscale = 0.3f;
	float   object_alpha         = 0.5f;
	int32_t front_layer_index    = 0;
	int32_t back_layer_index     = kLayerMaxCount - 1;
};

std::unique_ptr<vkb::VulkanSampleC> create_oit_depth_peeling();
