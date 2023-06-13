/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "common/utils.h"
#include "common/vk_common.h"
#include "rendering/render_pipeline.h"
#include "scene_graph/components/camera.h"
#include "vulkan_sample.h"

/**
 * @brief Pipeline creation and caching
 */
class PipelineCache : public vkb::VulkanSample
{
  public:
	PipelineCache();

	virtual ~PipelineCache();

	virtual bool prepare(const vkb::ApplicationOptions &options) override;

	virtual void update(float delta_time) override;

  private:
	vkb::sg::Camera *camera{nullptr};

	VkPipelineCache pipeline_cache{VK_NULL_HANDLE};

	ImVec2 button_size{150, 30};

	bool enable_pipeline_cache{true};

	bool record_frame_time_next_frame{false};

	float rebuild_pipelines_frame_time_ms{0.0f};

	virtual void draw_gui() override;
};

std::unique_ptr<vkb::VulkanSample> create_pipeline_cache();
