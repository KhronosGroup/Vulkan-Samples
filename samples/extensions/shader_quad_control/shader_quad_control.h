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

#include "api_vulkan_sample.h"

class ShaderQuadControl : public ApiVulkanSample
{
  public:
	ShaderQuadControl();
	~ShaderQuadControl() override;

	bool prepare(const vkb::ApplicationOptions &options) override;
	void build_command_buffers() override;
	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;
	void render(float delta_time) override;

  private:
	void create_pipeline_layout();
	void create_pipeline();
	void draw();

	VkPipeline       pipeline{VK_NULL_HANDLE};
	VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
};

std::unique_ptr<vkb::VulkanSampleC> create_shader_quad_control();
