/* Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/hpp_device.h"
#include "scene_graph/component.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace sg
{
class Image;
}

namespace scene_graph
{
namespace components
{
/**
 * @param format Vulkan format
 * @return Whether the vulkan format is ASTC
 */
bool is_astc(vk::Format format);

/**
 * @brief Mipmap information
 */
struct HPPMipmap
{
	uint32_t     level  = 0;                /// Mipmap level
	uint32_t     offset = 0;                /// Byte offset used for uploading
	vk::Extent3D extent = {0, 0, 0};        /// Width depth and height of the mipmap
};

class HPPImage : public vkb::sg::Component
{
  public:
	using ComponentType = vkb::sg::Image;

	HPPImage(const std::string &name, std::vector<uint8_t> &&data = {}, std::vector<vkb::scene_graph::components::HPPMipmap> &&mipmaps = {{}});
	virtual ~HPPImage() = default;

  public:
	/**
	 * @brief Type of content held in image.
	 * This helps to steer the image loaders when deciding what the format should be.
	 * Some image containers don't know whether the data they contain is sRGB or not.
	 * Since most applications save color images in sRGB, knowing that an image
	 * contains color data helps us to better guess its format when unknown.
	 */
	enum ContentType
	{
		Unknown,
		Color,
		Other
	};

	static std::unique_ptr<vkb::scene_graph::components::HPPImage> load(const std::string &name, const std::string &uri, ContentType content_type);

	// from Component
	virtual std::type_index get_type() override;

	void                                                        clear_data();
	void                                                        coerce_format_to_srgb();
	void                                                        create_vk_image(vkb::core::HPPDevice &device, vk::ImageViewType image_view_type = vk::ImageViewType::e2D, vk::ImageCreateFlags flags = {});
	void                                                        generate_mipmaps();
	const std::vector<uint8_t>                                 &get_data() const;
	const vk::Extent3D                                         &get_extent() const;
	vk::Format                                                  get_format() const;
	const uint32_t                                              get_layers() const;
	const std::vector<vkb::scene_graph::components::HPPMipmap> &get_mipmaps() const;
	const std::vector<std::vector<vk::DeviceSize>>             &get_offsets() const;
	const vkb::core::HPPImage                                  &get_vk_image() const;
	const vkb::core::HPPImageView                              &get_vk_image_view() const;

  protected:
	vkb::scene_graph::components::HPPMipmap              &get_mipmap(size_t index);
	std::vector<uint8_t>                                 &get_mut_data();
	std::vector<vkb::scene_graph::components::HPPMipmap> &get_mut_mipmaps();
	void                                                  set_data(const uint8_t *raw_data, size_t size);
	void                                                  set_depth(uint32_t depth);
	void                                                  set_format(vk::Format format);
	void                                                  set_height(uint32_t height);
	void                                                  set_layers(uint32_t layers);
	void                                                  set_offsets(const std::vector<std::vector<vk::DeviceSize>> &offsets);
	void                                                  set_width(uint32_t width);

  private:
	std::vector<uint8_t>                                 data;
	vk::Format                                           format = vk::Format::eUndefined;
	uint32_t                                             layers = 1;
	std::vector<vkb::scene_graph::components::HPPMipmap> mipmaps{{}};
	std::vector<std::vector<vk::DeviceSize>>             offsets;        // Offsets stored like offsets[array_layer][mipmap_layer]
	std::unique_ptr<vkb::core::HPPImage>                 vk_image;
	std::unique_ptr<vkb::core::HPPImageView>             vk_image_view;
};

}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
