/* Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
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

#include <core/command_buffer.h>

#include <common/hpp_vk_common.h>
#include <core/hpp_image_view.h>

namespace vkb
{
namespace core
{
class HPPCommandPool;

/**
 * @brief facade class around vkb::CommandBuffer, providing a vulkan.hpp-based interface
 *
 * See vkb::CommandBuffer for documentation
 */
class HPPCommandBuffer : private vkb::CommandBuffer
{
  public:
	using vkb::CommandBuffer::end;
	using vkb::CommandBuffer::end_render_pass;

	enum class ResetMode
	{
		ResetPool,
		ResetIndividually,
		AlwaysAllocate,
	};

	HPPCommandBuffer(HPPCommandPool &command_pool, vk::CommandBufferLevel level) :
	    vkb::CommandBuffer(reinterpret_cast<vkb::CommandPool &>(command_pool), static_cast<VkCommandBufferLevel>(level))
	{}

	vk::Result begin(vk::CommandBufferUsageFlags flags, HPPCommandBuffer *primary_cmd_buf = nullptr)
	{
		return static_cast<vk::Result>(vkb::CommandBuffer::begin(static_cast<VkCommandBufferUsageFlags>(flags), reinterpret_cast<CommandBuffer *>(primary_cmd_buf)));
	}

	vk::CommandBuffer get_handle() const
	{
		return static_cast<vk::CommandBuffer>(vkb::CommandBuffer::get_handle());
	}

	void image_memory_barrier(const vkb::core::HPPImageView &image_view, const vkb::common::HPPImageMemoryBarrier &memory_barrier) const
	{
		vkb::CommandBuffer::image_memory_barrier(reinterpret_cast<vkb::core::ImageView const &>(image_view),
		                                         reinterpret_cast<vkb::ImageMemoryBarrier const &>(memory_barrier));
	}

	void reset(ResetMode reset_mode)
	{
		VkResult result = vkb::CommandBuffer::reset(static_cast<CommandBuffer::ResetMode>(reset_mode));
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("vkCommandBufferReset failed with errorCode " + vk::to_string(static_cast<vk::Result>(result)));
		}
	}
};
}        // namespace core
}        // namespace vkb
