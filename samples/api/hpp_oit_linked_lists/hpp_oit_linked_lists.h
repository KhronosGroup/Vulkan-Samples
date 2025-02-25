/* Copyright (c) 2023-2024, NVIDIA
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

#include <hpp_api_vulkan_sample.h>

class HPPOITLinkedLists : public HPPApiVulkanSample
{
  public:
	HPPOITLinkedLists();
	~HPPOITLinkedLists();

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

	void                    clear_sized_resources();
	void                    create_constant_buffers();
	vk::DescriptorPool      create_descriptor_pool();
	vk::DescriptorSetLayout create_descriptor_set_layout();
	void                    create_descriptors();
	void                    create_fragment_resources(vk::Extent2D const &extent);
	void                    create_gather_pass_objects(vk::Extent2D const &extent);
	void                    create_pipelines();
	void                    create_sized_objects(vk::Extent2D const &extent);
	void                    destroy_sized_objects();
	void                    fill_instance_data();
	void                    initialize_camera();
	void                    load_assets();
	void                    update_descriptors();
	void                    update_scene_constants();

  private:
	static constexpr uint32_t kInstanceRowCount    = 4;
	static constexpr uint32_t kInstanceColumnCount = 4;
	static constexpr uint32_t kInstanceLayerCount  = 4;
	static constexpr uint32_t kInstanceCount       = kInstanceRowCount * kInstanceColumnCount * kInstanceLayerCount;

	static constexpr uint32_t kFragmentsPerPixelAverage = 8;

	static constexpr uint32_t kSortedFragmentMinCount = 1;
	static constexpr uint32_t kSortedFragmentMaxCount = 16;

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
	std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> object;
	HPPTexture                                                background_texture;

	std::unique_ptr<vkb::core::BufferCpp> scene_constants;
	std::unique_ptr<vkb::core::BufferCpp> instance_data;

	std::unique_ptr<vkb::core::HPPImage>     linked_list_head_image;
	std::unique_ptr<vkb::core::HPPImageView> linked_list_head_image_view;
	std::unique_ptr<vkb::core::BufferCpp>    fragment_buffer;
	std::unique_ptr<vkb::core::BufferCpp>    fragment_counter;
	glm::uint                                fragment_max_count = 0U;

	vk::RenderPass  gather_render_pass = nullptr;
	vk::Framebuffer gather_framebuffer = nullptr;

	vk::DescriptorSetLayout descriptor_set_layout = nullptr;
	vk::DescriptorPool      descriptor_pool       = nullptr;
	vk::DescriptorSet       descriptor_set        = nullptr;

	vk::PipelineLayout pipeline_layout     = nullptr;
	vk::Pipeline       gather_pipeline     = nullptr;
	vk::Pipeline       background_pipeline = nullptr;
	vk::Pipeline       combine_pipeline    = nullptr;

	bool    sort_fragments        = true;
	bool    camera_auto_rotation  = false;
	int32_t sorted_fragment_count = kSortedFragmentMaxCount;
	float   background_grayscale  = 0.3f;
};

std::unique_ptr<vkb::Application> create_hpp_oit_linked_lists();
