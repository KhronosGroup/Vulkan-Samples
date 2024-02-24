/* Copyright (c) 2023 Holochip Corporation
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
 * Basic example for VK_EXT_mesh_shader there is only a mesh shader and a fragment shader.
 * The mesh shader creates the vertices for a single triangle.
 */

#pragma once

#include "api_vulkan_sample.h"

class MeshShading : public ApiVulkanSample
{
  public:
	VkPipeline            pipeline;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;

	MeshShading();
	~MeshShading() override;

	void request_gpu_features(vkb::PhysicalDevice &gpu) override;

	void prepare_pipelines();
	void build_command_buffers() override;
	void draw();
	bool prepare(const vkb::ApplicationOptions &options) override;
	void render(float delta_time) override;
};

std::unique_ptr<vkb::VulkanSample> create_mesh_shading();
