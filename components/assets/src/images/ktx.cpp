/* Copyright (c) 2019-2021, Arm Limited and Contributors
 * Copyright (c) 2019-2021, Sascha Willems
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

#include "components/images/ktx.hpp"

#include <cassert>

#include <ktx.h>
#include <ktxvulkan.h>

namespace components
{
namespace images
{
/// Row padding is different between KTX (pad to 4) and Vulkan (none).
/// Also region->bufferOffset, i.e. the start of each image, has
/// to be a multiple of 4 and also a multiple of the element size.
static ktx_error_code_e KTX_APIENTRY optimal_tiling_callback(
    int          mip_level,
    int          face,
    int          width,
    int          height,
    int          depth,
    ktx_uint64_t face_lod_size,
    void *       pixels,
    void *       user_data)
{
	// Get mipmaps
	auto &mipmaps = *reinterpret_cast<std::vector<Mipmap> *>(user_data);
	assert(static_cast<size_t>(mip_level) < mipmaps.size() && "Not enough space in the mipmap vector");

	auto &mipmap         = mipmaps.at(mip_level);
	mipmap.level         = mip_level;
	mipmap.extent.width  = width;
	mipmap.extent.height = height;
	mipmap.extent.depth  = depth;
	mipmap.byte_length   = static_cast<uint32_t>(face_lod_size);

	// Set offset for the next mip level
	auto next_mip_level = static_cast<size_t>(mip_level + 1);
	if (next_mip_level < mipmaps.size())
	{
		mipmaps.at(next_mip_level).offset = mipmap.offset + static_cast<uint32_t>(face_lod_size);
	}

	return KTX_SUCCESS;
}

StackErrorPtr KtxLoader::load_from_file(const std::string &name, vfs::FileSystem &fs, const std::string &path, ImagePtr *o_image) const
{
	if (!fs.file_exists(path))
	{
		return StackError::unique("file does not exist " + path);
	}

	std::shared_ptr<vfs::Blob> blob;

	if (fs.read_file(path, &blob) != vfs::status::Success)
	{
		return StackError::unique("failed to read file " + path);
	}

	return load_from_memory(name, blob->binary(), o_image);
}

StackErrorPtr KtxLoader::load_from_memory(const std::string &name, const std::vector<uint8_t> &data, ImagePtr *o_image) const
{
	auto data_buffer = reinterpret_cast<const ktx_uint8_t *>(data.data());
	auto data_size   = static_cast<ktx_size_t>(data.size());

	ktxTexture *texture;
	auto        load_ktx_result = ktxTexture_CreateFromMemory(data_buffer,
                                                       data_size,
                                                       KTX_TEXTURE_CREATE_NO_FLAGS,
                                                       &texture);
	if (load_ktx_result != KTX_SUCCESS)
	{
		return StackError::unique("Error loading KTX texture: " + name, "images/ktx.cpp", __LINE__);
	}

	auto &image = *o_image = std::shared_ptr<images::Image>();

	if (texture->pData)
	{
		image->data = std::vector<uint8_t>(texture->pData, texture->pData + texture->dataSize);
	}
	else
	{
		// data not loaded, load data
		image->data.resize(texture->dataSize);
		auto load_data_result = ktxTexture_LoadImageData(texture, image->data.data(), texture->dataSize);
		if (load_data_result != KTX_SUCCESS)
		{
			return StackError::unique("Error loading KTX texture: " + name, "images/ktx.cpp", __LINE__);
		}
	}

	image->mips.resize(texture->numLevels);
	auto result = ktxTexture_IterateLevels(texture, optimal_tiling_callback, &image->mips);
	if (result != KTX_SUCCESS)
	{
		return StackError::unique("Error loading KTX texture: " + name, "images/ktx.cpp", __LINE__);
	}

	if (!image->mips.empty())
	{
		image->mips[0].extent = {texture->baseWidth, texture->baseHeight, texture->baseDepth};
	}

	image->layers = texture->numLayers == 1 && texture->numFaces == 6 ? 6 : texture->numLayers;
	image->format = ktxTexture_GetVkFormat(texture);

	ktxTexture_Destroy(texture);

	return nullptr;
}
}        // namespace images
}        // namespace components