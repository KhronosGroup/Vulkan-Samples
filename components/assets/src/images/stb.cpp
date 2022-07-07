/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "components/images/stb.hpp"

#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include <components/vfs/helpers.hpp>

namespace components
{
namespace images
{
namespace detail
{
struct ComponentsAndStride
{
	uint32_t components;
	uint32_t byte_stride;
};

inline ComponentsAndStride get_components_and_stride(const Image &image)
{
	switch (image.format)
	{
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8_USCALED:
		case VK_FORMAT_R8_SSCALED:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_SRGB:
			return {1, 8};
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8_USCALED:
		case VK_FORMAT_R8G8_SSCALED:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8_SRGB:
			return {2, 16};
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8_SNORM:
		case VK_FORMAT_R8G8B8_USCALED:
		case VK_FORMAT_R8G8B8_SSCALED:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_SNORM:
		case VK_FORMAT_B8G8R8_USCALED:
		case VK_FORMAT_B8G8R8_SSCALED:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SINT:
		case VK_FORMAT_B8G8R8_SRGB:
			return {3, 3 * 8};
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_USCALED:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_UINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
			return {4, 4 * 8};
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R16_USCALED:
		case VK_FORMAT_R16_SSCALED:
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R16_SFLOAT:
			return {1, 16};
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16_USCALED:
		case VK_FORMAT_R16G16_SSCALED:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16_SFLOAT:
			return {2, 2 * 16};
		case VK_FORMAT_R16G16B16_UNORM:
		case VK_FORMAT_R16G16B16_SNORM:
		case VK_FORMAT_R16G16B16_USCALED:
		case VK_FORMAT_R16G16B16_SSCALED:
		case VK_FORMAT_R16G16B16_UINT:
		case VK_FORMAT_R16G16B16_SINT:
		case VK_FORMAT_R16G16B16_SFLOAT:
			return {3, 3 * 16};
		case VK_FORMAT_R16G16B16A16_UNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
		case VK_FORMAT_R16G16B16A16_USCALED:
		case VK_FORMAT_R16G16B16A16_SSCALED:
		case VK_FORMAT_R16G16B16A16_UINT:
		case VK_FORMAT_R16G16B16A16_SINT:
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return {4, 4 * 16};
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_R32_SFLOAT:
			return {1, 32};
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32_SFLOAT:
			return {2, 2 * 32};
		case VK_FORMAT_R32G32B32_UINT:
		case VK_FORMAT_R32G32B32_SINT:
		case VK_FORMAT_R32G32B32_SFLOAT:
			return {3, 3 * 32};
		case VK_FORMAT_R32G32B32A32_UINT:
		case VK_FORMAT_R32G32B32A32_SINT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return {4, 4 * 32};
		case VK_FORMAT_R64_UINT:
		case VK_FORMAT_R64_SINT:
		case VK_FORMAT_R64_SFLOAT:
			return {1, 64};
		case VK_FORMAT_R64G64_UINT:
		case VK_FORMAT_R64G64_SINT:
		case VK_FORMAT_R64G64_SFLOAT:
			return {2, 2 * 64};
		case VK_FORMAT_R64G64B64_UINT:
		case VK_FORMAT_R64G64B64_SINT:
		case VK_FORMAT_R64G64B64_SFLOAT:
			return {3, 3 * 64};
		case VK_FORMAT_R64G64B64A64_UINT:
		case VK_FORMAT_R64G64B64A64_SINT:
		case VK_FORMAT_R64G64B64A64_SFLOAT:
			return {4, 4 * 64};
		default:
			return {0, 0};
	}
}        // namespace detail
}        // namespace detail

StackErrorPtr StbLoader::load_from_file(const std::string &name, vfs::FileSystem &fs, const std::string &path, ImagePtr *o_image) const
{
	if (!fs.file_exists(path))
	{
		return StackError::unique("file does not exist " + path, "images/stb.cpp", __LINE__);
	}

	std::shared_ptr<vfs::Blob> blob;

	if (auto err = fs.read_file(path, &blob))
	{
		err->push("failed to read file " + path, "images/stb.cpp", __LINE__);
		return std::move(err);
	}

	return load_from_memory(name, blob->binary(), o_image);
}

StackErrorPtr StbLoader::load_from_memory(const std::string &name, const std::vector<uint8_t> &data, ImagePtr *o_image) const
{
	int width;
	int height;
	int comp;
	int req_comp = 4;

	auto data_buffer = reinterpret_cast<const stbi_uc *>(data.data());
	auto data_size   = static_cast<int>(data.size());

	auto *raw_data = stbi_load_from_memory(data_buffer, data_size, &width, &height, &comp, req_comp);

	if (!raw_data)
	{
		return StackError::unique("failed to read image data: " + name, "images/stb.cpp", __LINE__);
	}

	auto &image = *o_image = std::shared_ptr<images::Image>();

	image->data = std::vector<uint8_t>{raw_data, raw_data + width * height * req_comp};
	stbi_image_free(raw_data);

	image->format = VK_FORMAT_R8G8B8A8_UNORM;

	image->mips.push_back(Mipmap{
	    0,
	    0,
	    static_cast<uint32_t>(image->data.size()),
	    {
	        static_cast<uint32_t>(width),
	        static_cast<uint32_t>(height),
	        1u,
	    },
	});

	return nullptr;
}

struct WriteContext
{
	std::string      path;
	vfs::FileSystem &fs;
};

void write_func(void *context, void *data, size_t size)
{
	auto &write_context = *static_cast<WriteContext *>(context);
	write_context.fs.write_file(write_context.path, data, size);
};

StackErrorPtr StbWriter::write_to_file(vfs::FileSystem &fs, const std::string &path, const Image &image) const
{
	if (!image.valid())
	{
		return StackError::unique("image not valid", "images/stb.cpp", __LINE__);
	}

	const auto &mip = image.mips[0];

	WriteContext context{
	    path,
	    fs,
	};

	int width  = static_cast<int>(mip.extent.width);
	int height = static_cast<int>(mip.extent.height);

	auto stats = detail::get_components_and_stride(image);

	int result;

	switch (m_target)
	{
		case Target::PNG:
			result = stbi_write_png_to_func(reinterpret_cast<stbi_write_func *>(write_func), static_cast<void *>(&context), width, height, stats.components, image.data.data() + mip.offset, width * stats.components);
			break;
		case Target::BMP:
			result = stbi_write_bmp_to_func(reinterpret_cast<stbi_write_func *>(write_func), static_cast<void *>(&context), width, height, stats.components, image.data.data() + mip.offset);
			break;
		case Target::TGA:
			result = stbi_write_tga_to_func(reinterpret_cast<stbi_write_func *>(write_func), static_cast<void *>(&context), width, height, stats.components, image.data.data() + mip.offset);
			break;
		case Target::JPG:
			result = stbi_write_jpg_to_func(reinterpret_cast<stbi_write_func *>(write_func), static_cast<void *>(&context), width, height, stats.components, image.data.data() + mip.offset, 100);
			break;
		default:
			return StackError::unique("image target not supported", "images/stb.cpp", __LINE__);
	}

	if (!result)
	{
		const char *reason = stbi_failure_reason();
		return StackError::unique(reason, "images/stb.cpp", __LINE__);
	}

	return nullptr;
}

}        // namespace images
}        // namespace components
