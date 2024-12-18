/* Copyright (c) 2019-2024, Arm Limited and Contributors
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
#include "rendering/render_pipeline.h"
#include "scene_graph/components/camera.h"
#include "vulkan_sample.h"

/**
 * @brief Using triple buffering over double buffering
 */
class SwapchainImages : public vkb::VulkanSampleC
{
  public:
	SwapchainImages();

	virtual ~SwapchainImages() = default;

	virtual bool prepare(const vkb::ApplicationOptions &options) override;

	virtual void update(float delta_time) override;

  private:
	vkb::sg::Camera *camera{nullptr};

	virtual void draw_gui() override;

	int swapchain_image_count{3};

	int last_swapchain_image_count{3};
};

std::unique_ptr<vkb::VulkanSampleC> create_swapchain_images();
