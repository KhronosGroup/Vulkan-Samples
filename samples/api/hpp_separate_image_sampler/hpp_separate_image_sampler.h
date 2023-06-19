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
	bool prepare(const vkb::ApplicationOptions &options) override;

	// from HPPVulkanSample
	void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;

	// from HPPApiVulkanSample
	void build_command_buffers() override;
	void on_update_ui_overlay(vkb::HPPDrawer &drawer) override;
	void render(float delta_time) override;
	void view_changed() override;

	void draw();
	void generate_quad();
	void load_assets();
	void prepare_pipelines();
	void prepare_uniform_buffers();
	void setup_descriptor_pool();
	void setup_descriptor_set();
	void setup_descriptor_set_layout();
	void setup_samplers();
	void update_uniform_buffers();

  private:
	vk::DescriptorSet                     base_descriptor_set;
	vk::DescriptorSetLayout               base_descriptor_set_layout;
	std::unique_ptr<vkb::core::HPPBuffer> index_buffer;
	uint32_t                              index_count = 0;
	vk::Pipeline                          pipeline;
	vk::PipelineLayout                    pipeline_layout;
	vk::DescriptorSetLayout               sampler_descriptor_set_layout;
	std::array<vk::DescriptorSet, 2>      sampler_descriptor_sets;
	std::array<vk::Sampler, 2>            samplers;
	int32_t                               selected_sampler = 0;
	HPPTexture                            texture;
	UBO                                   ubo_vs;
	std::unique_ptr<vkb::core::HPPBuffer> uniform_buffer_vs;
	std::unique_ptr<vkb::core::HPPBuffer> vertex_buffer;
};

std::unique_ptr<vkb::Application> create_hpp_separate_image_sampler();
