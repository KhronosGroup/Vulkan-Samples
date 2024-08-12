/* Copyright (c) 2024, Sascha Willems
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

#include <ktx.h>

#include "api_vulkan_sample.h"

class HostImageCopy : public ApiVulkanSample
{
  public:
	// Contains all Vulkan objects that are required to store and use a texture
	// Note that this repository contains a texture class (vulkan_texture.h) that encapsulates texture loading functionality in a class that is used in subsequent demos
	struct Texture
	{
		VkSampler      sampler;
		VkImage        image;
		VkDeviceMemory device_memory;
		VkImageView    view;
		uint32_t       width, height;
		uint32_t       mip_levels;
	} texture{};

	std::unique_ptr<vkb::sg::SubMesh> cube;

	struct
	{
		glm::mat4 projection{1.0f};
		glm::mat4 model{1.0f};
		glm::vec4 view_pos{0.0f};
		float     lod_bias = 0.0f;
	} ubo_vs;

	std::unique_ptr<vkb::core::Buffer> uniform_buffer_vs;

	VkPipeline            pipeline{VK_NULL_HANDLE};
	VkPipelineLayout      pipeline_layout{VK_NULL_HANDLE};
	VkDescriptorSet       descriptor_set{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};

	HostImageCopy();
	~HostImageCopy();
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         load_texture();
	void         load_assets();
	void         destroy_texture(Texture texture);
	void         build_command_buffers() override;
	void         draw();
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

std::unique_ptr<vkb::VulkanSampleC> create_host_image_copy();
