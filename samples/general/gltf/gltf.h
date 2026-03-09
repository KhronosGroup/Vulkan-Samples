/* Copyright (c) 2026, Arm Limited and Contributors
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

#include "gltf_api_vulkan_sample.h"

class GLTF : public GLTFApiVulkanSample
{
  public:
	GLTF();
	~GLTF() override;

	// Override basic framework functionality
	void build_command_buffers() override;
	void render(float delta_time) override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void setup_render_pass() override;
	void prepare_pipelines();
	void setup_framebuffer() override;
	void recreate_swapchain_resources() override;
	void setup_additional_descriptor_pool();
};

std::unique_ptr<vkb::VulkanSampleC> create_gltf();
