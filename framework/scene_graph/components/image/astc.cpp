/* Copyright (c) 2019-2022, Arm Limited and Contributors
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

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
#if defined(_WIN32) || defined(_WIN64)
// Windows.h defines IGNORE, so we must #undef it to avoid clashes with astc header
#	undef IGNORE
#endif
#include <astcenc.h>
VKBP_ENABLE_WARNINGS()

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
	// Initializes ASTC library
	static bool                  initialized{false};
	static std::mutex            initialization;
	std::unique_lock<std::mutex> lock{initialization};
	if (!initialized)
	{
		static const unsigned int block_x = 6;
		static const unsigned int block_y = 6;
		static const unsigned int block_z = 1;
		static const astcenc_profile profile = ASTCENC_PRF_LDR;
		static const float quality = ASTCENC_PRE_MEDIUM;

		config.block_x = block_x;
		config.block_y = block_y;
		config.profile = profile;
		// Init stuff
		status = astcenc_config_init(profile, block_x, block_y, block_z, quality, 0, &config);
		if (status != ASTCENC_SUCCESS)
		{
			printf("ERROR: Codec config init failed: %s\n", astcenc_get_error_string(status));
			return;
		}
		initialized = true;
	}
}

void Astc::decode(BlockDim blockdim, VkExtent3D extent, const uint8_t *data_)
{
	// Actual decoding
	astcenc_type         bitness     = ASTCENC_TYPE_U8;
	astcenc_swizzle   swz_decode{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };
	static const unsigned int thread_count = 1;

	int xdim = blockdim.x;
	int ydim = blockdim.y;
	int zdim = blockdim.z;

	if ((xdim < 3 || xdim > 6 || ydim < 3 || ydim > 6 || zdim < 3 || zdim > 6) &&
	    (xdim < 4 || xdim == 7 || xdim == 9 || xdim == 11 || xdim > 12 ||
	     ydim < 4 || ydim == 7 || ydim == 9 || ydim == 11 || ydim > 12 || zdim != 1))
	{
		throw std::runtime_error{"Error reading astc: invalid block"};
	}

	uint32_t xsize = extent.width;
	uint32_t ysize = extent.height;
	uint32_t zsize = extent.depth;

	if (xsize == 0 || ysize == 0 || zsize == 0)
	{
		throw std::runtime_error{"Error reading astc: invalid size"};
	}

	int xblocks = static_cast<int>(xsize + xdim - 1) / xdim;
	int yblocks = static_cast<int>(ysize + ydim - 1) / ydim;
	int zblocks = static_cast<int>(zsize + zdim - 1) / zdim;

	// Space needed for 16 bytes of output per compressed block
	size_t comp_len = xblocks * yblocks * zblocks * 16;
	auto* comp_data = new uint8_t[comp_len];
	astcenc_context* context;
	status = astcenc_context_alloc(&config, thread_count, &context);
	if (status != ASTCENC_SUCCESS)
	{
		printf("ERROR: Codec context alloc failed: %s\n", astcenc_get_error_string(status));
		return;
	}

	astcenc_image astc_image{};
	astc_image.dim_x = xsize;
	astc_image.dim_y = ysize;
	astc_image.dim_z = zsize;
	astc_image.data_type = bitness;
	astc_image.data = (void **) &data_;

	status = astcenc_decompress_image(context, comp_data, comp_len, &astc_image, &swz_decode, 0);
	if (status != ASTCENC_SUCCESS)
	{
		printf("ERROR: Codec decompress failed: %s\n", astcenc_get_error_string(status));
		return;
	}

	astcenc_context_free(context);
	delete [] comp_data;
}

Astc::Astc(const Image &image) :
    Image{image.get_name()},
    config(),
    status()
{
	init();

	// Locate mip #0 in the KTX. This is the first one in the data array for KTX1s, but the last one in KTX2s!
	auto mip_it = std::find_if(image.get_mipmaps().begin(), image.get_mipmaps().end(),
	                           [](auto &mip) { return mip.level == 0; });
	assert(mip_it != image.get_mipmaps().end() && "Mip #0 not found");

	// When decoding ASTC on CPU (as it is the case in here), we don't decode all mips in the mip chain.
	// Instead, we just decode mip #0 and re-generate the other LODs later (via image->generate_mipmaps()).
	const auto     blockdim = to_blockdim(image.get_format());
	const uint8_t *data_ptr = image.get_data().data() + mip_it->offset;
	decode(blockdim, mip_it->extent, data_ptr);
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

	decode(blockdim, extent, data.data() + sizeof(AstcHeader));
}

}        // namespace sg
}        // namespace vkb
