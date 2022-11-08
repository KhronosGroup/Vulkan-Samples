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

#include <set>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
class HPPDevice;

struct HPPSwapchainProperties
{
	vk::SwapchainKHR                old_swapchain;
	uint32_t                        image_count{3};
	vk::Extent2D                    extent;
	vk::SurfaceFormatKHR            surface_format;
	uint32_t                        array_layers;
	vk::ImageUsageFlags             image_usage;
	vk::SurfaceTransformFlagBitsKHR pre_transform;
	vk::CompositeAlphaFlagBitsKHR   composite_alpha;
	vk::PresentModeKHR              present_mode;
};

class HPPSwapchain
{
  public:
	/**
	 * @brief Constructor to create a swapchain by changing the extent
	 *        only and preserving the configuration from the old swapchain.
	 */
	HPPSwapchain(HPPSwapchain &old_swapchain, const vk::Extent2D &extent);

	/**
	 * @brief Constructor to create a swapchain by changing the image count
	 *        only and preserving the configuration from the old swapchain.
	 */
	HPPSwapchain(HPPSwapchain &old_swapchain, const uint32_t image_count);

	/**
	 * @brief Constructor to create a swapchain by changing the image usage
	 * only and preserving the configuration from the old swapchain.
	 */
	HPPSwapchain(HPPSwapchain &old_swapchain, const std::set<vk::ImageUsageFlagBits> &image_usage_flags);

	/**
	 * @brief Constructor to create a swapchain by changing the extent
	 *        and transform only and preserving the configuration from the old swapchain.
	 */
	HPPSwapchain(HPPSwapchain &swapchain, const vk::Extent2D &extent, const vk::SurfaceTransformFlagBitsKHR transform);

	/**
	 * @brief Constructor to create a swapchain.
	 */
	HPPSwapchain(HPPDevice &                              device,
	             vk::SurfaceKHR                           surface,
	             const vk::Extent2D &                     extent                       = {},
	             const uint32_t                           image_count                  = 3,
	             const vk::SurfaceTransformFlagBitsKHR    transform                    = vk::SurfaceTransformFlagBitsKHR::eIdentity,
	             const vk::PresentModeKHR                 present_mode                 = vk::PresentModeKHR::eFifo,
	             const std::set<vk::ImageUsageFlagBits> & image_usage_flags            = {vk::ImageUsageFlagBits::eColorAttachment, vk::ImageUsageFlagBits::eTransferSrc},
	             const std::vector<vk::PresentModeKHR> &  present_mode_priority_list   = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox},
	             const std::vector<vk::SurfaceFormatKHR> &surface_format_priority_list = {{vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
	                                                                                      {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}},
	             vk::SwapchainKHR                         old_swapchain                = nullptr);

	HPPSwapchain(const HPPSwapchain &) = delete;

	HPPSwapchain(HPPSwapchain &&other);

	~HPPSwapchain();

	HPPSwapchain &operator=(const HPPSwapchain &) = delete;

	HPPSwapchain &operator=(HPPSwapchain &&) = delete;

	void create();

	bool is_valid() const;

	HPPDevice const &get_device() const;

	vk::SwapchainKHR get_handle() const;

	std::pair<vk::Result, uint32_t> acquire_next_image(vk::Semaphore image_acquired_semaphore, vk::Fence fence = nullptr) const;

	const vk::Extent2D &get_extent() const;

	vk::Format get_format() const;
	void       set_format(vk::Format);

	const std::vector<vk::Image> &get_images() const;

	vk::SurfaceTransformFlagBitsKHR get_transform() const;

	vk::SurfaceKHR get_surface() const;

	vk::ImageUsageFlags get_usage() const;

	vk::PresentModeKHR get_present_mode() const;
	void               set_present_mode(vk::PresentModeKHR present_mode);

	/**
	 * @brief Sets the order in which the swapchain prioritizes selecting its present mode
	 */
	void set_present_mode_priority(const std::vector<vk::PresentModeKHR> &present_mode_priority_list);

	/**
	 * @brief Sets the order in which the swapchain prioritizes selecting its surface format
	 */
	void set_surface_format_priority(const std::vector<vk::SurfaceFormatKHR> &surface_format_priority_list);

  private:
	HPPDevice &device;

	vk::SurfaceKHR surface;

	vk::SwapchainKHR handle;

	std::vector<vk::Image> images;

	std::vector<vk::SurfaceFormatKHR> surface_formats;

	std::vector<vk::PresentModeKHR> present_modes;

	HPPSwapchainProperties properties;

	// A list of present modes in order of priority (vector[0] has high priority, vector[size-1] has low priority)
	std::vector<vk::PresentModeKHR> present_mode_priority_list;

	// A list of surface formats in order of priority (vector[0] has high priority, vector[size-1] has low priority)
	std::vector<vk::SurfaceFormatKHR> surface_format_priority_list;

	std::set<vk::ImageUsageFlagBits> image_usage_flags;
};
}        // namespace core
}        // namespace vkb
