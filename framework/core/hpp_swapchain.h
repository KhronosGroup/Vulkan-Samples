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

#include <core/swapchain.h>

namespace vkb
{
namespace core
{
/**
 * @brief facade class around vkb::Swapchain, providing a vulkan.hpp-based interface
 *
 * See vkb::Swapchain for documentation
 */
class HPPSwapchain : protected vkb::Swapchain
{
  public:
	vk::Result acquire_next_image(uint32_t &image_index, vk::Semaphore image_acquired_semaphore, vk::Fence fence = nullptr) const
	{
		return static_cast<vk::Result>(vkb::Swapchain::acquire_next_image(image_index, image_acquired_semaphore, fence));
	}

	vk::Format get_format() const
	{
		return static_cast<vk::Format>(vkb::Swapchain::get_format());
	}

	vk::SwapchainKHR get_handle() const
	{
		return vkb::Swapchain::get_handle();
	}

	std::vector<VkImage> const &get_images() const
	{
		return vkb::Swapchain::get_images();
	}

	vk::SurfaceKHR get_surface() const
	{
		return static_cast<vk::SurfaceKHR>(vkb::Swapchain::get_surface());
	}
};
}        // namespace core
}        // namespace vkb
