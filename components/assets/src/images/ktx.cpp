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

#include <algorithm>
#include <cassert>

#include <ktx.h>
#include <ktxvulkan.h>

namespace components
{
namespace images
{
namespace
{
bool sort_mips(assets::Mipmap first, assets::Mipmap second)
{
	return first.byte_length > second.byte_length;
};
}        // namespace

struct CallbackData final
{
	ktxTexture                  *texture;
	std::vector<assets::Mipmap> *mipmaps;
};

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
    void * /* pixels */,
    void *user_data)
{
	auto *callback_data = reinterpret_cast<CallbackData *>(user_data);

	// Get mipmaps
	assert(static_cast<size_t>(mip_level) < callback_data->mipmaps->size() && "Not enough space in the mipmap vector");

	ktx_size_t mipmap_offset = 0;
	auto       result        = ktxTexture_GetImageOffset(callback_data->texture, mip_level, 0, face, &mipmap_offset);
	if (result != KTX_SUCCESS)
	{
		return result;
	}

	auto &mipmap         = callback_data->mipmaps->at(mip_level);
	mipmap.level         = mip_level;
	mipmap.offset        = static_cast<uint32_t>(mipmap_offset);
	mipmap.extent.width  = width;
	mipmap.extent.height = height;
	mipmap.extent.depth  = depth;
	mipmap.byte_length   = static_cast<uint32_t>(face_lod_size);

	return KTX_SUCCESS;
}

void KtxLoader::load_from_memory(const std::string &name, const std::vector<uint8_t> &data, assets::ImageAssetPtr *o_image) const
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
		ERRORF("Error loading KTX texture: {}", name);
	}

	auto &image = *o_image = std::make_shared<assets::ImageAsset>();

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
			ERRORF("Error loading KTX texture: {}", name);
		}
	}

	CallbackData callback_data{};
	callback_data.texture = texture;
	callback_data.mipmaps = &image->mips;

	image->mips.resize(texture->numLevels);
	auto result = ktxTexture_IterateLevels(texture, optimal_tiling_callback, &callback_data);
	if (result != KTX_SUCCESS)
	{
		ERRORF("Error loading KTX texture: {}", name);
	}

	if (!image->mips.empty())
	{
		image->mips[0].extent = {texture->baseWidth, texture->baseHeight, texture->baseDepth};
	}

	image->name   = name;
	image->layers = texture->numLayers == 1 && texture->numFaces == 6 ? 6 : texture->numLayers;
	image->format = ktxTexture_GetVkFormat(texture);

	std::sort(image->mips.begin(), image->mips.end(), sort_mips);

	ktxTexture_Destroy(texture);
}
}        // namespace images
}        // namespace components