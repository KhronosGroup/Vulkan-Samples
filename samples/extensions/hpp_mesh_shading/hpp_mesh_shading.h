/* Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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
 * Basic example for VK_EXT_mesh_shader, using Vulkan-Hpp.
 * There is only a mesh shader and a fragment shader. The mesh shader creates the vertices for a single triangle.
 */

#pragma once

#include <hpp_api_vulkan_sample.h>

// #include "api_vulkan_sample.h"
// #include "glsl_compiler.h"

class HPPMeshShading : public HPPApiVulkanSample
{
  public:
	HPPMeshShading();
	~HPPMeshShading() override;

  private:
	// from vkb::Application
	bool prepare(const vkb::ApplicationOptions &options) override;

	// from vkb::VulkanSample
	void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;

	// from HPPApiVulkanSample
	void build_command_buffers() override;
	void render(float delta_time) override;

	vk::Pipeline create_pipeline();
	void         draw();

  private:
	vk::Pipeline            pipeline              = nullptr;
	vk::PipelineLayout      pipeline_layout       = nullptr;
	vk::DescriptorSet       descriptor_set        = nullptr;
	vk::DescriptorSetLayout descriptor_set_layout = nullptr;
};

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::Cpp>> create_hpp_mesh_shading();
