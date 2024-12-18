/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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
 * Separate samplers and image to draw a single image with different sampling options, using vulkan.hpp
 */

#pragma once

#include <hpp_api_vulkan_sample.h>

class HPPSeparateImageSampler : public HPPApiVulkanSample
{
  public:
	HPPSeparateImageSampler();
	~HPPSeparateImageSampler() override;

  private:
	// Vertex layout for this example
	struct VertexStructure
	{
		float pos[3];
		float uv[2];
		float normal[3];
	};

	struct UBO
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 view_pos;
	};

  private:
	// from vkb::Application
	bool prepare(const vkb::ApplicationOptions &options) override;

	// from vkb::VulkanSample
	void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;

	// from HPPApiVulkanSample
	void build_command_buffers() override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void render(float delta_time) override;
	void view_changed() override;

	vk::DescriptorSetLayout create_base_descriptor_set_layout();
	vk::DescriptorPool      create_descriptor_pool();
	vk::Pipeline            create_graphics_pipeline();
	vk::PipelineLayout      create_pipeline_layout(std::vector<vk::DescriptorSetLayout> const &descriptor_set_layouts);
	vk::Sampler             create_sampler(vk::Filter filter);
	vk::DescriptorSetLayout create_sampler_descriptor_set_layout();
	void                    draw();
	void                    generate_quad();
	void                    load_assets();
	void                    prepare_uniform_buffers();
	void                    update_base_descriptor_set();
	void                    update_sampler_descriptor_set(size_t index);
	void                    update_uniform_buffers();

  private:
	vk::DescriptorSet                     base_descriptor_set;
	vk::DescriptorSetLayout               base_descriptor_set_layout;
	std::unique_ptr<vkb::core::BufferCpp> index_buffer;
	uint32_t                              index_count = 0;
	vk::Pipeline                          pipeline;
	vk::PipelineLayout                    pipeline_layout;
	vk::DescriptorSetLayout               sampler_descriptor_set_layout;
	std::array<vk::DescriptorSet, 2>      sampler_descriptor_sets;
	std::array<vk::Sampler, 2>            samplers;
	int32_t                               selected_sampler = 0;
	HPPTexture                            texture;
	UBO                                   ubo_vs;
	std::unique_ptr<vkb::core::BufferCpp> uniform_buffer_vs;
	std::unique_ptr<vkb::core::BufferCpp> vertex_buffer;
};

std::unique_ptr<vkb::Application> create_hpp_separate_image_sampler();
