/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/hpp_image.h"

#include "core/hpp_device.h"

namespace vkb
{
namespace
{
inline vk::ImageType find_image_type(vk::Extent3D const &extent)
{
	uint32_t dim_num = !!extent.width + !!extent.height + (1 < extent.depth);
	switch (dim_num)
	{
		case 1:
			return vk::ImageType::e1D;
		case 2:
			return vk::ImageType::e2D;
		case 3:
			return vk::ImageType::e3D;
		default:
			throw std::runtime_error("No image type found.");
			return vk::ImageType();
	}
}
}        // namespace

namespace core
{
HPPImage::HPPImage(HPPDevice              &device,
                   const vk::Extent3D     &extent,
                   vk::Format              format,
                   vk::ImageUsageFlags     image_usage,
                   VmaMemoryUsage          memory_usage,
                   vk::SampleCountFlagBits sample_count,
                   const uint32_t          mip_levels,
                   const uint32_t          array_layers,
                   vk::ImageTiling         tiling,
                   vk::ImageCreateFlags    flags,
                   uint32_t                num_queue_families,
                   const uint32_t         *queue_families) :
    HPPVulkanResource{nullptr, &device},
    type{find_image_type(extent)},
    extent{extent},
    format{format},
    sample_count{sample_count},
    usage{image_usage},
    array_layer_count{array_layers},
    tiling{tiling}
{
	assert(0 < mip_levels && "HPPImage should have at least one level");
	assert(0 < array_layers && "HPPImage should have at least one layer");

	subresource.mipLevel   = mip_levels;
	subresource.arrayLayer = array_layers;

	vk::ImageCreateInfo image_info(flags, type, format, extent, mip_levels, array_layers, sample_count, tiling, image_usage);

	if (num_queue_families != 0)
	{
		image_info.sharingMode           = vk::SharingMode::eConcurrent;
		image_info.queueFamilyIndexCount = num_queue_families;
		image_info.pQueueFamilyIndices   = queue_families;
	}

	VmaAllocationCreateInfo memory_info{};
	memory_info.usage = memory_usage;

	if (image_usage & vk::ImageUsageFlagBits::eTransientAttachment)
	{
		memory_info.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
	}

	auto result = vmaCreateImage(device.get_memory_allocator(),
	                             reinterpret_cast<VkImageCreateInfo const *>(&image_info),
	                             &memory_info,
	                             const_cast<VkImage *>(reinterpret_cast<VkImage const *>(&get_handle())),
	                             &memory,
	                             nullptr);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create HPPImage"};
	}
}

HPPImage::HPPImage(HPPDevice              &device,
                   vk::Image               handle,
                   const vk::Extent3D     &extent,
                   vk::Format              format,
                   vk::ImageUsageFlags     image_usage,
                   vk::SampleCountFlagBits sample_count) :
    HPPVulkanResource{handle, &device}, type{find_image_type(extent)}, extent{extent}, format{format}, sample_count{sample_count}, usage{image_usage}
{
	subresource.mipLevel   = 1;
	subresource.arrayLayer = 1;
}

HPPImage::HPPImage(HPPImage &&other) :
    HPPVulkanResource{std::move(other)},
    memory(std::exchange(other.memory, {})),
    type(std::exchange(other.type, {})),
    extent(std::exchange(other.extent, {})),
    format(std::exchange(other.format, {})),
    sample_count(std::exchange(other.sample_count, {})),
    usage(std::exchange(other.usage, {})),
    tiling(std::exchange(other.tiling, {})),
    subresource(std::exchange(other.subresource, {})),
    views(std::exchange(other.views, {})),
    mapped_data(std::exchange(other.mapped_data, {})),
    mapped(std::exchange(other.mapped, {}))
{
	// Update image views references to this image to avoid dangling pointers
	for (auto &view : views)
	{
		view->set_image(*this);
	}
}

HPPImage::~HPPImage()
{
	if (get_handle() && memory)
	{
		unmap();
		vmaDestroyImage(get_device().get_memory_allocator(), get_handle(), memory);
	}
}

VmaAllocation HPPImage::get_memory() const
{
	return memory;
}

uint8_t *HPPImage::map()
{
	if (!mapped_data)
	{
		if (tiling != vk::ImageTiling::eLinear)
		{
			LOGW("Mapping image memory that is not linear");
		}
		VK_CHECK(vmaMapMemory(get_device().get_memory_allocator(), memory, reinterpret_cast<void **>(&mapped_data)));
		mapped = true;
	}
	return mapped_data;
}

void HPPImage::unmap()
{
	if (mapped)
	{
		vmaUnmapMemory(get_device().get_memory_allocator(), memory);
		mapped_data = nullptr;
		mapped      = false;
	}
}

vk::ImageType HPPImage::get_type() const
{
	return type;
}

const vk::Extent3D &HPPImage::get_extent() const
{
	return extent;
}

vk::Format HPPImage::get_format() const
{
	return format;
}

vk::SampleCountFlagBits HPPImage::get_sample_count() const
{
	return sample_count;
}

vk::ImageUsageFlags HPPImage::get_usage() const
{
	return usage;
}

vk::ImageTiling HPPImage::get_tiling() const
{
	return tiling;
}

vk::ImageSubresource HPPImage::get_subresource() const
{
	return subresource;
}

uint32_t HPPImage::get_array_layer_count() const
{
	return array_layer_count;
}

std::unordered_set<vkb::core::HPPImageView *> &HPPImage::get_views()
{
	return views;
}

}        // namespace core
}        // namespace vkb
