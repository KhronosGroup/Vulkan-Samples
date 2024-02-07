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

#include "hpp_image.h"

#include "common/hpp_utils.h"
#include "filesystem/legacy.h"
#include "scene_graph/components/image/astc.h"
#include "scene_graph/components/image/ktx.h"
#include "scene_graph/components/image/stb.h"
#include <stb_image_resize.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_format_traits.hpp>

namespace vkb
{
namespace scene_graph
{
namespace components
{
bool is_astc(const vk::Format format)
{
	return strncmp(vk::compressionScheme(format), "ASTC", 4) == 0;
}

HPPImage::HPPImage(const std::string &name, std::vector<uint8_t> &&d, std::vector<vkb::scene_graph::components::HPPMipmap> &&m) :
    Component{name},
    data{std::move(d)},
    format{vk::Format::eR8G8B8A8Unorm},
    mipmaps{std::move(m)}
{}

std::unique_ptr<vkb::scene_graph::components::HPPImage> HPPImage::load(const std::string &name, const std::string &uri,
                                                                       ContentType content_type)
{
	std::unique_ptr<vkb::scene_graph::components::HPPImage> image{nullptr};

	auto data = fs::read_asset(uri);

	// Get extension
	auto extension = get_extension(uri);

	// the derived classes Stb, Astc, and Ktx are not transcoded (yet), so we need some more complex casting here...
	if (extension == "png" || extension == "jpg")
	{
		image = std::unique_ptr<vkb::scene_graph::components::HPPImage>(reinterpret_cast<vkb::scene_graph::components::HPPImage *>(
		    std::make_unique<vkb::sg::Stb>(name, data, static_cast<vkb::sg::Image::ContentType>(content_type)).release()));
	}
	else if (extension == "astc")
	{
		image = std::unique_ptr<vkb::scene_graph::components::HPPImage>(
		    reinterpret_cast<vkb::scene_graph::components::HPPImage *>(std::make_unique<vkb::sg::Astc>(name, data).release()));
	}
	else if ((extension == "ktx") || (extension == "ktx2"))
	{
		image = std::unique_ptr<vkb::scene_graph::components::HPPImage>(reinterpret_cast<vkb::scene_graph::components::HPPImage *>(
		    std::make_unique<vkb::sg::Ktx>(name, data, static_cast<vkb::sg::Image::ContentType>(content_type)).release()));
	}

	return image;
}

std::type_index HPPImage::get_type()
{
	return typeid(HPPImage);
}

void HPPImage::clear_data()
{
	data.clear();
	data.shrink_to_fit();
}

void HPPImage::coerce_format_to_srgb()
{
	// When the color-space of a loaded image is unknown (from KTX1 for example) we
	// may want to assume that the loaded data is in sRGB format (since it usually is).
	// In those cases, this helper will get called which will force an existing unorm
	// format to become an srgb format where one exists. If none exist, the format will
	// remain unmodified.
	switch (format)
	{
		case vk::Format::eR8Unorm:
			format = vk::Format::eR8Srgb;
			break;
		case vk::Format::eR8G8Unorm:
			format = vk::Format::eR8G8Srgb;
			break;
		case vk::Format::eR8G8B8Unorm:
			format = vk::Format::eR8G8B8Srgb;
			break;
		case vk::Format::eB8G8R8Unorm:
			format = vk::Format::eB8G8R8Srgb;
			break;
		case vk::Format::eR8G8B8A8Unorm:
			format = vk::Format::eR8G8B8A8Srgb;
			break;
		case vk::Format::eB8G8R8A8Unorm:
			format = vk::Format::eB8G8R8A8Srgb;
			break;
		case vk::Format::eA8B8G8R8UnormPack32:
			format = vk::Format::eA8B8G8R8SrgbPack32;
			break;
		case vk::Format::eBc1RgbUnormBlock:
			format = vk::Format::eBc1RgbSrgbBlock;
			break;
		case vk::Format::eBc1RgbaUnormBlock:
			format = vk::Format::eBc1RgbaSrgbBlock;
			break;
		case vk::Format::eBc2UnormBlock:
			format = vk::Format::eBc2SrgbBlock;
			break;
		case vk::Format::eBc3UnormBlock:
			format = vk::Format::eBc3SrgbBlock;
			break;
		case vk::Format::eBc7UnormBlock:
			format = vk::Format::eBc7SrgbBlock;
			break;
		case vk::Format::eEtc2R8G8B8UnormBlock:
			format = vk::Format::eEtc2R8G8B8SrgbBlock;
			break;
		case vk::Format::eEtc2R8G8B8A1UnormBlock:
			format = vk::Format::eEtc2R8G8B8A1SrgbBlock;
			break;
		case vk::Format::eEtc2R8G8B8A8UnormBlock:
			format = vk::Format::eEtc2R8G8B8A8SrgbBlock;
			break;
		case vk::Format::eAstc4x4UnormBlock:
			format = vk::Format::eAstc4x4SrgbBlock;
			break;
		case vk::Format::eAstc5x4UnormBlock:
			format = vk::Format::eAstc5x4SrgbBlock;
			break;
		case vk::Format::eAstc5x5UnormBlock:
			format = vk::Format::eAstc5x5SrgbBlock;
			break;
		case vk::Format::eAstc6x5UnormBlock:
			format = vk::Format::eAstc6x5SrgbBlock;
			break;
		case vk::Format::eAstc6x6UnormBlock:
			format = vk::Format::eAstc6x6SrgbBlock;
			break;
		case vk::Format::eAstc8x5UnormBlock:
			format = vk::Format::eAstc8x5SrgbBlock;
			break;
		case vk::Format::eAstc8x6UnormBlock:
			format = vk::Format::eAstc8x6SrgbBlock;
			break;
		case vk::Format::eAstc8x8UnormBlock:
			format = vk::Format::eAstc8x8SrgbBlock;
			break;
		case vk::Format::eAstc10x5UnormBlock:
			format = vk::Format::eAstc10x5SrgbBlock;
			break;
		case vk::Format::eAstc10x6UnormBlock:
			format = vk::Format::eAstc10x6SrgbBlock;
			break;
		case vk::Format::eAstc10x8UnormBlock:
			format = vk::Format::eAstc10x8SrgbBlock;
			break;
		case vk::Format::eAstc10x10UnormBlock:
			format = vk::Format::eAstc10x10SrgbBlock;
			break;
		case vk::Format::eAstc12x10UnormBlock:
			format = vk::Format::eAstc12x10SrgbBlock;
			break;
		case vk::Format::eAstc12x12UnormBlock:
			format = vk::Format::eAstc12x12SrgbBlock;
			break;
		case vk::Format::ePvrtc12BppUnormBlockIMG:
			format = vk::Format::ePvrtc12BppSrgbBlockIMG;
			break;
		case vk::Format::ePvrtc14BppUnormBlockIMG:
			format = vk::Format::ePvrtc14BppSrgbBlockIMG;
			break;
		case vk::Format::ePvrtc22BppUnormBlockIMG:
			format = vk::Format::ePvrtc22BppSrgbBlockIMG;
			break;
		case vk::Format::ePvrtc24BppUnormBlockIMG:
			format = vk::Format::ePvrtc24BppSrgbBlockIMG;
			break;
		default:
			break;
	}
}

void HPPImage::create_vk_image(vkb::core::HPPDevice &device, vk::ImageViewType image_view_type, vk::ImageCreateFlags flags)
{
	assert(!vk_image && !vk_image_view && "Vulkan HPPImage already constructed");

	vk_image = std::make_unique<vkb::core::HPPImage>(device,
	                                                 get_extent(),
	                                                 format,
	                                                 vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
	                                                 VMA_MEMORY_USAGE_GPU_ONLY,
	                                                 vk::SampleCountFlagBits::e1,
	                                                 to_u32(mipmaps.size()),
	                                                 layers,
	                                                 vk::ImageTiling::eOptimal,
	                                                 flags);
	vk_image->set_debug_name(get_name());

	vk_image_view = std::make_unique<vkb::core::HPPImageView>(*vk_image, image_view_type);
	vk_image_view->set_debug_name("View on " + get_name());
}

void HPPImage::generate_mipmaps()
{
	assert(mipmaps.size() == 1 && "Mipmaps already generated");

	if (mipmaps.size() > 1)
	{
		return;        // Do not generate again
	}

	auto extent      = get_extent();
	auto next_width  = std::max<uint32_t>(1u, extent.width / 2);
	auto next_height = std::max<uint32_t>(1u, extent.height / 2);
	auto channels    = 4;
	auto next_size   = next_width * next_height * channels;

	while (true)
	{
		// Make space for next mipmap
		auto old_size = to_u32(data.size());
		data.resize(old_size + next_size);

		auto &prev_mipmap = mipmaps.back();
		// Update mipmaps
		vkb::scene_graph::components::HPPMipmap next_mipmap{};
		next_mipmap.level  = prev_mipmap.level + 1;
		next_mipmap.offset = old_size;
		next_mipmap.extent = vk::Extent3D(next_width, next_height, 1u);

		// Fill next mipmap memory
		stbir_resize_uint8(data.data() + prev_mipmap.offset, prev_mipmap.extent.width, prev_mipmap.extent.height, 0,
		                   data.data() + next_mipmap.offset, next_mipmap.extent.width, next_mipmap.extent.height, 0, channels);

		mipmaps.emplace_back(std::move(next_mipmap));

		// Next mipmap values
		next_width  = std::max<uint32_t>(1u, next_width / 2);
		next_height = std::max<uint32_t>(1u, next_height / 2);
		next_size   = next_width * next_height * channels;

		if (next_width == 1 && next_height == 1)
		{
			break;
		}
	}
}

const std::vector<uint8_t> &HPPImage::get_data() const
{
	return data;
}

const vk::Extent3D &HPPImage::get_extent() const
{
	assert(!mipmaps.empty());
	return mipmaps[0].extent;
}

vk::Format HPPImage::get_format() const
{
	return format;
}

const uint32_t HPPImage::get_layers() const
{
	return layers;
}

const std::vector<vkb::scene_graph::components::HPPMipmap> &HPPImage::get_mipmaps() const
{
	return mipmaps;
}

const std::vector<std::vector<vk::DeviceSize>> &HPPImage::get_offsets() const
{
	return offsets;
}

const vkb::core::HPPImage &HPPImage::get_vk_image() const
{
	assert(vk_image && "Vulkan HPPImage was not created");
	return *vk_image;
}

const vkb::core::HPPImageView &HPPImage::get_vk_image_view() const
{
	assert(vk_image_view && "Vulkan HPPImage view was not created");
	return *vk_image_view;
}

vkb::scene_graph::components::HPPMipmap &HPPImage::get_mipmap(const size_t index)
{
	assert(index < mipmaps.size());
	return mipmaps[index];
}

std::vector<uint8_t> &HPPImage::get_mut_data()
{
	return data;
}

std::vector<vkb::scene_graph::components::HPPMipmap> &HPPImage::get_mut_mipmaps()
{
	return mipmaps;
}

void HPPImage::set_data(const uint8_t *raw_data, size_t size)
{
	assert(data.empty() && "HPPImage data already set");
	data = {raw_data, raw_data + size};
}

void HPPImage::set_depth(const uint32_t depth)
{
	assert(!mipmaps.empty());
	mipmaps[0].extent.depth = depth;
}

void HPPImage::set_format(const vk::Format f)
{
	format = f;
}

void HPPImage::set_height(const uint32_t height)
{
	assert(!mipmaps.empty());
	mipmaps[0].extent.height = height;
}

void HPPImage::set_layers(uint32_t l)
{
	layers = l;
}

void HPPImage::set_offsets(const std::vector<std::vector<vk::DeviceSize>> &o)
{
	offsets = o;
}

void HPPImage::set_width(const uint32_t width)
{
	assert(!mipmaps.empty());
	mipmaps[0].extent.width = width;
}

}        // namespace components
}        // namespace scene_graph
}        // namespace vkb