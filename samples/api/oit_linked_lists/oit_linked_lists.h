/* Copyright (c) 2023-2024, Google
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

class OITLinkedLists : public ApiVulkanSample
{
  public:
	OITLinkedLists();
	~OITLinkedLists();

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
	void create_fragment_resources(const uint32_t width, const uint32_t height);
	void clear_sized_resources();

	void load_assets();
	void create_constant_buffers();
	void create_descriptors();
	void create_pipelines();

	void update_descriptors();
	void update_scene_constants();
	void fill_instance_data();

  private:
	static constexpr uint32_t kInstanceRowCount    = 4;
	static constexpr uint32_t kInstanceColumnCount = 4;
	static constexpr uint32_t kInstanceLayerCount  = 4;
	static constexpr uint32_t kInstanceCount       = kInstanceRowCount * kInstanceColumnCount * kInstanceLayerCount;

	static constexpr uint32_t kFragmentsPerPixelAverage = 8;

	static constexpr int32_t kSortedFragmentMinCount = 1;
	static constexpr int32_t kSortedFragmentMaxCount = 16;

	static constexpr float kBackgroundGrayscaleMin = 0.0f;
	static constexpr float kBackgroundGrayscaleMax = 1.0f;

	static constexpr uint32_t kLinkedListEndSentinel = 0xFFFFFFFFU;

	struct SceneConstants
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::f32  background_grayscale;
		glm::uint sort_fragments;
		glm::uint fragment_max_count;
		glm::uint sorted_fragment_count;
	};

	struct Instance
	{
		glm::mat4 model;
		glm::vec4 color;
	};

  private:
	std::unique_ptr<vkb::sg::SubMesh> object;
	Texture                           background_texture;

	std::unique_ptr<vkb::core::BufferC> scene_constants;
	std::unique_ptr<vkb::core::BufferC> instance_data;

	std::unique_ptr<vkb::core::Image>     linked_list_head_image;
	std::unique_ptr<vkb::core::ImageView> linked_list_head_image_view;
	std::unique_ptr<vkb::core::BufferC>   fragment_buffer;
	std::unique_ptr<vkb::core::BufferC>   fragment_counter;
	glm::uint                             fragment_max_count = 0U;

	VkRenderPass  gather_render_pass = VK_NULL_HANDLE;
	VkFramebuffer gather_framebuffer = VK_NULL_HANDLE;

	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorPool      descriptor_pool       = VK_NULL_HANDLE;
	VkDescriptorSet       descriptor_set        = VK_NULL_HANDLE;

	VkPipelineLayout pipeline_layout     = VK_NULL_HANDLE;
	VkPipeline       gather_pipeline     = VK_NULL_HANDLE;
	VkPipeline       background_pipeline = VK_NULL_HANDLE;
	VkPipeline       combine_pipeline    = VK_NULL_HANDLE;

	int32_t sort_fragments        = true;
	int32_t camera_auto_rotation  = false;
	int32_t sorted_fragment_count = kSortedFragmentMaxCount;
	float   background_grayscale  = 0.3f;
};

std::unique_ptr<vkb::VulkanSampleC> create_oit_linked_lists();
