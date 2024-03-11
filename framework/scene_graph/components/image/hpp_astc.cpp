/* Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "scene_graph/components/image/hpp_astc.h"

#include <vulkan/vulkan_format_traits.hpp>

VKBP_DISABLE_WARNINGS()
// #include "common/glm_common.h"
#if defined(_WIN32) || defined(_WIN64)
// Windows.h defines IGNORE, so we must #undef it to avoid clashes with astc header
#	undef IGNORE
#endif
#include <astc_codec_internals.h>
VKBP_ENABLE_WARNINGS()

#define MAGIC_FILE_CONSTANT 0x5CA1AB13

namespace vkb
{
namespace scene_graph
{
namespace components
{
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

void HPPAstc::init()
{
	// Initializes ASTC library
	static bool                  initialized{false};
	static std::mutex            initialization;
	std::unique_lock<std::mutex> lock{initialization};
	if (!initialized)
	{
		// Init stuff
		prepare_angular_tables();
		build_quantization_mode_table();
		initialized = true;
	}
}

void HPPAstc::decode(std::array<uint8_t, 3> const &blockdim, vk::Extent3D const &extent, const uint8_t *data_)
{
	// Actual decoding
	astc_decode_mode decode_mode = DECODE_LDR_SRGB;
	uint32_t         bitness     = 8;
	swizzlepattern   swz_decode  = {0, 1, 2, 3};

	int xdim = blockdim[0];
	int ydim = blockdim[1];
	int zdim = blockdim[2];

	if ((xdim < 3 || xdim > 6 || ydim < 3 || ydim > 6 || zdim < 3 || zdim > 6) &&
	    (xdim < 4 || xdim == 7 || xdim == 9 || xdim == 11 || xdim > 12 ||
	     ydim < 4 || ydim == 7 || ydim == 9 || ydim == 11 || ydim > 12 || zdim != 1))
	{
		throw std::runtime_error{"Error reading astc: invalid block"};
	}

	int xsize = extent.width;
	int ysize = extent.height;
	int zsize = extent.depth;

	if (xsize == 0 || ysize == 0 || zsize == 0)
	{
		throw std::runtime_error{"Error reading astc: invalid size"};
	}

	int xblocks = (xsize + xdim - 1) / xdim;
	int yblocks = (ysize + ydim - 1) / ydim;
	int zblocks = (zsize + zdim - 1) / zdim;

	auto astc_image = allocate_image(bitness, xsize, ysize, zsize, 0);
	initialize_image(astc_image);

	imageblock pb;
	for (int z = 0; z < zblocks; z++)
	{
		for (int y = 0; y < yblocks; y++)
		{
			for (int x = 0; x < xblocks; x++)
			{
				int            offset = (((z * yblocks + y) * xblocks) + x) * 16;
				const uint8_t *bp     = data_ + offset;

				physical_compressed_block pcb = *reinterpret_cast<const physical_compressed_block *>(bp);
				symbolic_compressed_block scb;

				physical_to_symbolic(xdim, ydim, zdim, pcb, &scb);
				decompress_symbolic_block(decode_mode, xdim, ydim, zdim, x * xdim, y * ydim, z * zdim, &scb, &pb);
				write_imageblock(astc_image, &pb, xdim, ydim, zdim, x * xdim, y * ydim, z * zdim, swz_decode);
			}
		}
	}

	set_data(astc_image->imagedata8[0][0], astc_image->xsize * astc_image->ysize * astc_image->zsize * 4);
	set_format(vk::Format::eR8G8B8A8Srgb);
	set_width(static_cast<uint32_t>(astc_image->xsize));
	set_height(static_cast<uint32_t>(astc_image->ysize));
	set_depth(static_cast<uint32_t>(astc_image->zsize));

	destroy_image(astc_image);
}

HPPAstc::HPPAstc(const vkb::scene_graph::components::HPPImage &image) :
    vkb::scene_graph::components::HPPImage{image.get_name()}
{
	init();

	// Locate mip #0 in the KTX. This is the first one in the data array for KTX1s, but the last one in KTX2s!
	auto mip_it = std::find_if(image.get_mipmaps().begin(), image.get_mipmaps().end(),
	                           [](auto &mip) { return mip.level == 0; });
	assert(mip_it != image.get_mipmaps().end() && "Mip #0 not found");

	// When decoding ASTC on CPU (as it is the case in here), we don't decode all mips in the mip chain.
	// Instead, we just decode mip #0 and re-generate the other LODs later (via image->generate_mipmaps()).
	const auto     blockdim = vk::blockExtent(image.get_format());
	const uint8_t *data_ptr = image.get_data().data() + mip_it->offset;
	decode(blockdim, mip_it->extent, data_ptr);
}

HPPAstc::HPPAstc(const std::string &name, const std::vector<uint8_t> &data) :
    vkb::scene_graph::components::HPPImage{name}
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

	std::array<uint8_t, 3> blockdim = {header.blockdim_x, header.blockdim_y, header.blockdim_z};

	VkExtent3D extent = {
	    /* width  = */ static_cast<uint32_t>(header.xsize[0] + 256 * header.xsize[1] + 65536 * header.xsize[2]),
	    /* height = */ static_cast<uint32_t>(header.ysize[0] + 256 * header.ysize[1] + 65536 * header.ysize[2]),
	    /* depth  = */ static_cast<uint32_t>(header.zsize[0] + 256 * header.zsize[1] + 65536 * header.zsize[2])};

	decode(blockdim, extent, data.data() + sizeof(AstcHeader));
}

}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
