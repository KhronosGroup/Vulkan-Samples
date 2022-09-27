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
struct HPPSwapchainProperties
{
	vk::SwapchainKHR                old_swapchain;
	uint32_t                        image_count{3};
	vk::Extent2D                    extent{};
	vk::SurfaceFormatKHR            surface_format;
	uint32_t                        array_layers;
	vk::ImageUsageFlags             image_usage;
	vk::SurfaceTransformFlagBitsKHR pre_transform;
	vk::CompositeAlphaFlagBitsKHR   composite_alpha;
	vk::PresentModeKHR              present_mode;
};

class HPPSwapchain : private vkb::Swapchain
{
  public:
	using vkb::Swapchain::create;

	HPPSwapchain(HPPSwapchain &old_swapchain, const vk::Extent2D &extent) :
	    vkb::Swapchain(reinterpret_cast<vkb::Swapchain &>(old_swapchain), static_cast<VkExtent2D>(extent))
	{}

	HPPSwapchain(HPPSwapchain &old_swapchain, const std::set<vk::ImageUsageFlagBits> &image_usage_flags) :
	    vkb::Swapchain(reinterpret_cast<vkb::Swapchain &>(old_swapchain), reinterpret_cast<std::set<VkImageUsageFlagBits> const &>(image_usage_flags))
	{}

	HPPSwapchain(HPPSwapchain &swapchain, const vk::Extent2D &extent, const vk::SurfaceTransformFlagBitsKHR transform) :
	    vkb::Swapchain(reinterpret_cast<vkb::Swapchain &>(swapchain), static_cast<VkExtent2D>(extent), static_cast<VkSurfaceTransformFlagBitsKHR>(transform))
	{}

	HPPSwapchain(HPPDevice &                             device,
	             vk::SurfaceKHR                          surface,
	             const vk::Extent2D &                    extent            = {},
	             const uint32_t                          image_count       = 3,
	             const vk::SurfaceTransformFlagBitsKHR   transform         = vk::SurfaceTransformFlagBitsKHR::eIdentity,
	             const vk::PresentModeKHR                present_mode      = vk::PresentModeKHR::eFifo,
	             const std::set<vk::ImageUsageFlagBits> &image_usage_flags = {vk::ImageUsageFlagBits::eColorAttachment, vk::ImageUsageFlagBits::eTransferSrc}) :
	    vkb::Swapchain(reinterpret_cast<vkb::Device &>(device),
	                   static_cast<VkSurfaceKHR>(surface),
	                   static_cast<VkExtent2D const &>(extent),
	                   image_count,
	                   static_cast<VkSurfaceTransformFlagBitsKHR>(transform),
	                   static_cast<VkPresentModeKHR>(present_mode),
	                   reinterpret_cast<std::set<VkImageUsageFlagBits> const &>(image_usage_flags))
	{}

	vk::Result acquire_next_image(uint32_t &image_index, vk::Semaphore image_acquired_semaphore, vk::Fence fence = nullptr) const
	{
		return static_cast<vk::Result>(vkb::Swapchain::acquire_next_image(image_index, image_acquired_semaphore, fence));
	}

	const vk::Extent2D &get_extent() const
	{
		return reinterpret_cast<vk::Extent2D const &>(vkb::Swapchain::get_extent());
	}

	vk::Format get_format() const
	{
		return static_cast<vk::Format>(vkb::Swapchain::get_format());
	}

	vk::SwapchainKHR get_handle() const
	{
		return vkb::Swapchain::get_handle();
	}

	std::vector<vk::Image> const &get_images() const
	{
		return reinterpret_cast<std::vector<vk::Image> const &>(vkb::Swapchain::get_images());
	}

	HPPSwapchainProperties &get_properties()
	{
		return reinterpret_cast<HPPSwapchainProperties &>(vkb::Swapchain::get_properties());
	}

	vk::SurfaceKHR get_surface() const
	{
		return static_cast<vk::SurfaceKHR>(vkb::Swapchain::get_surface());
	}

	vk::ImageUsageFlags get_usage() const
	{
		return static_cast<vk::ImageUsageFlags>(vkb::Swapchain::get_usage());
	}

	void set_present_mode_priority(const std::vector<vk::PresentModeKHR> &present_mode_priority_list)
	{
		vkb::Swapchain::set_present_mode_priority(reinterpret_cast<std::vector<VkPresentModeKHR> const &>(present_mode_priority_list));
	}

	void set_surface_format_priority(const std::vector<vk::SurfaceFormatKHR> &surface_format_priority_list)
	{
		vkb::Swapchain::set_surface_format_priority(reinterpret_cast<std::vector<VkSurfaceFormatKHR> const &>(surface_format_priority_list));
	}
};
}        // namespace core
}        // namespace vkb
