/* Copyright (c) 2023, Holochip Corporation
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
 * Demonstrate and showcase a sample application using mesh shader rendering pipeline.
 */

#pragma once

#include "api_vulkan_sample.h"
#include "glsl_compiler.h"

const float cull_center_delta = 0.01f;

class MeshShadingCulling : public ApiVulkanSample
{
  private:
	struct UBO
	{
		float cull_center_x   = 0.0f;
		float cull_center_y   = 0.0f;
		float cull_radius     = 1.0f;
		float meshlet_density = 1.0f;
	} ubo_cull{};

	std::unique_ptr<vkb::core::Buffer> uniform_buffer{};

  public:
	VkPipeline            pipeline              = VK_NULL_HANDLE;
	VkPipelineLayout      pipeline_layout       = VK_NULL_HANDLE;
	VkDescriptorSet       descriptor_set        = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;

  public:
	MeshShadingCulling();
	~MeshShadingCulling() override;

	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;

	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	void setup_descriptor_sets();
	void prepare_pipelines();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void draw();
	bool prepare(vkb::Platform &platform) override;
	void render(float delta_time) override;

	// TODO: add on_update_ui_overlay()



};

std::unique_ptr<vkb::VulkanSample> create_mesh_shader_culling();
