/* Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
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
 * Texture loading (and display) example (including mip maps), using vulkan.hpp
 */

#pragma once

#include <hpp_api_vulkan_sample.h>

#include <ktx.h>

// Vertex layout for this example
struct HPPTextureLoadingVertexStructure
{
	float pos[3];
	float uv[2];
	float normal[3];
};

class HPPTextureLoading : public HPPApiVulkanSample
{
  public:
	// Contains all Vulkan objects that are required to store and use a texture
	// Note that this repository contains a texture class (vulkan_texture.h) that encapsulates texture loading functionality in a class that is used in subsequent demos
	struct Texture
	{
		vk::Sampler      sampler;
		vk::Image        image;
		vk::ImageLayout  image_layout;
		vk::DeviceMemory device_memory;
		vk::ImageView    view;
		vk::Extent2D     extent;
		uint32_t         mip_levels;
	} texture;

	std::unique_ptr<vkb::core::HPPBuffer> vertex_buffer;
	std::unique_ptr<vkb::core::HPPBuffer> index_buffer;
	uint32_t                              index_count;

	std::unique_ptr<vkb::core::HPPBuffer> uniform_buffer_vs;

	struct
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 view_pos;
		float     lod_bias = 0.0f;
	} ubo_vs;

	vk::Pipeline            pipeline;
	vk::PipelineLayout      pipeline_layout;
	vk::DescriptorSet       descriptor_set;
	vk::DescriptorSetLayout descriptor_set_layout;

	HPPTextureLoading();
	~HPPTextureLoading();
	virtual void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;
	void         load_texture();
	void         destroy_texture(Texture texture);
	void         build_command_buffers() override;
	void         draw();
	void         generate_quad();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_set();
	void         prepare_pipeline();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	bool         prepare(const vkb::ApplicationOptions &options) override;
	virtual void render(float delta_time) override;
	virtual void view_changed() override;
	virtual void on_update_ui_overlay(vkb::HPPDrawer &drawer) override;
};

std::unique_ptr<vkb::Application> create_hpp_texture_loading();
