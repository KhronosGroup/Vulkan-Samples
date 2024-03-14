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

#include "rendering/render_pipeline.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/perspective_camera.h"
#include "vulkan_sample.h"

class WaitIdle : public vkb::VulkanSample<vkb::BindingType::C>
{
  public:
	WaitIdle();

	virtual ~WaitIdle() = default;

	virtual bool prepare(const vkb::ApplicationOptions &options) override;

	/**
	 * @brief This RenderContext is responsible containing the scene's RenderFrames
	 *		  It implements a custom wait_frame function which alternates between waiting with WaitIdle or Fences
	 */
	class CustomRenderContext : public vkb::RenderContext
	{
	  public:
		CustomRenderContext(vkb::Device &device, VkSurfaceKHR surface, const vkb::Window &window, int &wait_idle_enabled);

		virtual void wait_frame() override;

	  private:
		int &wait_idle_enabled;
	};

	virtual void prepare_render_context() override;

  private:
	vkb::sg::PerspectiveCamera *camera{nullptr};

	virtual void draw_gui() override;

	int wait_idle_enabled{0};
};

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_wait_idle();
