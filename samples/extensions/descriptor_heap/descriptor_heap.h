/*
 * Copyright (c) 2026 Sascha Willems
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

class DescriptorHeap : public ApiVulkanSample
{
  public:
	DescriptorHeap();
	~DescriptorHeap() override;

	bool prepare(const vkb::ApplicationOptions &options) override;

	void render(float delta_time) override;
	void create_command_pool() override;
	void build_command_buffers() override;
	void build_command_buffer();
	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;
	void setup_render_pass() override;
	void setup_framebuffer() override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

  private:
	bool animate = true;

	static const size_t cube_count{2};
	static const size_t sampler_count{2};

	struct Cube
	{
		Texture   texture;
		glm::vec3 rotation;
	};
	std::array<Cube, cube_count> cubes;

	struct UniformData
	{
		glm::mat4 projection_matrix;
		glm::mat4 view_matrix;
		glm::mat4 model_matrix[cube_count];
	} uniform_data;

	VkPhysicalDeviceDescriptorHeapPropertiesEXT      descriptor_heap_properties{};
	std::unique_ptr<vkb::core::BufferC>              descriptor_heap_resources;
	std::unique_ptr<vkb::core::BufferC>              descriptor_heap_samplers;
	std::vector<std::unique_ptr<vkb::core::BufferC>> uniform_buffers;

	int32_t selected_sampler{0};

	std::unique_ptr<vkb::sg::SubMesh> cube;

	// Size and offset values for heap objects
	VkDeviceSize buffer_heap_offset{0};
	VkDeviceSize buffer_descriptor_size{0};
	VkDeviceSize image_heap_offset{0};
	VkDeviceSize image_descriptor_size{0};
	VkDeviceSize sampler_heap_offset{0};
	VkDeviceSize sampler_descriptor_size{0};

	std::vector<std::string> sampler_names{"Linear", "Nearest"};

	VkPipeline pipeline{nullptr};

	uint32_t get_api_version() const override;

	void load_assets();
	void prepare_uniform_buffers();
	void update_uniform_buffers(float delta_time);
	void create_descriptor_heaps();
	void create_pipeline();
	void draw(float delta_time);
};

std::unique_ptr<vkb::VulkanSampleC> create_descriptor_heap();
