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
	VkSwapchainKHR                      old_swapchain;
	uint32_t                            image_count{3};
	VkExtent2D                          extent{};
	VkSurfaceFormatKHR                  surface_format{};
	uint32_t                            array_layers;
	VkImageUsageFlags                   image_usage;
	VkSurfaceTransformFlagBitsKHR       pre_transform;
	VkCompositeAlphaFlagBitsKHR         composite_alpha;
	VkPresentModeKHR                    present_mode;
	VkImageCompressionFlagBitsEXT       requested_compression{VK_IMAGE_COMPRESSION_DEFAULT_EXT};
	VkImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate{VK_IMAGE_COMPRESSION_FIXED_RATE_NONE_EXT};

	SwapchainProperties &with_image_count(uint32_t image_count);
	SwapchainProperties &with_extent(const VkExtent2D &extent);
	SwapchainProperties &with_extent_and_transform(const VkExtent2D &extent, VkSurfaceTransformFlagBitsKHR transform);
	SwapchainProperties &with_image_usage(const std::set<VkImageUsageFlagBits> &image_usage);
	SwapchainProperties &with_compression(VkImageCompressionFlagBitsEXT requested_compression, VkImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate);

	SwapchainProperties &validate(vkb::Device &device, VkSurfaceKHR surface);
};

class Swapchain
{
  public:
	/**
	 * @brief Constructor to create a swapchain by changing the extent
	 *        only and preserving the configuration from the old swapchain.
	 */
	Swapchain(const Swapchain &old_swapchain, const VkExtent2D &extent);

	/**
	 * @brief Constructor to create a swapchain by changing the image count
	 *        only and preserving the configuration from the old swapchain.
	 */
	Swapchain(const Swapchain &old_swapchain, const uint32_t image_count);

	/**
	 * @brief Constructor to create a swapchain by changing the image usage
	 *        only and preserving the configuration from the old swapchain.
	 */
	Swapchain(const Swapchain &old_swapchain, const std::set<VkImageUsageFlagBits> &image_usage_flags);

	/**
	 * @brief Constructor to create a swapchain by changing the extent
	 *        and transform only and preserving the configuration from the old swapchain.
	 */
	Swapchain(const Swapchain &swapchain, const VkExtent2D &extent, const VkSurfaceTransformFlagBitsKHR transform);

	/**
	 * @brief Constructor to create a swapchain by changing the compression settings
	 *        only and preserving the configuration from the old swapchain.
	 */
	Swapchain(const Swapchain &swapchain, const VkImageCompressionFlagBitsEXT requested_compression, const VkImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate);

	/**
	 * @brief Constructor to create a swapchain.
	 */
	Swapchain(Device                                   &device,
	          VkSurfaceKHR                              surface,
	          const VkPresentModeKHR                    present_mode,
	          const VkExtent2D                         &extent                           = {},
	          const uint32_t                            image_count                      = 3,
	          const VkSurfaceTransformFlagBitsKHR       transform                        = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
	          const std::set<VkImageUsageFlagBits>     &image_usage_flags                = {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT},
	          const VkImageCompressionFlagBitsEXT       requested_compression            = VK_IMAGE_COMPRESSION_DEFAULT_EXT,
	          const VkImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate = VK_IMAGE_COMPRESSION_FIXED_RATE_NONE_EXT);

	static void                                   set_present_mode_priority_list(const std::vector<VkPresentModeKHR> &present_mode_priority_list);
	static void                                   set_surface_format_priority_list(const std::vector<VkSurfaceFormatKHR> &surface_format_priority_list);
	static const std::vector<VkPresentModeKHR>   &get_present_mode_priority_list();
	static const std::vector<VkSurfaceFormatKHR> &get_surface_format_priority_list();

	Swapchain(const Swapchain &) = delete;

	Swapchain(Swapchain &&other) noexcept;

	~Swapchain();

	Swapchain &operator=(const Swapchain &) = delete;

	Swapchain &operator=(Swapchain &&) = delete;

	bool is_valid() const;

	Device &get_device();

	VkSwapchainKHR get_handle() const;

	VkResult acquire_next_image(uint32_t &image_index, VkSemaphore image_acquired_semaphore, VkFence fence = VK_NULL_HANDLE) const;

	const VkExtent2D &get_extent() const;

	VkFormat get_format() const;

	VkSurfaceFormatKHR get_surface_format() const;

	const std::vector<VkImage> &get_images() const;

	VkSurfaceTransformFlagBitsKHR get_transform() const;

	VkSurfaceKHR get_surface() const;

	VkImageUsageFlags get_usage() const;

	VkPresentModeKHR get_present_mode() const;

	VkImageCompressionFlagsEXT get_applied_compression() const;

	/**
	 * Helper functions for compression controls
	 */

	struct SurfaceFormatCompression
	{
		VkSurfaceFormat2KHR             surface_format{VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR};
		VkImageCompressionPropertiesEXT compression_properties{VK_STRUCTURE_TYPE_IMAGE_COMPRESSION_PROPERTIES_EXT};
	};

	static std::vector<SurfaceFormatCompression> query_supported_fixed_rate_compression(Device &device, const VkSurfaceKHR &surface);

  private:
	static std::vector<VkPresentModeKHR>   present_mode_priority_list;
	static std::vector<VkSurfaceFormatKHR> surface_format_priority_list;

	/**
	 * @brief Constructor to create a swapchain.
	 */
	Swapchain(Device &device, VkSurfaceKHR surface, const SwapchainProperties &initial_properties);

	SwapchainProperties old_swapchain_properties() const;

	Device &device;

	VkSurfaceKHR surface{VK_NULL_HANDLE};

	VkSwapchainKHR handle{VK_NULL_HANDLE};

	std::vector<VkImage> images;

	SwapchainProperties properties;
};
}        // namespace vkb
