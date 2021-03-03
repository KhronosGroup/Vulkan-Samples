/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

#include <unordered_set>

#include "common/helpers.h"
#include "common/vk_common.h"

namespace vkb
{
class Device;

namespace core
{
class ImageView;
class Image
{
  public:
	Image(Device &              device,
	      VkImage               handle,
	      const VkExtent3D &    extent,
	      VkFormat              format,
	      VkImageUsageFlags     image_usage,
	      VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT);

	Image(Device &              device,
	      const VkExtent3D &    extent,
	      VkFormat              format,
	      VkImageUsageFlags     image_usage,
	      VmaMemoryUsage        memory_usage,
	      VkSampleCountFlagBits sample_count       = VK_SAMPLE_COUNT_1_BIT,
	      uint32_t              mip_levels         = 1,
	      uint32_t              array_layers       = 1,
	      VkImageTiling         tiling             = VK_IMAGE_TILING_OPTIMAL,
	      VkImageCreateFlags    flags              = 0,
	      uint32_t              num_queue_families = 0,
	      const uint32_t *      queue_families     = nullptr);

	Image(const Image &) = delete;

	Image(Image &&other);

	~Image();

	Image &operator=(const Image &) = delete;

	Image &operator=(Image &&) = delete;

	Device &get_device();

	VkImage get_handle() const;

	VmaAllocation get_memory() const;

	/**
	 * @brief Maps vulkan memory to an host visible address
	 * @return Pointer to host visible memory
	 */
	uint8_t *map();

	/**
	 * @brief Unmaps vulkan memory from the host visible address
	 */
	void unmap();

	VkImageType get_type() const;

	const VkExtent3D &get_extent() const;

	VkFormat get_format() const;

	VkSampleCountFlagBits get_sample_count() const;

	VkImageUsageFlags get_usage() const;

	VkImageTiling get_tiling() const;

	VkImageSubresource get_subresource() const;

	uint32_t get_array_layer_count() const;

	std::unordered_set<ImageView *> &get_views();

  private:
	Device &device;

	VkImage handle{VK_NULL_HANDLE};

	VmaAllocation memory{VK_NULL_HANDLE};

	VkImageType type{};

	VkExtent3D extent{};

	VkFormat format{};

	VkImageUsageFlags usage{};

	VkSampleCountFlagBits sample_count{};

	VkImageTiling tiling{};

	VkImageSubresource subresource{};

	uint32_t array_layer_count{0};

	/// Image views referring to this image
	std::unordered_set<ImageView *> views;

	uint8_t *mapped_data{nullptr};

	/// Whether it was mapped with vmaMapMemory
	bool mapped{false};
};
}        // namespace core
}        // namespace vkb
