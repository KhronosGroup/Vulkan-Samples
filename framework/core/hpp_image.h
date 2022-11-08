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

#include <core/image.h>

#include <core/hpp_device.h>

namespace vkb
{
namespace core
{
/**
 * @brief facade class around vkb::core::Image, providing a vulkan.hpp-based interface
 *
 * See vkb::core::Image for documentation
 */
class HPPImage : private Image
{
  public:
	using Image::get_array_layer_count;

	HPPImage(HPPDevice const &       device,
	         vk::Image               handle,
	         const vk::Extent3D &    extent,
	         vk::Format              format,
	         vk::ImageUsageFlags     image_usage,
	         vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1) :
	    Image(reinterpret_cast<vkb::Device const &>(device),
	          handle,
	          static_cast<VkExtent3D const &>(extent),
	          static_cast<VkFormat>(format),
	          static_cast<VkImageUsageFlags>(image_usage),
	          static_cast<VkSampleCountFlagBits>(sample_count))
	{}

	HPPImage(HPPDevice const &       device,
	         const vk::Extent3D &    extent,
	         vk::Format              format,
	         vk::ImageUsageFlags     image_usage,
	         VmaMemoryUsage          memory_usage,
	         vk::SampleCountFlagBits sample_count       = vk::SampleCountFlagBits::e1,
	         uint32_t                mip_levels         = 1,
	         uint32_t                array_layers       = 1,
	         vk::ImageTiling         tiling             = vk::ImageTiling::eOptimal,
	         vk::ImageCreateFlags    flags              = {},
	         uint32_t                num_queue_families = 0,
	         const uint32_t *        queue_families     = nullptr) :
	    Image(reinterpret_cast<vkb::Device const &>(device),
	          static_cast<VkExtent3D>(extent),
	          static_cast<VkFormat>(format),
	          static_cast<VkImageUsageFlags>(image_usage),
	          memory_usage,
	          static_cast<VkSampleCountFlagBits>(sample_count),
	          mip_levels,
	          array_layers,
	          static_cast<VkImageTiling>(tiling),
	          static_cast<VkImageCreateFlags>(flags),
	          num_queue_families,
	          queue_families)
	{}

	vk::Image get_handle() const
	{
		return Image::get_handle();
	}
};
}        // namespace core
}        // namespace vkb
