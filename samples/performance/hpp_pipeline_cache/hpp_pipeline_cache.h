/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "imgui.h"
#include "scene_graph/components/camera.h"
#include "vulkan_sample.h"

/**
 * @brief Pipeline creation and caching
 */
class HPPPipelineCache : public vkb::VulkanSample<vkb::BindingType::Cpp>
{
  public:
	HPPPipelineCache();
	virtual ~HPPPipelineCache();

  private:
	// from vkb::VulkanSample
	virtual void draw_gui() override;
	virtual bool prepare(const vkb::ApplicationOptions &options) override;
	virtual void update(float delta_time) override;

  private:
	ImVec2            button_size           = ImVec2(150, 30);
	vkb::sg::Camera  *camera                = nullptr;
	bool              enable_pipeline_cache = true;
	vk::PipelineCache pipeline_cache;
	float             rebuild_pipelines_frame_time_ms = 0.0f;
	bool              record_frame_time_next_frame    = false;
};

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::Cpp>> create_hpp_pipeline_cache();
