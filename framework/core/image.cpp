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

#include "image.h"

#include "device.h"
#include "image_view.h"

namespace vkb
{
namespace
{
inline VkImageType find_image_type(VkExtent3D extent)
{
	VkImageType result{};

	uint32_t dim_num{0};

	if (extent.width >= 1)
	{
		dim_num++;
	}

	if (extent.height >= 1)
	{
		dim_num++;
	}

	if (extent.depth > 1)
	{
		dim_num++;
	}

	switch (dim_num)
	{
		case 1:
			result = VK_IMAGE_TYPE_1D;
			break;
		case 2:
			result = VK_IMAGE_TYPE_2D;
			break;
		case 3:
			result = VK_IMAGE_TYPE_3D;
			break;
		default:
			throw std::runtime_error("No image type found.");
			break;
	}

	return result;
}
}        // namespace

namespace core
{
Image::Image(Device &              device,
             const VkExtent3D &    extent,
             VkFormat              format,
             VkImageUsageFlags     image_usage,
             VmaMemoryUsage        memory_usage,
             VkSampleCountFlagBits sample_count,
             const uint32_t        mip_levels,
             const uint32_t        array_layers,
             VkImageTiling         tiling,
             VkImageCreateFlags    flags,
             uint32_t              num_queue_families,
             const uint32_t *      queue_families) :
    device{device},
    type{find_image_type(extent)},
    extent{extent},
    format{format},
    sample_count{sample_count},
    usage{image_usage},
    array_layer_count{array_layers},
    tiling{tiling}
{
	assert(mip_levels > 0 && "Image should have at least one level");
	assert(array_layers > 0 && "Image should have at least one layer");

	subresource.mipLevel   = mip_levels;
	subresource.arrayLayer = array_layers;

	VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	image_info.flags       = flags;
	image_info.imageType   = type;
	image_info.format      = format;
	image_info.extent      = extent;
	image_info.mipLevels   = mip_levels;
	image_info.arrayLayers = array_layers;
	image_info.samples     = sample_count;
	image_info.tiling      = tiling;
	image_info.usage       = image_usage;

	if (num_queue_families != 0)
	{
		image_info.sharingMode           = VK_SHARING_MODE_CONCURRENT;
		image_info.queueFamilyIndexCount = num_queue_families;
		image_info.pQueueFamilyIndices   = queue_families;
	}

	VmaAllocationCreateInfo memory_info{};
	memory_info.usage = memory_usage;

	if (image_usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
	{
		memory_info.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
	}

	auto result = vmaCreateImage(device.get_memory_allocator(),
	                             &image_info, &memory_info,
	                             &handle, &memory,
	                             nullptr);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create Image"};
	}
}

Image::Image(Device &device, VkImage handle, const VkExtent3D &extent, VkFormat format, VkImageUsageFlags image_usage, VkSampleCountFlagBits sample_count) :
    device{device},
    handle{handle},
    type{find_image_type(extent)},
    extent{extent},
    format{format},
    sample_count{sample_count},
    usage{image_usage}
{
	subresource.mipLevel   = 1;
	subresource.arrayLayer = 1;
}

Image::Image(Image &&other) :
    device{other.device},
    handle{other.handle},
    memory{other.memory},
    type{other.type},
    extent{other.extent},
    format{other.format},
    sample_count{other.sample_count},
    usage{other.usage},
    tiling{other.tiling},
    subresource{other.subresource},
    mapped_data{other.mapped_data},
    mapped{other.mapped}
{
	other.handle      = VK_NULL_HANDLE;
	other.memory      = VK_NULL_HANDLE;
	other.mapped_data = nullptr;
	other.mapped      = false;

	// Update image views references to this image to avoid dangling pointers
	for (auto &view : views)
	{
		view->set_image(*this);
	}
}

Image::~Image()
{
	if (handle != VK_NULL_HANDLE && memory != VK_NULL_HANDLE)
	{
		unmap();
		vmaDestroyImage(device.get_memory_allocator(), handle, memory);
	}
}

Device &Image::get_device()
{
	return device;
}

VkImage Image::get_handle() const
{
	return handle;
}

VmaAllocation Image::get_memory() const
{
	return memory;
}

uint8_t *Image::map()
{
	if (!mapped_data)
	{
		if (tiling != VK_IMAGE_TILING_LINEAR)
		{
			LOGW("Mapping image memory that is not linear");
		}
		VK_CHECK(vmaMapMemory(device.get_memory_allocator(), memory, reinterpret_cast<void **>(&mapped_data)));
		mapped = true;
	}
	return mapped_data;
}

void Image::unmap()
{
	if (mapped)
	{
		vmaUnmapMemory(device.get_memory_allocator(), memory);
		mapped_data = nullptr;
		mapped      = false;
	}
}

VkImageType Image::get_type() const
{
	return type;
}

const VkExtent3D &Image::get_extent() const
{
	return extent;
}

VkFormat Image::get_format() const
{
	return format;
}

VkSampleCountFlagBits Image::get_sample_count() const
{
	return sample_count;
}

VkImageUsageFlags Image::get_usage() const
{
	return usage;
}

VkImageTiling Image::get_tiling() const
{
	return tiling;
}

VkImageSubresource Image::get_subresource() const
{
	return subresource;
}

uint32_t Image::get_array_layer_count() const
{
	return array_layer_count;
}

std::unordered_set<ImageView *> &Image::get_views()
{
	return views;
}

}        // namespace core
}        // namespace vkb
