/* Copyright (c) 2023, Holochip Inc.
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

#include "rendering/render_pipeline.h"
#include "scene_graph/components/camera.h"
#include "api_vulkan_sample.h"

class video : public ApiVulkanSample
{
	VkPipeline            pipeline;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;

  public:
	video();
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	bool prepare(vkb::Platform &platform) override;
	void         build_command_buffers() override;

	void         draw();
	~video() override;
	void render(float delta_time) override;
};

std::unique_ptr<vkb::VulkanSample> create_video();
