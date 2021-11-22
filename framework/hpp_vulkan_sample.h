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

#include <core/hpp_device.h>
#include <core/hpp_instance.h>
#include <platform/hpp_platform.h>
#include <rendering/hpp_render_context.h>
#include <vulkan_sample.h>

namespace vkb
{
/**
 * @brief facade class around vkb::VulkanSample, providing a vulkan.hpp-based interface
 *
 * See vkb::VulkanSample for documentation
 */
class HPPVulkanSample : public VulkanSample
{
  public:
	void create_render_context(Platform &platform, std::vector<vk::SurfaceFormatKHR> const &surface_priority_list)
	{
		std::vector<VkSurfaceFormatKHR> spl;
		spl.reserve(surface_priority_list.size());
		for (auto const &format : surface_priority_list)
		{
			spl.push_back(static_cast<VkSurfaceFormatKHR>(format));
		}
		render_context = platform.create_render_context(*device, surface, spl);
	}

	vkb::core::HPPDevice const &get_device() const
	{
		return *reinterpret_cast<vkb::core::HPPDevice const *>(&*device);
	}

	vkb::core::HPPInstance const &get_instance() const
	{
		return *reinterpret_cast<vkb::core::HPPInstance const *>(&*instance);
	}

	vkb::platform::HPPPlatform const &get_platform() const
	{
		return *reinterpret_cast<vkb::platform::HPPPlatform const *>(platform);
	}

	vkb::rendering::HPPRenderContext &get_render_context() const
	{
		return *reinterpret_cast<vkb::rendering::HPPRenderContext *>(&*render_context);
	}
};
}        // namespace vkb
