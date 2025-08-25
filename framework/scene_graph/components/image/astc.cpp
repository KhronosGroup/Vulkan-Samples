/* Copyright (c) 2019-2025, Arm Limited and Contributors
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

#include <filesystem>
#include <mutex>

#include "common/error.h"
#include "core/util/profiling.hpp"

#include "common/glm_common.h"
#if defined(_WIN32) || defined(_WIN64)
// Windows.h defines IGNORE, so we must #undef it to avoid clashes with astc header
#	undef IGNORE
#endif
#include <astcenc.h>

#include <filesystem/filesystem.hpp>

#define MAGIC_FILE_CONSTANT 0x5CA1AB13
#define ASTC_CACHE_DIRECTORY "cache/astc_to_bin"

constexpr uint32_t ASTC_CACHE_HEADER_SIZE = 64;
constexpr uint32_t ASTC_CACHE_SEED        = 1619;

namespace vkb
{
namespace sg
{

using Path = std::filesystem::path;

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

inline astcenc_profile to_profile(const VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
		case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
		case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
		case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
		case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
			return ASTCENC_PRF_LDR;
		case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
		case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
		case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
		case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
		case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
		case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
		case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
			return ASTCENC_PRF_LDR_SRGB;
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
	PROFILE_SCOPE("Decode ASTC Image");

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

	auto fs = vkb::filesystem::get();

	size_t key = ASTC_CACHE_SEED;
	glm::detail::hash_combine(key, image.get_data_hash());

	constexpr bool       use_cache                                 = true;
	constexpr uint32_t   bytes_per_pixel                           = 4;
	constexpr const char file_cache_header[ASTC_CACHE_HEADER_SIZE] = "ASTCConvertedDataV01";
	const auto           profile                                   = to_profile(image.get_format());

	auto can_load_from_file = [this, profile, fs, file_cache_header, bytes_per_pixel, use_cache](const Path &path, std::vector<uint8_t> &dst_data, uint32_t width, uint32_t height, uint32_t depth) {
		if (!use_cache)
		{
			LOGD("Device does not support ASTC format and cache is disabled. ASTC image {} will be decoded.", get_name())
			return false;
		}
		try
		{
			if (!fs->exists(path))
			{
				LOGW("Device does not support ASTC format and cache file {} does not exist. ASTC image {} will be decoded.", path.string(), get_name())
				return false;
			}
			else
			{
				LOGD("Loading ASTC image {} from cache file {}", get_name(), path.string())
			}
			size_t offset = 0;

			auto copy_from_file = [fs, path](void *dst, size_t *offset, size_t content_size) {
				const auto bin_content = fs->read_chunk(path, *offset, content_size);
				std::memcpy(dst, &bin_content[0], content_size);
				*offset += content_size;
			};

			char header[ASTC_CACHE_HEADER_SIZE];
			copy_from_file(&header, &offset, ASTC_CACHE_HEADER_SIZE);
			if (std::strcmp(header, file_cache_header) != 0)
			{
				return false;
			}

			uint32_t file_width, file_height, file_depth;
			copy_from_file(&file_width, &offset, sizeof(std::uint32_t));
			copy_from_file(&file_height, &offset, sizeof(std::uint32_t));
			copy_from_file(&file_depth, &offset, sizeof(std::uint32_t));

			if (file_width != width || width == 0 ||
			    file_height != height || height == 0 ||
			    file_depth != depth || depth == 0)
			{
				return false;
			}
			else
			{
				set_width(width);
				set_height(height);
				set_depth(depth);
				set_format(profile == ASTCENC_PRF_LDR_SRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);
			}

			auto image_size = width * height * depth * bytes_per_pixel;
			dst_data.resize(image_size);
			copy_from_file(dst_data.data(), &offset, image_size);

			return true;
		}
		catch (const std::runtime_error &e)
		{
			LOGE("ERROR loading file {} from cache. Error: <{}>", path.string(), e.what())
			// file is truncated
			return false;
		}
	};

	auto save_to_file = [fs, file_cache_header, bytes_per_pixel, use_cache](const Path &path, uint8_t *dst_data, uint32_t width, uint32_t height, uint32_t depth) {
		if (!use_cache)
		{
			return;
		}
		try
		{
			LOGI("Saving ASTC cache data to file: {}", path.string());

			auto image_size = width * height * depth * bytes_per_pixel;

			std::vector<uint8_t> astc_file_content;
			astc_file_content.reserve(sizeof(file_cache_header) + (3 * sizeof(std::uint32_t)) + image_size);

			auto append_to_file = [](std::vector<uint8_t> &dst_file, const std::uint8_t *content, size_t content_size) {
				dst_file.insert(dst_file.end(), content, content + content_size);
			};

			append_to_file(astc_file_content, (uint8_t *) &file_cache_header, sizeof(file_cache_header));
			append_to_file(astc_file_content, (uint8_t *) &width, sizeof(uint32_t));
			append_to_file(astc_file_content, (uint8_t *) &height, sizeof(uint32_t));
			append_to_file(astc_file_content, (uint8_t *) &depth, sizeof(uint32_t));
			append_to_file(astc_file_content, (uint8_t *) dst_data, image_size);

			fs->write_file(path, astc_file_content);
		}
		catch (const std::runtime_error &e)
		{
			LOGE("ERROR: saving to file: {}\nError<{}>", path.string(), e.what())
		}
	};

	// Locate mip #0 in the KTX. This is the first one in the data array for KTX1s, but the last one in KTX2s!
	auto mip_it = std::ranges::find_if(image.get_mipmaps(),
	                                   [](auto &mip) { return mip.level == 0; });
	assert(mip_it != image.get_mipmaps().end() && "Mip #0 not found");

	const std::string path = fmt::format("{}/{}.bin", ASTC_CACHE_DIRECTORY, uint64_t(key));

	if (!can_load_from_file(path, get_mut_data(), mip_it->extent.width, mip_it->extent.height, mip_it->extent.depth))
	{
		// When decoding ASTC on CPU (as it is the case in here), we don't decode all mips in the mip chain.
		// Instead, we just decode mip #0 and re-generate the other LODs later (via image->generate_mipmaps()).
		const auto     blockdim = to_blockdim(image.get_format());
		const auto    &extent   = mip_it->extent;
		auto           size     = extent.width * extent.height * extent.depth * 4;
		const uint8_t *data_ptr = image.get_data().data() + mip_it->offset;

		decode(blockdim, mip_it->extent, data_ptr, size);

		save_to_file(path, get_mut_data().data(), mip_it->extent.width, mip_it->extent.height, mip_it->extent.depth);
	}

	update_hash(image.get_data_hash());
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

	update_hash(get_data_hash());
}

}        // namespace sg
}        // namespace vkb
