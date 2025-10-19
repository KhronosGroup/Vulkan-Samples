/* Copyright (c) 2025, Holochip Inc.
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
	void create_output_buffer_and_descriptors();
	void create_compute_pipeline();

	// Pipeline objects
	VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
	VkPipeline       compute_pipeline{VK_NULL_HANDLE};

	// Descriptor objects
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
	VkDescriptorPool      descriptor_pool{VK_NULL_HANDLE};
	VkDescriptorSet       descriptor_set{VK_NULL_HANDLE};

	// Result buffer (host-visible)
	VkBuffer       result_buffer{VK_NULL_HANDLE};
	VkDeviceMemory result_memory{VK_NULL_HANDLE};
	uint32_t       result_count{16};         // 4x4 local size
	uint32_t       result_stride{16};        // 3 floats (v,dx,dy) + padding -> 16 bytes
	VkDeviceSize   result_size{result_stride * result_count};
	bool           printed_once{false};

	// Cached log text to show in GUI
	std::string log_text_;
};

std::unique_ptr<vkb::Application> create_compute_shader_derivatives();
