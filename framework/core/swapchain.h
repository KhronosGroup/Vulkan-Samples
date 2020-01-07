/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "common/helpers.h"
#include "common/vk_common.h"

namespace vkb
{
class Device;

enum ImageFormat
{
	sRGB,
	UNORM
};

struct SwapchainProperties
{
	VkSwapchainKHR                old_swapchain;
	uint32_t                      image_count{3};
	VkExtent2D                    extent{};
	VkSurfaceFormatKHR            surface_format{};
	uint32_t                      array_layers;
	VkImageUsageFlags             image_usage;
	VkSurfaceTransformFlagBitsKHR pre_transform;
	VkCompositeAlphaFlagBitsKHR   composite_alpha;
	VkPresentModeKHR              present_mode;
};

class Swapchain
{
  public:
	/**
	 * @brief Constructor to create a swapchain by changing the extent
	 *        only and preserving the configuration from the old swapchain.
	 */
	Swapchain(Swapchain &old_swapchain, const VkExtent2D &extent);

	/**
	 * @brief Constructor to create a swapchain by changing the image count
	 *        only and preserving the configuration from the old swapchain.
	 */
	Swapchain(Swapchain &old_swapchain, const uint32_t image_count);

	/**
	 * @brief Constructor to create a swapchain by changing the image usage
	 * only and preserving the configuration from the old swapchain.
	 */
	Swapchain(Swapchain &old_swapchain, const std::set<VkImageUsageFlagBits> &image_usage_flags);

	/**
	 * @brief Constructor to create a swapchain by changing the extent
	 *        and transform only and preserving the configuration from the old swapchain.
	 */
	Swapchain(Swapchain &swapchain, const VkExtent2D &extent, const VkSurfaceTransformFlagBitsKHR transform);

	/**
	 * @brief Constructor to create a swapchain.
	 */
	Swapchain(Device &                              device,
	          VkSurfaceKHR                          surface,
	          const VkExtent2D &                    extent            = {},
	          const uint32_t                        image_count       = 3,
	          const VkSurfaceTransformFlagBitsKHR   transform         = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
	          const VkPresentModeKHR                present_mode      = VK_PRESENT_MODE_FIFO_KHR,
	          const std::set<VkImageUsageFlagBits> &image_usage_flags = {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT});

	/**
	 * @brief Constructor to create a swapchain from the old swapchain
	 *        by configuring all parameters.
	 */
	Swapchain(Swapchain &                           old_swapchain,
	          Device &                              device,
	          VkSurfaceKHR                          surface,
	          const VkExtent2D &                    extent            = {},
	          const uint32_t                        image_count       = 3,
	          const VkSurfaceTransformFlagBitsKHR   transform         = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
	          const VkPresentModeKHR                present_mode      = VK_PRESENT_MODE_FIFO_KHR,
	          const std::set<VkImageUsageFlagBits> &image_usage_flags = {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT});

	Swapchain(const Swapchain &) = delete;

	Swapchain(Swapchain &&other);

	~Swapchain();

	Swapchain &operator=(const Swapchain &) = delete;

	Swapchain &operator=(Swapchain &&) = delete;

	void create();

	bool is_valid() const;

	Device &get_device();

	VkSwapchainKHR get_handle() const;

	SwapchainProperties &get_properties();

	VkResult acquire_next_image(uint32_t &image_index, VkSemaphore image_acquired_semaphore, VkFence fence = VK_NULL_HANDLE);

	const VkExtent2D &get_extent() const;

	VkFormat get_format() const;

	const std::vector<VkImage> &get_images() const;

	VkSurfaceTransformFlagBitsKHR get_transform() const;

	VkSurfaceKHR get_surface() const;

	VkImageUsageFlags get_usage() const;

	VkPresentModeKHR get_present_mode() const;

	/**
	 * @brief Sets the order in which the swapchain prioritizes selecting its present mode
	 */
	void set_present_mode_priority(const std::vector<VkPresentModeKHR> &present_mode_priority_list);

	/**
	 * @brief Sets the order in which the swapchain prioritizes selecting its surface format
	 */
	void set_surface_format_priority(const std::vector<VkSurfaceFormatKHR> &surface_format_priority_list);

  private:
	Device &device;

	VkSurfaceKHR surface{VK_NULL_HANDLE};

	VkSwapchainKHR handle{VK_NULL_HANDLE};

	std::vector<VkImage> images;

	std::vector<VkSurfaceFormatKHR> surface_formats{};

	std::vector<VkPresentModeKHR> present_modes{};

	SwapchainProperties properties;

	// A list of present modes in order of priority (vector[0] has high priority, vector[size-1] has low priority)
	std::vector<VkPresentModeKHR> present_mode_priority_list = {
	    VK_PRESENT_MODE_FIFO_KHR,
	    VK_PRESENT_MODE_MAILBOX_KHR};

	// A list of surface formats in order of priority (vector[0] has high priority, vector[size-1] has low priority)
	std::vector<VkSurfaceFormatKHR> surface_format_priority_list = {
	    {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
	    {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
	    {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
	    {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}};

	VkPresentModeKHR present_mode{};

	std::set<VkImageUsageFlagBits> image_usage_flags;
};
}        // namespace vkb
