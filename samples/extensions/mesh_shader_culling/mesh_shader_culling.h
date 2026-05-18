/* Copyright (c) 2023-2026, Holochip Corporation
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
 * Demonstrate and showcase a sample application using mesh shader culling features.
 */

#pragma once

#include "api_vulkan_sample.h"

class MeshShaderCulling : public ApiVulkanSample
{
  private:
	int32_t density_level = 2;

	VkPipeline            pipeline              = VK_NULL_HANDLE;
	VkPipelineLayout      pipeline_layout       = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;

	std::array<std::unique_ptr<vkb::core::BufferC>, max_concurrent_frames> uniform_buffers;
	std::array<VkDescriptorSet, max_concurrent_frames>                     descriptor_sets{};

	uint64_t                                       pipeline_stats[max_concurrent_frames][3] = {0};
	std::array<VkQueryPool, max_concurrent_frames> query_pools{};

  public:
	struct UBO
	{
		float cull_center_x   = 2.0f;
		float cull_center_y   = 2.0f;
		float cull_radius     = 1.0f;
		float meshlet_density = 2.0f;
	} ubo_cull{};
	MeshShaderCulling();
	~MeshShaderCulling() override;
	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;
	void build_command_buffer();
	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	void setup_descriptor_sets();
	void prepare_pipelines();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	bool prepare(const vkb::ApplicationOptions &options) override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void setup_query_pools();
};

std::unique_ptr<vkb::VulkanSampleC> create_mesh_shader_culling();
