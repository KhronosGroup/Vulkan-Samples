/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/hpp_image_view.h"

#include "common/hpp_vk_common.h"
#include "core/hpp_device.h"
#include <vulkan/vulkan_format_traits.hpp>

namespace vkb
{
namespace core
{
HPPImageView::HPPImageView(vkb::core::HPPImage &img,
                           vk::ImageViewType    view_type,
                           vk::Format           format,
                           uint32_t             mip_level,
                           uint32_t             array_layer,
                           uint32_t             n_mip_levels,
                           uint32_t             n_array_layers) :
    HPPVulkanResource{nullptr, &img.get_device()}, image{&img}, format{format}
{
	if (format == vk::Format::eUndefined)
	{
		this->format = format = image->get_format();
	}

	subresource_range =
	    vk::ImageSubresourceRange((std::string(vk::componentName(format, 0)) == "D") ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor,
	                              mip_level,
	                              n_mip_levels == 0 ? image->get_subresource().mipLevel : n_mip_levels,
	                              array_layer,
	                              n_array_layers == 0 ? image->get_subresource().arrayLayer : n_array_layers);

	vk::ImageViewCreateInfo image_view_create_info({}, image->get_handle(), view_type, format, {}, subresource_range);

	set_handle(get_device().get_handle().createImageView(image_view_create_info));

	// Register this image view to its image
	// in order to be notified when it gets moved
	image->get_views().emplace(this);
}

HPPImageView::HPPImageView(HPPImageView &&other) :
    HPPVulkanResource{std::move(other)}, image{other.image}, format{other.format}, subresource_range{other.subresource_range}
{
	// Remove old view from image set and add this new one
	auto &views = image->get_views();
	views.erase(&other);
	views.emplace(this);

	other.set_handle(nullptr);
}

HPPImageView::~HPPImageView()
{
	if (get_handle())
	{
		get_device().get_handle().destroyImageView(get_handle());
	}
}

vk::Format HPPImageView::get_format() const
{
	return format;
}

const vkb::core::HPPImage &HPPImageView::get_image() const
{
	assert(image && "vkb::core::HPPImage view is referring an invalid image");
	return *image;
}

void HPPImageView::set_image(vkb::core::HPPImage &img)
{
	image = &img;
}

vk::ImageSubresourceLayers HPPImageView::get_subresource_layers() const
{
	return vk::ImageSubresourceLayers(
	    subresource_range.aspectMask, subresource_range.baseMipLevel, subresource_range.baseArrayLayer, subresource_range.layerCount);
}

vk::ImageSubresourceRange HPPImageView::get_subresource_range() const
{
	return subresource_range;
}

}        // namespace core
}        // namespace vkb
