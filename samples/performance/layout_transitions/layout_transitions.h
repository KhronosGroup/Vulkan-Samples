/* Copyright (c) 2019-2026, Arm Limited and Contributors
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
 * @brief Transitioning images from UNDEFINED vs last known layout
 */
class LayoutTransitions : public vkb::VulkanSampleC
{
  public:
	LayoutTransitions();

	virtual ~LayoutTransitions() = default;

	bool prepare(const vkb::ApplicationOptions &options) override;
#if defined(PLATFORM__MACOS) && TARGET_OS_IOS && TARGET_OS_SIMULATOR
	void request_instance_extensions(std::unordered_map<std::string, vkb::RequestMode> &requested_extensions) const override;
#endif

  private:
	enum LayoutTransitionType : int
	{
		UNDEFINED,
		LAST_LAYOUT
	};

	vkb::sg::Camera *camera{nullptr};

	std::unique_ptr<vkb::RenderTarget> create_render_target(vkb::core::Image &&swapchain_image);

	virtual void prepare_render_context() override;

	void draw(vkb::core::CommandBufferC &command_buffer, vkb::RenderTarget &render_target) override;

	virtual void draw_gui() override;

	VkImageLayout pick_old_layout(VkImageLayout last_layout);

	vkb::RenderPipeline gbuffer_pipeline;

	vkb::RenderPipeline lighting_pipeline;

	LayoutTransitionType layout_transition_type{LayoutTransitionType::UNDEFINED};
};

std::unique_ptr<vkb::VulkanSampleC> create_layout_transitions();
