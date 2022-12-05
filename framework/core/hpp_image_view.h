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

#include <core/image_view.h>

namespace vkb
{
namespace core
{
class HPPImage;

/**
 * @brief facade class around vkb::core::ImageView, providing a vulkan.hpp-based interface
 *
 * See vkb::core::ImageView for documentation
 */
class HPPImageView : private vkb::core::ImageView
{
  public:
	HPPImageView(HPPImage &image, vk::ImageViewType view_type, vk::Format format = vk::Format::eUndefined,
	             uint32_t base_mip_level = 0, uint32_t base_array_layer = 0,
	             uint32_t n_mip_levels = 0, uint32_t n_array_layers = 0) :
	    vkb::core::ImageView(reinterpret_cast<vkb::core::Image &>(image), static_cast<VkImageViewType>(view_type), static_cast<VkFormat>(format), base_mip_level, base_array_layer, n_mip_levels, n_array_layers)
	{}

	vk::Format get_format() const
	{
		return static_cast<vk::Format>(vkb::core::ImageView::get_format());
	}

	vk::ImageView get_handle() const
	{
		return static_cast<vk::ImageView>(vkb::core::ImageView::get_handle());
	}

	const HPPImage &get_image() const
	{
		return reinterpret_cast<HPPImage const &>(ImageView::get_image());
	}

	vk::ImageSubresourceRange get_subresource_range() const
	{
		return static_cast<vk::ImageSubresourceRange>(vkb::core::ImageView::get_subresource_range());
	}

	void set_image(HPPImage &image)
	{
		vkb::core::ImageView::set_image(reinterpret_cast<vkb::core::Image &>(image));
	}
};
}        // namespace core
}        // namespace vkb
