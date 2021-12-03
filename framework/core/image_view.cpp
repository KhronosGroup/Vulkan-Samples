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

#include "image_view.h"

#include "core/image.h"
#include "device.h"

namespace vkb
{
namespace core
{
ImageView::ImageView(Image &img, VkImageViewType view_type, VkFormat format,
                     uint32_t mip_level, uint32_t array_layer,
                     uint32_t n_mip_levels, uint32_t n_array_layers) :
    VulkanResource{VK_NULL_HANDLE, &img.get_device()},
    image{&img},
    format{format}
{
	if (format == VK_FORMAT_UNDEFINED)
	{
		this->format = format = image->get_format();
	}

	subresource_range.baseMipLevel   = mip_level;
	subresource_range.baseArrayLayer = array_layer;
	subresource_range.levelCount     = n_mip_levels == 0 ? image->get_subresource().mipLevel : n_mip_levels;
	subresource_range.layerCount     = n_array_layers == 0 ? image->get_subresource().arrayLayer : n_array_layers;

	if (is_depth_stencil_format(format))
	{
		subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else
	{
		subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	view_info.image            = image->get_handle();
	view_info.viewType         = view_type;
	view_info.format           = format;
	view_info.subresourceRange = subresource_range;

	auto result = vkCreateImageView(device->get_handle(), &view_info, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create ImageView"};
	}

	// Register this image view to its image
	// in order to be notified when it gets moved
	image->get_views().emplace(this);
}

ImageView::ImageView(ImageView &&other) :
    VulkanResource{std::move(other)},
    image{other.image},
    format{other.format},
    subresource_range{other.subresource_range}
{
	// Remove old view from image set and add this new one
	auto &views = image->get_views();
	views.erase(&other);
	views.emplace(this);

	other.handle = VK_NULL_HANDLE;
}

ImageView::~ImageView()
{
	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyImageView(device->get_handle(), handle, nullptr);
	}
}

const Image &ImageView::get_image() const
{
	assert(image && "Image view is referring an invalid image");
	return *image;
}

void ImageView::set_image(Image &img)
{
	image = &img;
}

VkFormat ImageView::get_format() const
{
	return format;
}

VkImageSubresourceRange ImageView::get_subresource_range() const
{
	return subresource_range;
}

VkImageSubresourceLayers ImageView::get_subresource_layers() const
{
	VkImageSubresourceLayers subresource{};
	subresource.aspectMask     = subresource_range.aspectMask;
	subresource.baseArrayLayer = subresource_range.baseArrayLayer;
	subresource.layerCount     = subresource_range.layerCount;
	subresource.mipLevel       = subresource_range.baseMipLevel;
	return subresource;
}
}        // namespace core
}        // namespace vkb
