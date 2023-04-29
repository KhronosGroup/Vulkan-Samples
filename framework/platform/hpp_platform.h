/* Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
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

#include <platform/platform.h>

#include <core/hpp_device.h>
#include <platform/hpp_window.h>
#include <rendering/hpp_render_context.h>

namespace vkb
{
namespace platform
{
/**
 * @brief facade class around vkb::Platform, providing a vulkan.hpp-based interface
 *
 * See vkb::Platform for documentation
 */
class HPPPlatform : private vkb::Platform
{
  public:
	using vkb::Platform::get_surface_extension;

	std::unique_ptr<vkb::rendering::HPPRenderContext>
	    create_render_context(vkb::core::HPPDevice &device, vk::SurfaceKHR surface, const std::vector<vk::SurfaceFormatKHR> &surface_format_priority) const
	{
		assert(!surface_format_priority.empty() && "Surface format priority list must contain at least one preferred surface format");

		auto context = std::make_unique<vkb::rendering::HPPRenderContext>(device, surface, reinterpret_cast<vkb::platform::HPPWindow const &>(*window));

		context->set_surface_format_priority(surface_format_priority);

		context->request_image_format(surface_format_priority[0].format);

		context->set_present_mode_priority({vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eImmediate});

		switch (window_properties.vsync)
		{
			case Window::Vsync::ON:
				context->request_present_mode(vk::PresentModeKHR::eFifo);
				break;
			case Window::Vsync::OFF:
			default:
				context->request_present_mode(vk::PresentModeKHR::eMailbox);
				break;
		}

		return std::move(context);
	}

	vkb::platform::HPPWindow &get_window()
	{
		return reinterpret_cast<vkb::platform::HPPWindow &>(vkb::Platform::get_window());
	}

	void on_post_draw(vkb::rendering::HPPRenderContext &context)
	{
		vkb::Platform::on_post_draw(reinterpret_cast<vkb::RenderContext &>(context));
	}
};
}        // namespace platform
}        // namespace vkb
