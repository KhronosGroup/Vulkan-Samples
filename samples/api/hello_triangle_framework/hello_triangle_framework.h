/* Copyright (c) 2023, Mobica Limited
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
#include "common/vk_common.h"
#include "core/instance.h"
#include "platform/application.h"

/**
 * @brief Triangle rendering using framework
 */
class HelloTriangleFramework : public ApiVulkanSample
{
  public:
	HelloTriangleFramework();
	virtual ~HelloTriangleFramework();

	void prepare_pipelines();

	// Override basic functionality
	void build_command_buffers() override;
	void render(float delta_time) override;
	bool prepare(vkb::Platform &platform) override;

  private:
	// Sample specific data
	VkPipeline       triangle_pipeline{};
	VkPipelineLayout triangle_pipeline_layout{};
};

std::unique_ptr<vkb::Application> create_hello_triangle_framework();
