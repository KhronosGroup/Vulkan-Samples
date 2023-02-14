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

class MeshShadingCulling : public ApiVulkanSample
{
  public:
	VkPipeline            pipeline;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;

	MeshShadingCulling();
	~MeshShadingCulling() override;

	void request_gpu_features(vkb::PhysicalDevice &gpu) override;

	void prepare_pipelines();
	void build_command_buffers() override;
	void draw();
	bool prepare(vkb::Platform &platform) override;
	void render(float delta_time) override;
};

std::unique_ptr<vkb::VulkanSample> create_mesh_shader_culling();
