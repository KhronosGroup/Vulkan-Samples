/* Copyright (c) 2026, Holochip Inc
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
#include <string>

// Minimal sample that demonstrates VK_KHR_pipeline_binary usage by creating a
// trivial compute pipeline, querying the pipeline key, and (if supported)
// capturing its pipeline binary.
class PipelineBinary : public ApiVulkanSample
{
  public:
	PipelineBinary();
	~PipelineBinary() override;

	void build_command_buffers() override;
	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

  private:
	// Resources for a minimal compute pipeline used for demonstrating pipeline binaries
	VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
	VkPipeline       compute_pipeline{VK_NULL_HANDLE};
	VkShaderModule   compute_shader{VK_NULL_HANDLE};

	// Cached shader stage and pipeline create info for reuse (avoid reloading/rebuilding)
	VkPipelineShaderStageCreateInfo compute_shader_stage{};
	VkComputePipelineCreateInfo     compute_ci_cache{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};

	// Pipeline binary objects
	VkPipelineBinaryKHR pipeline_binary{VK_NULL_HANDLE};

	// Aggregated UI log text shown in the overlay
	std::string log_text_{};

	// Binary data storage for save/load operations
	std::vector<uint8_t>   binary_data_{};
	VkPipelineBinaryKeyKHR binary_key_{VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR};
	size_t                 binary_size_{0};

	// Performance timing measurements (in milliseconds)
	float last_create_time_ms_{0.0f};
	float last_binary_create_time_ms_{0.0f};
	int   creation_count_{0};
	int   binary_creation_count_{0};

	// File path for binary persistence
	std::string binary_file_path_{"pipeline_binary.bin"};
	std::string status_message_{};

	// State flags
	bool binary_available_{false};
	bool binary_file_exists_{false};

	// Internal helpers
	void create_compute_pipeline();
	void recreate_pipeline_from_scratch();
	void recreate_pipeline_from_binary();
	void save_binary_to_file();
	void load_binary_from_file();
	bool check_binary_file_exists();

	// Demonstrations of VK_KHR_pipeline_binary
	void log_pipeline_binary_support();
	void demo_pipeline_key_and_binary();
};

std::unique_ptr<vkb::Application> create_pipeline_binary();
