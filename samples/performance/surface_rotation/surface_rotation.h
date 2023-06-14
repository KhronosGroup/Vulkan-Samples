/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include <iomanip>        // setprecision
#include <sstream>        // stringstream

#include "common/vk_common.h"
#include "rendering/render_pipeline.h"
#include "scene_graph/components/perspective_camera.h"
#include "vulkan_sample.h"

/**
 * @brief Appropriate use of surface rotation
 */
class SurfaceRotation : public vkb::VulkanSample
{
  public:
	SurfaceRotation();

	virtual ~SurfaceRotation() = default;

	virtual bool prepare(const vkb::ApplicationOptions &options) override;

	virtual void update(float delta_time) override;

  private:
	vkb::sg::PerspectiveCamera *camera{nullptr};

	virtual void draw_gui() override;

	void recreate_swapchain();

	bool pre_rotate = false;

	bool last_pre_rotate = false;

	/*
	 * @brief Returns the preTransform value to use when recreating
	 *        the swapchain, which depends on whether or not the
	 *        application is implementing pre-rotation
	 */
	VkSurfaceTransformFlagBitsKHR select_pre_transform();

	/* @brief 180 degree rotations do not trigger a resize, but
	 *        if pre_rotation is enabled a new swapchain
	 *        needs to be created with the corresponding
	 *        preTransform value
	 */
	void handle_no_resize_rotations();
};

std::unique_ptr<vkb::VulkanSample> create_surface_rotation();
