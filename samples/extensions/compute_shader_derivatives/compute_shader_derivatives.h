/* Copyright (c) 202, Holochip Inc.
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

#include <string>

#include "api_vulkan_sample.h"

class ComputeShaderDerivatives : public ApiVulkanSample
{
  public:
	ComputeShaderDerivatives();
	~ComputeShaderDerivatives() override;

	void build_command_buffers() override;        // unused, per-frame recording
	bool prepare(const vkb::ApplicationOptions &options) override;
	void render(float delta_time) override;
	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

  private:
	void create_storage_image();
	void create_output_buffer_and_descriptors();
	void create_compute_pipeline();
	void create_graphics_pipeline();

	// Image dimensions for visualization
	static constexpr uint32_t image_width{512};
	static constexpr uint32_t image_height{512};

	// Compute pipeline objects
	VkPipelineLayout compute_pipeline_layout{VK_NULL_HANDLE};
	VkPipeline       compute_pipeline{VK_NULL_HANDLE};

	// Graphics pipeline objects (for displaying the image)
	VkPipelineLayout graphics_pipeline_layout{VK_NULL_HANDLE};
	VkPipeline       graphics_pipeline{VK_NULL_HANDLE};

	// Compute descriptor objects
	VkDescriptorSetLayout compute_descriptor_set_layout{VK_NULL_HANDLE};
	VkDescriptorPool      compute_descriptor_pool{VK_NULL_HANDLE};
	VkDescriptorSet       compute_descriptor_set{VK_NULL_HANDLE};

	// Graphics descriptor objects (for sampling the image)
	VkDescriptorSetLayout graphics_descriptor_set_layout{VK_NULL_HANDLE};
	VkDescriptorPool      graphics_descriptor_pool{VK_NULL_HANDLE};
	VkDescriptorSet       graphics_descriptor_set{VK_NULL_HANDLE};

	// Storage image for compute shader output
	VkImage        storage_image{VK_NULL_HANDLE};
	VkDeviceMemory storage_image_memory{VK_NULL_HANDLE};
	VkImageView    storage_image_view{VK_NULL_HANDLE};
	VkSampler      storage_image_sampler{VK_NULL_HANDLE};
};

std::unique_ptr<vkb::Application> create_compute_shader_derivatives();
