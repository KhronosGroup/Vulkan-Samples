/* Copyright (c) 2019-2023, Sascha Willems
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
 * Texture loading (and display) example (including mip maps)
 */

#pragma once

#include <ktx.h>

#include "api_vulkan_sample.h"

// Vertex layout for this example
struct TextureLoadingVertexStructure
{
	float pos[3];
	float uv[2];
	float normal[3];
};

class TextureLoading : public ApiVulkanSample
{
  public:
	// Contains all Vulkan objects that are required to store and use a texture
	// Note that this repository contains a texture class (vulkan_texture.h) that encapsulates texture loading functionality in a class that is used in subsequent demos
	struct Texture
	{
		VkSampler      sampler;
		VkImage        image;
		VkImageLayout  image_layout;
		VkDeviceMemory device_memory;
		VkImageView    view;
		uint32_t       width, height;
		uint32_t       mip_levels;
	} texture;

	std::unique_ptr<vkb::core::Buffer> vertex_buffer;
	std::unique_ptr<vkb::core::Buffer> index_buffer;
	uint32_t                           index_count;

	std::unique_ptr<vkb::core::Buffer> uniform_buffer_vs;

	struct
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 view_pos;
		float     lod_bias = 0.0f;
	} ubo_vs;

	struct
	{
		VkPipeline solid;
	} pipelines;

	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;

	TextureLoading();
	~TextureLoading();
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         load_texture();
	void         destroy_texture(Texture texture);
	void         build_command_buffers() override;
	void         draw();
	void         generate_quad();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_set();
	void         prepare_pipelines();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	bool         prepare(const vkb::ApplicationOptions &options) override;
	virtual void render(float delta_time) override;
	virtual void view_changed() override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
};

std::unique_ptr<vkb::Application> create_texture_loading();
