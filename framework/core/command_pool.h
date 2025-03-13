/* Copyright (c) 2019-2025, Arm Limited and Contributors
 * Copyright (c) 2024-2025, NVIDIA CORPORATION. All rights reserved.
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

#include "core/hpp_device.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
class Device;
class RenderFrame;

namespace rendering
{
class HPPRenderFrame;
}

namespace core
{
template <vkb::BindingType bindingType>
class CommandBuffer;
using CommandBufferCpp = CommandBuffer<vkb::BindingType::Cpp>;

namespace
{
// type trait to get the default value for request_command_buffer
template <typename T>
struct DefaultCommandBufferLevelValue;
template <>
struct DefaultCommandBufferLevelValue<vk::CommandBufferLevel>
{
	static constexpr vk::CommandBufferLevel value = vk::CommandBufferLevel::ePrimary;
};
template <>
struct DefaultCommandBufferLevelValue<VkCommandBufferLevel>
{
	static constexpr VkCommandBufferLevel value = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
};
}        // namespace

template <vkb::BindingType bindingType>
class CommandPool
{
  public:
	using CommandBufferLevelType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandBufferLevel, VkCommandBufferLevel>::type;
	using CommandPoolType        = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandPool, VkCommandPool>::type;

	using DeviceType      = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPDevice, vkb::Device>::type;
	using RenderFrameType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPRenderFrame, vkb::RenderFrame>::type;

  public:
	CommandPool(DeviceType                 &device,
	            uint32_t                    queue_family_index,
	            RenderFrameType            *render_frame = nullptr,
	            size_t                      thread_index = 0,
	            vkb::CommandBufferResetMode reset_mode   = vkb::CommandBufferResetMode::ResetPool);
	CommandPool(CommandPool<bindingType> const &) = delete;
	CommandPool(CommandPool<bindingType> &&other) = default;
	~CommandPool();

	CommandPool &operator=(CommandPool<bindingType> const &) = delete;
	CommandPool &operator=(CommandPool<bindingType> &&)      = default;

	DeviceType                            &get_device();
	CommandPoolType                        get_handle() const;
	uint32_t                               get_queue_family_index() const;
	RenderFrameType                       *get_render_frame();
	vkb::CommandBufferResetMode            get_reset_mode() const;
	size_t                                 get_thread_index() const;
	vkb::core::CommandBuffer<bindingType> &request_command_buffer(CommandBufferLevelType level = DefaultCommandBufferLevelValue<CommandBufferLevelType>::value);
	void                                   reset_pool();

  private:
	vkb::core::CommandBufferCpp &request_command_buffer_impl(vkb::core::CommandPool<vkb::BindingType::Cpp> &command_pool, vk::CommandBufferLevel level);

  private:
	vkb::core::HPPDevice                    &device;
	vk::CommandPool                          handle             = nullptr;
	vkb::rendering::HPPRenderFrame          *render_frame       = nullptr;
	size_t                                   thread_index       = 0;
	uint32_t                                 queue_family_index = 0;
	std::vector<vkb::core::CommandBufferCpp> primary_command_buffers;
	uint32_t                                 active_primary_command_buffer_count = 0;
	std::vector<vkb::core::CommandBufferCpp> secondary_command_buffers;
	uint32_t                                 active_secondary_command_buffer_count = 0;
	vkb::CommandBufferResetMode              reset_mode                            = vkb::CommandBufferResetMode::ResetPool;
};

using CommandPoolC   = CommandPool<vkb::BindingType::C>;
using CommandPoolCpp = CommandPool<vkb::BindingType::Cpp>;
}        // namespace core
}        // namespace vkb

#include "core/command_buffer.h"

namespace vkb
{
namespace core
{
template <vkb::BindingType bindingType>
inline vkb::core::CommandPool<bindingType>::CommandPool(
    DeviceType &device_, uint32_t queue_family_index, RenderFrameType *render_frame_, size_t thread_index, vkb::CommandBufferResetMode reset_mode) :
    device{reinterpret_cast<vkb::core::HPPDevice &>(device_)}, render_frame{reinterpret_cast<vkb::rendering::HPPRenderFrame *>(render_frame_)}, thread_index{thread_index}, reset_mode{reset_mode}
{
	vk::CommandPoolCreateFlags flags;
	switch (reset_mode)
	{
		case vkb::CommandBufferResetMode::ResetIndividually:
		case vkb::CommandBufferResetMode::AlwaysAllocate:
			flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
			break;
		case vkb::CommandBufferResetMode::ResetPool:
		default:
			flags = vk::CommandPoolCreateFlagBits::eTransient;
			break;
	}

	vk::CommandPoolCreateInfo command_pool_create_info(flags, queue_family_index);

	handle = device.get_handle().createCommandPool(command_pool_create_info);
}

template <vkb::BindingType bindingType>
inline CommandPool<bindingType>::~CommandPool()
{
	// clear command buffers before destroying the command pool
	primary_command_buffers.clear();
	secondary_command_buffers.clear();

	// Destroy command pool
	if (handle)
	{
		device.get_handle().destroyCommandPool(handle);
	}
}

template <vkb::BindingType bindingType>
inline typename vkb::core::CommandPool<bindingType>::DeviceType &CommandPool<bindingType>::get_device()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return device;
	}
	else
	{
		return reinterpret_cast<vkb::Device &>(device);
	}
}

template <vkb::BindingType bindingType>
inline typename vkb::core::CommandPool<bindingType>::CommandPoolType CommandPool<bindingType>::get_handle() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return handle;
	}
	else
	{
		return static_cast<VkCommandPool>(handle);
	}
}

template <vkb::BindingType bindingType>
inline uint32_t CommandPool<bindingType>::get_queue_family_index() const
{
	return queue_family_index;
}

template <vkb::BindingType bindingType>
inline typename vkb::core::CommandPool<bindingType>::RenderFrameType *CommandPool<bindingType>::get_render_frame()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return render_frame;
	}
	else
	{
		return reinterpret_cast<vkb::RenderFrame *>(render_frame);
	}
}

template <vkb::BindingType bindingType>
inline vkb::CommandBufferResetMode CommandPool<bindingType>::get_reset_mode() const
{
	return reset_mode;
}

template <vkb::BindingType bindingType>
inline size_t CommandPool<bindingType>::get_thread_index() const
{
	return thread_index;
}

template <vkb::BindingType bindingType>
vkb::core::CommandBuffer<bindingType> &CommandPool<bindingType>::request_command_buffer(CommandBufferLevelType level)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return request_command_buffer_impl(*this, level);
	}
	else
	{
		return reinterpret_cast<vkb::core::CommandBufferC &>(request_command_buffer_impl(reinterpret_cast<vkb::core::CommandPoolCpp &>(*this), static_cast<vk::CommandBufferLevel>(level)));
	}
}

template <vkb::BindingType bindingType>
inline vkb::core::CommandBufferCpp &CommandPool<bindingType>::request_command_buffer_impl(vkb::core::CommandPoolCpp &command_pool, vk::CommandBufferLevel level)
{
	if (level == vk::CommandBufferLevel::ePrimary)
	{
		if (active_primary_command_buffer_count < primary_command_buffers.size())
		{
			return primary_command_buffers[active_primary_command_buffer_count++];
		}

		primary_command_buffers.emplace_back(command_pool, level);

		active_primary_command_buffer_count++;

		return primary_command_buffers.back();
	}
	else
	{
		if (active_secondary_command_buffer_count < secondary_command_buffers.size())
		{
			return secondary_command_buffers[active_secondary_command_buffer_count++];
		}

		secondary_command_buffers.emplace_back(command_pool, level);

		active_secondary_command_buffer_count++;

		return secondary_command_buffers.back();
	}
}

template <vkb::BindingType bindingType>
inline void CommandPool<bindingType>::reset_pool()
{
	switch (reset_mode)
	{
		case vkb::CommandBufferResetMode::ResetIndividually:
			for (auto &cmd_buf : primary_command_buffers)
			{
				cmd_buf.reset(reset_mode);
			}
			active_primary_command_buffer_count = 0;

			for (auto &cmd_buf : secondary_command_buffers)
			{
				cmd_buf.reset(reset_mode);
			}
			active_secondary_command_buffer_count = 0;
			break;

		case vkb::CommandBufferResetMode::ResetPool:
			device.get_handle().resetCommandPool(handle);
			active_primary_command_buffer_count   = 0;
			active_secondary_command_buffer_count = 0;
			break;

		case vkb::CommandBufferResetMode::AlwaysAllocate:
			primary_command_buffers.clear();
			active_primary_command_buffer_count = 0;
			secondary_command_buffers.clear();
			active_secondary_command_buffer_count = 0;
			break;

		default:
			throw std::runtime_error("Unknown reset mode for command pools");
	}
}

}        // namespace core
}        // namespace vkb