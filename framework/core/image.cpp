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

Image ImageBuilder::build(Device &device) const
{
	return Image(device, *this);
}

ImagePtr ImageBuilder::build_unique(Device &device) const
{
	return std::make_unique<Image>(device, *this);
}

Image::Image(vkb::Device          &device,
             const VkExtent3D     &extent,
             VkFormat              format,
             VkImageUsageFlags     image_usage,
             VmaMemoryUsage        memory_usage,
             VkSampleCountFlagBits sample_count,
             const uint32_t        mip_levels,
             const uint32_t        array_layers,
             VkImageTiling         tiling,
             VkImageCreateFlags    flags,
             uint32_t              num_queue_families,
             const uint32_t       *queue_families) :
    // Pass through to the ImageBuilder ctor
    Image(device,
          ImageBuilder(extent)
              .with_format(format)
              .with_image_type(find_image_type(extent))
              .with_usage(image_usage)
              .with_mip_levels(mip_levels)
              .with_array_layers(array_layers)
              .with_tiling(tiling)
              .with_flags(flags)
              .with_vma_usage(memory_usage)
              .with_sample_count(sample_count)
              .with_queue_families(num_queue_families, queue_families)
              .with_implicit_sharing_mode())
{
}

Image::Image(vkb::Device &device, ImageBuilder const &builder) :
    Allocated{builder.alloc_create_info, VK_NULL_HANDLE, &device}, create_info(builder.create_info)
{
	set_handle(create_image(create_info));
	subresource.arrayLayer = create_info.arrayLayers;
	subresource.mipLevel   = create_info.mipLevels;
	if (!builder.debug_name.empty())
	{
		set_debug_name(builder.debug_name);
	}
}

Image::Image(Device &device, VkImage handle, const VkExtent3D &extent, VkFormat format, VkImageUsageFlags image_usage, VkSampleCountFlagBits sample_count) :
    Allocated{handle, &device}
{
	create_info.extent     = extent;
	create_info.imageType  = find_image_type(extent);
	create_info.format     = format;
	create_info.samples    = sample_count;
	create_info.usage      = image_usage;
	subresource.arrayLayer = create_info.arrayLayers = 1;
	subresource.mipLevel = create_info.mipLevels = 1;
}

Image::Image(Image &&other) noexcept :
    Allocated{std::move(other)},
    create_info{std::exchange(other.create_info, {})},
    subresource{std::exchange(other.subresource, {})},
    views(std::exchange(other.views, {}))
{
	// Update image views references to this image to avoid dangling pointers
	for (auto &view : views)
	{
		view->set_image(*this);
	}
}

Image::~Image()
{
	destroy_image(get_handle());
}

VkImageType Image::get_type() const
{
	return create_info.imageType;
}

const VkExtent3D &Image::get_extent() const
{
	return create_info.extent;
}

VkFormat Image::get_format() const
{
	return create_info.format;
}

VkSampleCountFlagBits Image::get_sample_count() const
{
	return create_info.samples;
}

VkImageUsageFlags Image::get_usage() const
{
	return create_info.usage;
}

VkImageTiling Image::get_tiling() const
{
	return create_info.tiling;
}

const VkImageSubresource &Image::get_subresource() const
{
	return subresource;
}

uint32_t Image::get_array_layer_count() const
{
	return create_info.arrayLayers;
}

std::unordered_set<ImageView *> &Image::get_views()
{
	return views;
}

VkDeviceSize Image::get_image_required_size() const
{
	VkMemoryRequirements memory_requirements;

	vkGetImageMemoryRequirements(get_device().get_handle(), get_handle(), &memory_requirements);

	return memory_requirements.size;
}

VkImageCompressionPropertiesEXT Image::get_applied_compression() const
{
	return query_applied_compression(get_device().get_handle(), get_handle());
}
}        // namespace core
}        // namespace vkb
