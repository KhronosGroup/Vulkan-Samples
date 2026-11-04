/* Copyright (c) 2023-2026, NVIDIA CORPORATION. All rights reserved.
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
 * @brief Image memory barrier structure used to define
 *        memory access for an image view during command recording.
 */
template <vkb::BindingType bindingType>
struct ImageMemoryBarrier
{
	using AccessFlagsType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::AccessFlags, VkAccessFlags>::type;
	using PipelineStageFlagsType = typename std::
	    conditional<bindingType == vkb::BindingType::Cpp, vk::PipelineStageFlags, VkPipelineStageFlags>::type;
	using ImageLayoutType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ImageLayout, VkImageLayout>::type;

  private:
	constexpr PipelineStageFlagsType src_stage_mask_default() const
	{
		if constexpr (bindingType == vkb::BindingType::Cpp)
		{
			return vk::PipelineStageFlagBits::eBottomOfPipe;
		}
		else
		{
			return static_cast<VkPipelineStageFlags>(ImageMemoryBarrier<vkb::BindingType::Cpp>{}.src_stage_mask);
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
			return static_cast<VkPipelineStageFlags>(ImageMemoryBarrier<vkb::BindingType::Cpp>{}.dst_stage_mask);
		}
	}

	constexpr ImageLayoutType layout_default() const
	{
		if constexpr (bindingType == vkb::BindingType::Cpp)
		{
			return vk::ImageLayout::eUndefined;
		}
		else
		{
			return static_cast<VkImageLayout>(ImageMemoryBarrier<vkb::BindingType::Cpp>{}.old_layout);
		}
	}

  public:
	PipelineStageFlagsType src_stage_mask   = src_stage_mask_default();
	PipelineStageFlagsType dst_stage_mask   = dst_stage_mask_default();
	AccessFlagsType        src_access_mask  = {};
	AccessFlagsType        dst_access_mask  = {};
	ImageLayoutType        old_layout       = layout_default();
	ImageLayoutType        new_layout       = layout_default();
	uint32_t               src_queue_family = vk::QueueFamilyIgnored;
	uint32_t               dst_queue_family = vk::QueueFamilyIgnored;
};

using ImageMemoryBarrierC   = ImageMemoryBarrier<vkb::BindingType::C>;
using ImageMemoryBarrierCpp = ImageMemoryBarrier<vkb::BindingType::Cpp>;
}        // namespace common
}        // namespace vkb
