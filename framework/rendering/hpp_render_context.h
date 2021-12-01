/* Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#include <rendering/render_context.h>

#include <core/hpp_command_buffer.h>
#include <core/hpp_swapchain.h>
#include <rendering/hpp_render_frame.h>

namespace vkb
{
namespace rendering
{
/**
 * @brief facade class around vkb::RenderContext, providing a vulkan.hpp-based interface
 *
 * See vkb::RenderContext for documentation
 */
class HPPRenderContext : protected vkb::RenderContext
{
  public:
	using vkb::RenderContext::get_render_frames;
	using vkb::RenderContext::handle_surface_changes;
	using vkb::RenderContext::has_swapchain;
	using vkb::RenderContext::prepare;

	vkb::core::HPPCommandBuffer &begin(vkb::CommandBuffer::ResetMode reset_mode = vkb::CommandBuffer::ResetMode::ResetPool)
	{
		return reinterpret_cast<vkb::core::HPPCommandBuffer &>(vkb::RenderContext::begin(reset_mode));
	}

	vkb::rendering::HPPRenderFrame &get_active_frame()
	{
		return reinterpret_cast<vkb::rendering::HPPRenderFrame &>(vkb::RenderContext::get_active_frame());
	}

	vk::Format get_format() const
	{
		return static_cast<vk::Format>(vkb::RenderContext::get_format());
	}

	vk::Extent2D const &get_surface_extent() const
	{
		return *reinterpret_cast<vk::Extent2D const *>(&vkb::RenderContext::get_surface_extent());
	}

	vkb::core::HPPSwapchain const &get_swapchain() const
	{
		return reinterpret_cast<vkb::core::HPPSwapchain const &>(vkb::RenderContext::get_swapchain());
	}

	void submit(vkb::core::HPPCommandBuffer &command_buffer)
	{
		vkb::RenderContext::submit(reinterpret_cast<vkb::CommandBuffer &>(command_buffer));
	}

	void update_swapchain(const std::set<vk::ImageUsageFlagBits> &image_usage_flags)
	{
		std::set<VkImageUsageFlagBits> flags;
		for (auto flag : image_usage_flags)
		{
			flags.insert(static_cast<VkImageUsageFlagBits>(flag));
		}
		vkb::RenderContext::update_swapchain(flags);
	}
};
}        // namespace rendering
}        // namespace vkb
