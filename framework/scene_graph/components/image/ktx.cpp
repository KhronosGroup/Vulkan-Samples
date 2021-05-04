/* Copyright (c) 2019, Arm Limited and Contributors
 * Copyright (c) 2019, Sascha Willems
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

#include "scene_graph/components/image/ktx.h"

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include <ktx.h>
#include <ktxvulkan.h>
VKBP_ENABLE_WARNINGS()

namespace vkb
{
namespace sg
{
/// Row padding is different between KTX (pad to 4) and Vulkan (none).
/// Also region->bufferOffset, i.e. the start of each image, has
/// to be a multiple of 4 and also a multiple of the element size.
static ktx_error_code_e __stdcall optimal_tiling_callback(int          mip_level,
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

	// Set offset for the next mip level
	auto next_mip_level = static_cast<size_t>(mip_level + 1);
	if (next_mip_level < mipmaps.size())
	{
		mipmaps.at(next_mip_level).offset = mipmap.offset + static_cast<uint32_t>(face_lod_size);
	}

	return KTX_SUCCESS;
}

Ktx::Ktx(const std::string &name, const std::vector<uint8_t> &data) :
    Image{name}
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
		throw std::runtime_error{"Error loading KTX texture: " + name};
	}

	if (texture->pData)
	{
		// Already loaded
		set_data(texture->pData, texture->dataSize);
	}
	else
	{
		// Load
		auto &mut_data = get_mut_data();
		auto  size     = texture->dataSize;
		mut_data.resize(size);
		auto load_data_result = ktxTexture_LoadImageData(texture, mut_data.data(), size);
		if (load_data_result != KTX_SUCCESS)
		{
			throw std::runtime_error{"Error loading KTX image data: " + name};
		}
	}

	// Update width and height
	set_width(texture->baseWidth);
	set_height(texture->baseHeight);
	set_depth(texture->baseDepth);
	set_layers(texture->numLayers);

	bool cubemap = false;

	// Use the faces if there are 6 (for cubemap)
	if (texture->numLayers == 1 && texture->numFaces == 6)
	{
		cubemap = true;
		set_layers(texture->numFaces);
	}

	// Update format
	auto updated_format = ktxTexture_GetVkFormat(texture);
	set_format(updated_format);

	// Update mip levels
	auto &mipmap_levels = get_mut_mipmaps();
	mipmap_levels.resize(texture->numLevels);
	auto result = ktxTexture_IterateLevels(texture, optimal_tiling_callback, &mipmap_levels);
	if (result != KTX_SUCCESS)
	{
		throw std::runtime_error("Error loading KTX texture");
	}

	// If the texture contains more than one layer, then populate the offsets otherwise take the mipmap level offsets
	if (texture->numLayers > 1 || cubemap)
	{
		uint32_t layer_count = cubemap ? texture->numFaces : texture->numLayers;

		std::vector<std::vector<VkDeviceSize>> offsets;
		for (uint32_t layer = 0; layer < layer_count; layer++)
		{
			std::vector<VkDeviceSize> layer_offsets{};
			for (uint32_t level = 0; level < texture->numLevels; level++)
			{
				ktx_size_t     offset;
				KTX_error_code result;
				if (cubemap)
				{
					result = ktxTexture_GetImageOffset(texture, level, 0, layer, &offset);
				}
				else
				{
					result = ktxTexture_GetImageOffset(texture, level, layer, 0, &offset);
				}
				layer_offsets.push_back(static_cast<VkDeviceSize>(offset));
			}
			offsets.push_back(layer_offsets);
		}
		set_offsets(offsets);
	}
	else
	{
		std::vector<std::vector<VkDeviceSize>> offsets{};
		offsets.resize(1);
		for (size_t level = 0; level < mipmap_levels.size(); level++)
		{
			offsets[0].push_back(static_cast<VkDeviceSize>(mipmap_levels[level].offset));
		}
		set_offsets(offsets);
	}

	ktxTexture_Destroy(texture);
}

}        // namespace sg
}        // namespace vkb
