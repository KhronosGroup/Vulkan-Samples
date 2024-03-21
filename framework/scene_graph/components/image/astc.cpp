/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "scene_graph/components/image/astc.h"

#include <mutex>

#include "common/error.h"

#include "common/glm_common.h"
#if defined(_WIN32) || defined(_WIN64)
// Windows.h defines IGNORE, so we must #undef it to avoid clashes with astc header
#	undef IGNORE
#endif
#include <astcenc.h>

#define MAGIC_FILE_CONSTANT 0x5CA1AB13

namespace vkb
{
namespace sg
{
BlockDim to_blockdim(const VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
		case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
			return {4, 4, 1};
		case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
		case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
			return {5, 4, 1};
		case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
			return {5, 5, 1};
		case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
			return {6, 5, 1};
		case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
			return {6, 6, 1};
		case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
			return {8, 5, 1};
		case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
			return {8, 6, 1};
		case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
			return {8, 8, 1};
		case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
			return {10, 5, 1};
		case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
			return {10, 6, 1};
		case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
			return {10, 8, 1};
		case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
			return {10, 10, 1};
		case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
		case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
			return {12, 10, 1};
		case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
		case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
			return {12, 12, 1};
		default:
			throw std::runtime_error{"Invalid astc format"};
	}
}

struct AstcHeader
{
	uint8_t magic[4];
	uint8_t blockdim_x;
	uint8_t blockdim_y;
	uint8_t blockdim_z;
	uint8_t xsize[3];        // x-size = xsize[0] + xsize[1] + xsize[2]
	uint8_t ysize[3];        // x-size, y-size and z-size are given in texels;
	uint8_t zsize[3];        // block count is inferred
};

void Astc::init()
{
}

void Astc::decode(BlockDim blockdim, VkExtent3D extent, const uint8_t *compressed_data, uint32_t compressed_size)
{
	// Actual decoding
	astcenc_swizzle swizzle = {ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};
	// Configure the compressor run
	astcenc_config astc_config;
	auto           atscresult = astcenc_config_init(
        ASTCENC_PRF_LDR_SRGB,
        blockdim.x,
        blockdim.y,
        blockdim.z,
        ASTCENC_PRE_FAST,
        ASTCENC_FLG_DECOMPRESS_ONLY,
        &astc_config);

	if (atscresult != ASTCENC_SUCCESS)
	{
		throw std::runtime_error{"Error initializing astc"};
	}

	if (extent.width == 0 || extent.height == 0 || extent.depth == 0)
	{
		throw std::runtime_error{"Error reading astc: invalid size"};
	}

	// Allocate working state given config and thread_count
	astcenc_context *astc_context;
	astcenc_context_alloc(&astc_config, 1, &astc_context);

	astcenc_image decoded{};
	decoded.dim_x     = extent.width;
	decoded.dim_y     = extent.height;
	decoded.dim_z     = extent.depth;
	decoded.data_type = ASTCENC_TYPE_U8;

	// allocate storage for the decoded image
	// The astcenc_decompress_image function will write directly to the image data vector
	auto  uncompressed_size = decoded.dim_x * decoded.dim_y * decoded.dim_z * 4;
	auto &decoded_data      = get_mut_data();
	decoded_data.resize(uncompressed_size);
	void *data_ptr = static_cast<void *>(decoded_data.data());
	decoded.data   = &data_ptr;

	atscresult = astcenc_decompress_image(astc_context, compressed_data, compressed_size, &decoded, &swizzle, 0);
	if (atscresult != ASTCENC_SUCCESS)
	{
		throw std::runtime_error("Error decoding astc");
	}
	astcenc_context_free(astc_context);

	set_format(VK_FORMAT_R8G8B8A8_SRGB);
	set_width(decoded.dim_x);
	set_height(decoded.dim_y);
	set_depth(decoded.dim_z);
}

Astc::Astc(const Image &image) :
    Image{image.get_name()}
{
	init();

	// Locate mip #0 in the KTX. This is the first one in the data array for KTX1s, but the last one in KTX2s!
	auto mip_it = std::find_if(image.get_mipmaps().begin(), image.get_mipmaps().end(),
	                           [](auto &mip) { return mip.level == 0; });
	assert(mip_it != image.get_mipmaps().end() && "Mip #0 not found");

	// When decoding ASTC on CPU (as it is the case in here), we don't decode all mips in the mip chain.
	// Instead, we just decode mip #0 and re-generate the other LODs later (via image->generate_mipmaps()).
	const auto     blockdim = to_blockdim(image.get_format());
	const auto    &extent   = mip_it->extent;
	auto           size     = extent.width * extent.height * extent.depth * 4;
	const uint8_t *data_ptr = image.get_data().data() + mip_it->offset;
	decode(blockdim, mip_it->extent, data_ptr, size);
}

Astc::Astc(const std::string &name, const std::vector<uint8_t> &data) :
    Image{name}
{
	init();

	// Read header
	if (data.size() < sizeof(AstcHeader))
	{
		throw std::runtime_error{"Error reading astc: invalid memory"};
	}
	AstcHeader header{};
	std::memcpy(&header, data.data(), sizeof(AstcHeader));
	uint32_t magicval = header.magic[0] + 256 * static_cast<uint32_t>(header.magic[1]) + 65536 * static_cast<uint32_t>(header.magic[2]) + 16777216 * static_cast<uint32_t>(header.magic[3]);
	if (magicval != MAGIC_FILE_CONSTANT)
	{
		throw std::runtime_error{"Error reading astc: invalid magic"};
	}

	BlockDim blockdim = {
	    /* xdim = */ header.blockdim_x,
	    /* ydim = */ header.blockdim_y,
	    /* zdim = */ header.blockdim_z};

	VkExtent3D extent = {
	    /* width  = */ static_cast<uint32_t>(header.xsize[0] + 256 * header.xsize[1] + 65536 * header.xsize[2]),
	    /* height = */ static_cast<uint32_t>(header.ysize[0] + 256 * header.ysize[1] + 65536 * header.ysize[2]),
	    /* depth  = */ static_cast<uint32_t>(header.zsize[0] + 256 * header.zsize[1] + 65536 * header.zsize[2])};

	decode(blockdim, extent, data.data() + sizeof(AstcHeader), to_u32(data.size() - sizeof(AstcHeader)));
}

}        // namespace sg
}        // namespace vkb
