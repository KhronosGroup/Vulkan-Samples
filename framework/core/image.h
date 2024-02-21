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

#include <unordered_set>

#include "common/helpers.h"
#include "common/vk_common.h"
#include "core/allocated.h"
#include "core/vulkan_resource.h"

namespace vkb
{
class Device;

namespace core
{

class Image;
using ImagePtr = std::unique_ptr<Image>;

struct ImageBuilder : public allocated::Builder<ImageBuilder, VkImageCreateInfo>
{
  private:
	using Parent = allocated::Builder<ImageBuilder, VkImageCreateInfo>;

  public:
	ImageBuilder(VkExtent3D const &extent) :
	    Parent(VkImageCreateInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr})
	{
		create_info.extent      = extent;
		create_info.arrayLayers = 1;
		create_info.mipLevels   = 1;
		create_info.imageType   = VK_IMAGE_TYPE_2D;
		create_info.format      = VK_FORMAT_R8G8B8A8_UNORM;
		create_info.samples     = VK_SAMPLE_COUNT_1_BIT;
	}

	ImageBuilder(uint32_t width, uint32_t height = 1, uint32_t depth = 1) :
	    ImageBuilder(VkExtent3D{width, height, depth})
	{
	}

	ImageBuilder &with_format(VkFormat format)
	{
		create_info.format = format;
		return *this;
	}

	ImageBuilder &with_usage(VkImageUsageFlags usage)
	{
		create_info.usage = usage;
		return *this;
	}

	ImageBuilder &with_sharing_mode(VkSharingMode sharing_mode)
	{
		create_info.sharingMode = sharing_mode;
		return *this;
	}

	ImageBuilder &with_flags(VkImageCreateFlags flags)
	{
		create_info.flags = flags;
		return *this;
	}

	ImageBuilder &with_image_type(VkImageType type)
	{
		create_info.imageType = type;
		return *this;
	}

	ImageBuilder &with_array_layers(uint32_t layers)
	{
		create_info.arrayLayers = layers;
		return *this;
	}

	ImageBuilder &with_mip_levels(uint32_t levels)
	{
		create_info.mipLevels = levels;
		return *this;
	}

	ImageBuilder &with_sample_count(VkSampleCountFlagBits sample_count)
	{
		create_info.samples = sample_count;
		return *this;
	}

	ImageBuilder &with_tiling(VkImageTiling tiling)
	{
		create_info.tiling = tiling;
		return *this;
	}

	ImageBuilder &with_implicit_sharing_mode()
	{
		if (create_info.queueFamilyIndexCount != 0)
		{
			create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		}
		return *this;
	}

	template <typename ExtensionType>
	ImageBuilder &with_extension(ExtensionType &extension)
	{
		extension.pNext = create_info.pNext;

		create_info.pNext = &extension;

		return *this;
	}

	Image    build(const Device &device) const;
	ImagePtr build_unique(const Device &device) const;
};

class ImageView;
class Image : public allocated::Allocated<VkImage>
{
	VkImageCreateInfo create_info;

  public:
	Image(Device const         &device,
	      VkImage               handle,
	      const VkExtent3D     &extent,
	      VkFormat              format,
	      VkImageUsageFlags     image_usage,
	      VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT);

	// [[deprecated("Use the ImageBuilder ctor instead")]]
	Image(
	    Device const         &device,
	    const VkExtent3D     &extent,
	    VkFormat              format,
	    VkImageUsageFlags     image_usage,
	    VmaMemoryUsage        memory_usage       = VMA_MEMORY_USAGE_AUTO,
	    VkSampleCountFlagBits sample_count       = VK_SAMPLE_COUNT_1_BIT,
	    uint32_t              mip_levels         = 1,
	    uint32_t              array_layers       = 1,
	    VkImageTiling         tiling             = VK_IMAGE_TILING_OPTIMAL,
	    VkImageCreateFlags    flags              = 0,
	    uint32_t              num_queue_families = 0,
	    const uint32_t       *queue_families     = nullptr);

	Image(Device const &device, ImageBuilder const &builder);

	Image(const Image &) = delete;

	Image(Image &&other) noexcept;

	~Image() override;

	Image &operator=(const Image &) = delete;

	Image &operator=(Image &&) = delete;

	VkImageType get_type() const;

	const VkExtent3D &get_extent() const;

	VkFormat get_format() const;

	VkSampleCountFlagBits get_sample_count() const;

	VkImageUsageFlags get_usage() const;

	VkImageTiling get_tiling() const;

	const VkImageSubresource &get_subresource() const;

	uint32_t get_array_layer_count() const;

	std::unordered_set<ImageView *> &get_views();

	VkDeviceSize get_image_allocated_size() const;

	VkImageCompressionPropertiesEXT get_applied_compression() const;

  private:
	/// Image views referring to this image
	std::unordered_set<ImageView *> views;
	VkImageSubresource              subresource{};
};
}        // namespace core
}        // namespace vkb
