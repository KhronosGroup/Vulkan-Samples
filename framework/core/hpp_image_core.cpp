/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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

HPPImage HPPImageBuilder::build(HPPDevice &device) const
{
	return HPPImage(device, *this);
}

HPPImagePtr HPPImageBuilder::build_unique(HPPDevice &device) const
{
	return std::make_unique<HPPImage>(device, *this);
}

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
    HPPImage{device,
             HPPImageBuilder{extent}
                 .with_format(format)
                 .with_mip_levels(mip_levels)
                 .with_array_layers(array_layers)
                 .with_sample_count(sample_count)
                 .with_tiling(tiling)
                 .with_flags(flags)
                 .with_usage(image_usage)
                 .with_queue_families(num_queue_families, queue_families)}
{}

HPPImage::HPPImage(HPPDevice &device, HPPImageBuilder const &builder) :
    HPPAllocated{builder.alloc_create_info, nullptr, &device}, create_info{builder.create_info}
{
	get_handle()           = create_image(create_info.operator const VkImageCreateInfo &());
	subresource.arrayLayer = create_info.arrayLayers;
	subresource.mipLevel   = create_info.mipLevels;
	if (!builder.debug_name.empty())
	{
		set_debug_name(builder.debug_name);
	}
}

HPPImage::HPPImage(HPPDevice              &device,
                   vk::Image               handle,
                   const vk::Extent3D     &extent,
                   vk::Format              format,
                   vk::ImageUsageFlags     image_usage,
                   vk::SampleCountFlagBits sample_count) :
    HPPAllocated{handle, &device}
{
	create_info.samples     = sample_count;
	create_info.format      = format;
	create_info.extent      = extent;
	create_info.imageType   = find_image_type(extent);
	create_info.arrayLayers = 1;
	create_info.mipLevels   = 1;
	subresource.mipLevel    = 1;
	subresource.arrayLayer  = 1;
}

HPPImage::HPPImage(HPPImage &&other) noexcept :
    HPPAllocated{std::move(other)},
    create_info(std::exchange(other.create_info, {})),
    subresource(std::exchange(other.subresource, {})),
    views(std::exchange(other.views, {}))
{
	// Update image views references to this image to avoid dangling pointers
	for (auto &view : views)
	{
		view->set_image(*this);
	}
}

HPPImage::~HPPImage()
{
	destroy_image(get_handle());
}

uint8_t *HPPImage::map()
{
	if (create_info.tiling != vk::ImageTiling::eLinear)
	{
		LOGW("Mapping image memory that is not linear");
	}
	return Allocated::map();
}

vk::ImageType HPPImage::get_type() const
{
	return create_info.imageType;
}

const vk::Extent3D &HPPImage::get_extent() const
{
	return create_info.extent;
}

vk::Format HPPImage::get_format() const
{
	return create_info.format;
}

vk::SampleCountFlagBits HPPImage::get_sample_count() const
{
	return create_info.samples;
}

vk::ImageUsageFlags HPPImage::get_usage() const
{
	return create_info.usage;
}

vk::ImageTiling HPPImage::get_tiling() const
{
	return create_info.tiling;
}

vk::ImageSubresource HPPImage::get_subresource() const
{
	return subresource;
}

uint32_t HPPImage::get_array_layer_count() const
{
	return create_info.arrayLayers;
}

std::unordered_set<vkb::core::HPPImageView *> &HPPImage::get_views()
{
	return views;
}

}        // namespace core
}        // namespace vkb
