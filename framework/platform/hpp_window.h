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

#include <platform/window.h>

#include <core/hpp_instance.h>

namespace vkb
{
namespace platform
{
/**
 * @brief facade class around vkb::Window, providing a vulkan.hpp-based interface
 *
 * See vkb::Window for documentation
 */
class HPPWindow : private vkb::Window
{
  public:
	using vkb::Window::create_surface;
	using vkb::Window::get_display_present_info;
	using vkb::Window::get_extent;
	using vkb::Window::get_window_mode;

	vk::SurfaceKHR create_surface(vkb::core::HPPInstance &instance)
	{
		return static_cast<vk::SurfaceKHR>(create_surface(reinterpret_cast<vkb::Instance &>(instance)));
	}

	vk::SurfaceKHR create_surface(vk::Instance instance, vk::PhysicalDevice physical_device)
	{
		return static_cast<vk::SurfaceKHR>(create_surface(static_cast<VkInstance>(instance), static_cast<VkPhysicalDevice>(physical_device)));
	}

	bool get_display_present_info(vk::DisplayPresentInfoKHR *info, uint32_t src_width, uint32_t src_height) const
	{
		return get_display_present_info(reinterpret_cast<VkDisplayPresentInfoKHR *>(info), src_width, src_height);
	}
};
}        // namespace platform
}        // namespace vkb
