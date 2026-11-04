/* Copyright (c) 2026, NVIDIA CORPORATION. All rights reserved.
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

#pragma once

#include "vk_common.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace common
{
/**
 * @brief Buffer memory barrier structure used to define
 *        memory access for a buffer during command recording.
 */
template <vkb::BindingType bindingType>
struct BufferMemoryBarrier
{
	using AccessFlagsType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::AccessFlags, VkAccessFlags>::type;
	using PipelineStageFlagsType = typename std::
	    conditional<bindingType == vkb::BindingType::Cpp, vk::PipelineStageFlags, VkPipelineStageFlags>::type;

  private:
	constexpr PipelineStageFlagsType src_stage_mask_default() const
	{
		if constexpr (bindingType == vkb::BindingType::Cpp)
		{
			return vk::PipelineStageFlagBits::eBottomOfPipe;
		}
		else
		{
			return static_cast<VkPipelineStageFlags>(BufferMemoryBarrier<vkb::BindingType::Cpp>{}.src_stage_mask);
		}
	}

	constexpr PipelineStageFlagsType dst_stage_mask_default() const
	{
		if constexpr (bindingType == vkb::BindingType::Cpp)
		{
			return vk::PipelineStageFlagBits::eTopOfPipe;
		}
		else
		{
			return static_cast<VkPipelineStageFlags>(BufferMemoryBarrier<vkb::BindingType::Cpp>{}.dst_stage_mask);
		}
	}

  public:
	PipelineStageFlagsType src_stage_mask  = src_stage_mask_default();
	PipelineStageFlagsType dst_stage_mask  = dst_stage_mask_default();
	AccessFlagsType        src_access_mask = {};
	AccessFlagsType        dst_access_mask = {};
};

using BufferMemoryBarrierC   = BufferMemoryBarrier<vkb::BindingType::C>;
using BufferMemoryBarrierCpp = BufferMemoryBarrier<vkb::BindingType::Cpp>;
}        // namespace common
}        // namespace vkb
