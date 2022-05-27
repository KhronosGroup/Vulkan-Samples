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

#include "components/images/astc.hpp"

#include <functional>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>

#include <components/common/hash.hpp>

namespace components
{
namespace images
{
struct BlockDim
{
	uint8_t x;
	uint8_t y;
	uint8_t z;
};

namespace detail
{
bool is_astc(const VkFormat format)
{
	return (format == VK_FORMAT_ASTC_4x4_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_4x4_SRGB_BLOCK ||
	        format == VK_FORMAT_ASTC_5x4_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_5x4_SRGB_BLOCK ||
	        format == VK_FORMAT_ASTC_5x5_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_5x5_SRGB_BLOCK ||
	        format == VK_FORMAT_ASTC_6x5_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_6x5_SRGB_BLOCK ||
	        format == VK_FORMAT_ASTC_6x6_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_6x6_SRGB_BLOCK ||
	        format == VK_FORMAT_ASTC_8x5_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_8x5_SRGB_BLOCK ||
	        format == VK_FORMAT_ASTC_8x6_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_8x6_SRGB_BLOCK ||
	        format == VK_FORMAT_ASTC_8x8_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_8x8_SRGB_BLOCK ||
	        format == VK_FORMAT_ASTC_10x5_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_10x5_SRGB_BLOCK ||
	        format == VK_FORMAT_ASTC_10x6_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_10x6_SRGB_BLOCK ||
	        format == VK_FORMAT_ASTC_10x8_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_10x8_SRGB_BLOCK ||
	        format == VK_FORMAT_ASTC_10x10_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_10x10_SRGB_BLOCK ||
	        format == VK_FORMAT_ASTC_12x10_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_12x10_SRGB_BLOCK ||
	        format == VK_FORMAT_ASTC_12x12_UNORM_BLOCK ||
	        format == VK_FORMAT_ASTC_12x12_SRGB_BLOCK);
}

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

bool config_is_same(const astcenc_config &first, const astcenc_config &second)
{
	return first.profile == second.profile &&
	       first.flags == second.flags &&
	       first.block_x == second.block_x &&
	       first.block_y == second.block_y &&
	       first.block_z == second.block_z &&
	       first.cw_r_weight == second.cw_r_weight &&
	       first.cw_g_weight == second.cw_g_weight &&
	       first.cw_b_weight == second.cw_b_weight &&
	       first.cw_a_weight == second.cw_a_weight &&
	       first.a_scale_radius == second.a_scale_radius &&
	       first.rgbm_m_scale == second.rgbm_m_scale &&
	       first.tune_partition_count_limit == second.tune_partition_count_limit &&
	       first.tune_partition_index_limit == second.tune_partition_index_limit &&
	       first.tune_block_mode_limit == second.tune_block_mode_limit &&
	       first.tune_refinement_limit == second.tune_refinement_limit &&
	       first.tune_candidate_limit == second.tune_candidate_limit &&
	       first.tune_db_limit == second.tune_db_limit &&
	       first.tune_mode0_mse_overshoot == second.tune_mode0_mse_overshoot &&
	       first.tune_refinement_mse_overshoot == second.tune_refinement_mse_overshoot &&
	       first.tune_2_partition_early_out_limit_factor == second.tune_2_partition_early_out_limit_factor &&
	       first.tune_3_partition_early_out_limit_factor == second.tune_3_partition_early_out_limit_factor &&
	       first.tune_2_plane_early_out_limit_correlation == second.tune_2_plane_early_out_limit_correlation &&
	       first.tune_low_weight_count_limit == second.tune_low_weight_count_limit;
}

struct ConfigWrapper
{
	astcenc_config config;

	bool operator==(const ConfigWrapper &other) const
	{
		return config_is_same(config, other.config);
	}
};
}        // namespace detail
}        // namespace images
}        // namespace components

namespace std
{
template <>
struct hash<components::images::detail::ConfigWrapper>
{
	std::size_t operator()(const components::images::detail::ConfigWrapper &wrapper) const
	{
		size_t seed = 0;
		components::common::hash_combine(seed, wrapper.config.profile);
		components::common::hash_combine(seed, wrapper.config.flags);
		components::common::hash_combine(seed, wrapper.config.block_x);
		components::common::hash_combine(seed, wrapper.config.block_y);
		components::common::hash_combine(seed, wrapper.config.block_z);
		components::common::hash_combine(seed, wrapper.config.cw_r_weight);
		components::common::hash_combine(seed, wrapper.config.cw_g_weight);
		components::common::hash_combine(seed, wrapper.config.cw_b_weight);
		components::common::hash_combine(seed, wrapper.config.cw_a_weight);
		components::common::hash_combine(seed, wrapper.config.a_scale_radius);
		components::common::hash_combine(seed, wrapper.config.rgbm_m_scale);
		components::common::hash_combine(seed, wrapper.config.tune_partition_count_limit);
		components::common::hash_combine(seed, wrapper.config.tune_partition_index_limit);
		components::common::hash_combine(seed, wrapper.config.tune_block_mode_limit);
		components::common::hash_combine(seed, wrapper.config.tune_refinement_limit);
		components::common::hash_combine(seed, wrapper.config.tune_candidate_limit);
		components::common::hash_combine(seed, wrapper.config.tune_db_limit);
		components::common::hash_combine(seed, wrapper.config.tune_mode0_mse_overshoot);
		components::common::hash_combine(seed, wrapper.config.tune_refinement_mse_overshoot);
		components::common::hash_combine(seed, wrapper.config.tune_2_partition_early_out_limit_factor);
		components::common::hash_combine(seed, wrapper.config.tune_3_partition_early_out_limit_factor);
		components::common::hash_combine(seed, wrapper.config.tune_2_plane_early_out_limit_correlation);
		components::common::hash_combine(seed, wrapper.config.tune_low_weight_count_limit);
		return seed;
	}
};
}        // namespace std

namespace components
{
namespace images
{
namespace detail
{
class ContextTracker
{
	struct AstcContext
	{
		astcenc_context *ptr{nullptr};
		uint32_t         ref_count{0};
	};

	using ContextMap = std::unordered_map<ConfigWrapper, AstcContext>;

  public:
	static ContextTracker &get()
	{
		static ContextTracker tracker;
		return tracker;
	}

	astcenc_context *alloc_context(const astcenc_config &config)
	{
		std::lock_guard<std::mutex> lock{m_mut};

		auto contexts_it = m_contexts.find(std::this_thread::get_id());
		if (contexts_it == m_contexts.end())
		{
			auto res    = m_contexts.emplace(std::this_thread::get_id(), ContextMap{});
			contexts_it = res.first;
		}

		auto &contexts = contexts_it->second;

		auto context_it = contexts.find(ConfigWrapper{config});
		if (context_it == contexts.end())
		{
			AstcContext   context;
			astcenc_error status = astcenc_context_alloc(&config, 1, &context.ptr);
			if (status != ASTCENC_SUCCESS)
			{
				return nullptr;
			}

			auto res   = contexts.emplace(ConfigWrapper{config}, context);
			context_it = res.first;
		}

		context_it->second.ref_count++;
		return context_it->second.ptr;
	}

	void free_context(const astcenc_config &config)
	{
		std::lock_guard<std::mutex> lock{m_mut};

		auto contexts_it = m_contexts.find(std::this_thread::get_id());
		if (contexts_it == m_contexts.end())
		{
			return;
		}

		auto &contexts = contexts_it->second;

		auto context_it = contexts.find(ConfigWrapper{config});
		if (context_it == contexts.end())
		{
			return;
		}

		context_it->second.ref_count--;

		if (context_it->second.ref_count == 0)
		{
			// no more contexts in use
			astcenc_context_free(context_it->second.ptr);
			contexts.erase(context_it);
		}
	}

  private:
	ContextTracker() = default;

	~ContextTracker()
	{
		std::lock_guard<std::mutex> lock{m_mut};

		for (auto &thread : m_contexts)
		{
			for (auto &context : thread.second)
			{
				if (context.second.ptr)
				{
					astcenc_context_free(context.second.ptr);
				}
			}
		}
	}

	std::mutex m_mut;

	std::unordered_map<std::thread::id, ContextMap> m_contexts;
};        // namespace detail
}        // namespace detail

void AstcCodec::encode(const assets::ImageAsset &image, assets::ImageAssetPtr */* o_image */) const
{
	if (!image.valid())
	{
		ERROR("image not valid");
	}

	// BlockDim blockdim = detail::to_blockdim(image.format);

	// astcenc_config config;
	// astcenc_error  status;
	// status = astcenc_config_init(ASTCENC_PRF_LDR_SRGB, blockdim.x, blockdim.y, blockdim.z, ASTCENC_PRE_MEDIUM, 0, &config);
	// if (status != ASTCENC_SUCCESS)
	// {
	// 	return StackError::unique("ASTC Codec config init failed: " + std::string{astcenc_get_error_string(status)}, "images/astc.cpp", __LINE__);
	// }

	// auto &tracker = detail::ContextTracker::get();
	// auto *context = tracker.alloc_context(config);

	ERROR("not implemented");
}

void AstcCodec::decode(const assets::ImageAsset &image, assets::ImageAssetPtr *o_image) const
{
	if (!image.valid())
	{
		ERROR("image not valid");
	}

	if (!detail::is_astc(image.format))
	{
		ERROR("input format not supported by this codec");
	}

	BlockDim blockdim = detail::to_blockdim(image.format);

	astcenc_config config;
	astcenc_error  status;
	status = astcenc_config_init(ASTCENC_PRF_LDR_SRGB, blockdim.x, blockdim.y, blockdim.z, ASTCENC_PRE_MEDIUM, 0, &config);
	if (status != ASTCENC_SUCCESS)
	{
		ERRORF("ASTC Codec config init failed: {}", astcenc_get_error_string(status));
	}

	auto &tracker = detail::ContextTracker::get();
	auto *context = tracker.alloc_context(config);

	astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};

	assets::ImageAssetPtr output_image = std::shared_ptr<assets::ImageAsset>();
	output_image->name    = image.name;
	output_image->format  = VK_FORMAT_R8G8B8A8_SRGB;
	output_image->layers  = image.layers;
	output_image->mips.reserve(image.mips.size());

	size_t running_offset = 0;
	for (const auto &mip : image.mips)
	{
		astcenc_image astc_image;
		astc_image.dim_x     = mip.extent.width;
		astc_image.dim_y     = mip.extent.height;
		astc_image.dim_z     = mip.extent.depth;
		astc_image.data_type = ASTCENC_TYPE_U8;

		size_t expected_output_size = mip.extent.width * mip.extent.height * mip.extent.depth * 4 * sizeof(float);
		size_t last_size            = output_image->data.size();
		output_image->data.resize(last_size + expected_output_size);
		uint8_t *data_ptr = output_image->data.data() + last_size;
		astc_image.data   = reinterpret_cast<void **>(&data_ptr);

		status = astcenc_decompress_image(context, image.data.data() + mip.offset, mip.byte_length, &astc_image, &swizzle, 0);
		if (status != ASTCENC_SUCCESS)
		{
			ERRORF("ASTC Codec decode failed: {}", astcenc_get_error_string(status));
		}

		assets::Mipmap copy      = mip;
		copy.byte_length = expected_output_size;
		copy.offset      = running_offset;
		output_image->mips.push_back(copy);

		running_offset += expected_output_size;
	}

	tracker.free_context(config);

	*o_image = output_image;
}

}        // namespace images
}        // namespace components
