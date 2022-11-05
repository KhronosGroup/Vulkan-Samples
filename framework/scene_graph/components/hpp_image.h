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

#include <scene_graph/components/image.h>

#include <core/hpp_device.h>
#include <core/hpp_image.h>

namespace vkb
{
namespace scene_graph
{
namespace components
{
/**
 * @brief facade class around vkb::sg::Image, providing a vulkan.hpp-based interface
 *
 * See vkb::sb::Image for documentation
 */
class HPPImage : private vkb::sg::Image
{
  public:
	using vkb::sg::Image::get_data;
	using vkb::sg::Image::get_layers;
	using vkb::sg::Image::get_mipmaps;
	using vkb::sg::Image::get_offsets;

	static std::unique_ptr<HPPImage> load(std::string const &name, std::string const &uri, vkb::sg::Image::ContentType content_type)
	{
		return std::unique_ptr<HPPImage>(reinterpret_cast<HPPImage *>(vkb::sg::Image::load(name, uri, content_type).release()));
	}

	void create_vk_image(vkb::core::HPPDevice const &device, vk::ImageViewType image_view_type = vk::ImageViewType::e2D, vk::ImageCreateFlags flags = {})
	{
		vkb::sg::Image::create_vk_image(
		    reinterpret_cast<vkb::Device const &>(device), static_cast<VkImageViewType>(image_view_type), static_cast<VkImageCreateFlags>(flags));
	}

	vk::Extent3D const &get_extent() const
	{
		return reinterpret_cast<vk::Extent3D const &>(vkb::sg::Image::get_extent());
	}

	vkb::core::HPPImage const &get_vk_image() const
	{
		return reinterpret_cast<vkb::core::HPPImage const &>(vkb::sg::Image::get_vk_image());
	}

	vkb::core::HPPImageView const &get_vk_image_view() const
	{
		return reinterpret_cast<vkb::core::HPPImageView const &>(vkb::sg::Image::get_vk_image_view());
	}
};

}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
