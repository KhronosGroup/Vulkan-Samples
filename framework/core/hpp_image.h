/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "core/hpp_vulkan_resource.h"
#include "hpp_allocated.h"
#include <unordered_set>

namespace vkb
{
namespace core
{
class HPPDevice;
class HPPImageView;
class HPPImage;
using HPPImagePtr = std::unique_ptr<HPPImage>;

struct HPPImageBuilder : public allocated::HPPBuilder<HPPImageBuilder, vk::ImageCreateInfo>
{
  private:
	using Parent = allocated::HPPBuilder<HPPImageBuilder, vk::ImageCreateInfo>;

  public:
	HPPImageBuilder(vk::Extent3D const &extent) :
	    // Better reasonable defaults than vk::ImageCreateInfo default ctor
	    Parent(vk::ImageCreateInfo{{}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, extent, 1, 1})
	{
	}

	HPPImageBuilder(vk::Extent2D const &extent) :
	    HPPImageBuilder(vk::Extent3D{extent.width, extent.height, 1})
	{
	}

	HPPImageBuilder(uint32_t width, uint32_t height = 1, uint32_t depth = 1) :
	    HPPImageBuilder(vk::Extent3D{width, height, depth})
	{
	}

	HPPImageBuilder &with_format(vk::Format format)
	{
		create_info.format = format;
		return *this;
	}

	HPPImageBuilder &with_image_type(vk::ImageType type)
	{
		create_info.imageType = type;
		return *this;
	}

	HPPImageBuilder &with_array_layers(uint32_t layers)
	{
		create_info.arrayLayers = layers;
		return *this;
	}

	HPPImageBuilder &with_mip_levels(uint32_t levels)
	{
		create_info.mipLevels = levels;
		return *this;
	}

	HPPImageBuilder &with_sample_count(vk::SampleCountFlagBits sample_count)
	{
		create_info.samples = sample_count;
		return *this;
	}

	HPPImageBuilder &with_tiling(vk::ImageTiling tiling)
	{
		create_info.tiling = tiling;
		return *this;
	}

	HPPImageBuilder &with_usage(vk::ImageUsageFlags usage)
	{
		create_info.usage = usage;
		return *this;
	}

	HPPImageBuilder &with_flags(vk::ImageCreateFlags flags)
	{
		create_info.flags = flags;
		return *this;
	}

	HPPImageBuilder &with_implicit_sharing_mode()
	{
		if (create_info.queueFamilyIndexCount != 0)
		{
			create_info.sharingMode = vk::SharingMode::eConcurrent;
		}
		return *this;
	}

	HPPImage    build(HPPDevice &device) const;
	HPPImagePtr build_unique(HPPDevice &device) const;
};

class HPPImage : public allocated::HPPAllocated<vk::Image>
{
  public:
	HPPImage(HPPDevice              &device,
	         vk::Image               handle,
	         const vk::Extent3D     &extent,
	         vk::Format              format,
	         vk::ImageUsageFlags     image_usage,
	         vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1);

	//[[deprecated("Use the HPPImageBuilder ctor instead")]]
	HPPImage(HPPDevice              &device,
	         const vk::Extent3D     &extent,
	         vk::Format              format,
	         vk::ImageUsageFlags     image_usage,
	         VmaMemoryUsage          memory_usage       = VMA_MEMORY_USAGE_AUTO,
	         vk::SampleCountFlagBits sample_count       = vk::SampleCountFlagBits::e1,
	         uint32_t                mip_levels         = 1,
	         uint32_t                array_layers       = 1,
	         vk::ImageTiling         tiling             = vk::ImageTiling::eOptimal,
	         vk::ImageCreateFlags    flags              = {},
	         uint32_t                num_queue_families = 0,
	         const uint32_t         *queue_families     = nullptr);

	HPPImage(HPPDevice             &device,
	         HPPImageBuilder const &builder);

	HPPImage(const HPPImage &) = delete;

	HPPImage(HPPImage &&other) noexcept;

	~HPPImage();

	HPPImage &operator=(const HPPImage &) = delete;

	HPPImage &operator=(HPPImage &&) = delete;

	/**
	 * @brief Maps vulkan memory to an host visible address
	 * @return Pointer to host visible memory
	 */
	uint8_t *map();

	vk::ImageType                                  get_type() const;
	const vk::Extent3D                            &get_extent() const;
	vk::Format                                     get_format() const;
	vk::SampleCountFlagBits                        get_sample_count() const;
	vk::ImageUsageFlags                            get_usage() const;
	vk::ImageTiling                                get_tiling() const;
	vk::ImageSubresource                           get_subresource() const;
	uint32_t                                       get_array_layer_count() const;
	std::unordered_set<vkb::core::HPPImageView *> &get_views();

  private:
	vk::ImageCreateInfo                           create_info;
	vk::ImageSubresource                          subresource;
	std::unordered_set<vkb::core::HPPImageView *> views;        /// HPPImage views referring to this image
};
}        // namespace core
}        // namespace vkb
