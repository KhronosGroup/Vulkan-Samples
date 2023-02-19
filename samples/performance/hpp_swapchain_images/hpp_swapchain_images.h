/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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

#include <hpp_vulkan_sample.h>

#include <scene_graph/components/camera.h>

/**
 * @brief Using triple buffering over double buffering, using Vulkan-Hpp
 */
class HPPSwapchainImages : public vkb::HPPVulkanSample
{
  public:
	HPPSwapchainImages();

  private:
	// from vkb::HPPVulkanSample
	virtual void draw_gui() override;
	virtual bool prepare(vkb::platform::HPPPlatform &platform) override;
	virtual void update(float delta_time) override;

  private:
	vkb::sg::Camera *camera{nullptr};
	int              swapchain_image_count{3};
	int              last_swapchain_image_count{3};
};

std::unique_ptr<vkb::Application> create_hpp_swapchain_images();
