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

#include "core/command_pool_base.h"
#include "core/device.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace rendering
{
template <vkb::BindingType bindingType>
class RenderFrame;
using RenderFrameC   = RenderFrame<vkb::BindingType::C>;
using RenderFrameCpp = RenderFrame<vkb::BindingType::Cpp>;
}        // namespace rendering

namespace core
{
template <vkb::BindingType bindingType>
class CommandBuffer;
using CommandBufferC   = CommandBuffer<vkb::BindingType::C>;
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
class CommandPool : private vkb::core::CommandPoolBase
{
  public:
	using CommandBufferLevelType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandBufferLevel, VkCommandBufferLevel>::type;
	using CommandPoolType        = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandPool, VkCommandPool>::type;

  public:
	CommandPool(vkb::core::Device<bindingType>           &device,
	            uint32_t                                  queue_family_index,
	            vkb::rendering::RenderFrame<bindingType> *render_frame = nullptr,
	            size_t                                    thread_index = 0,
	            vkb::CommandBufferResetMode               reset_mode   = vkb::CommandBufferResetMode::ResetIndividually);
	CommandPool(CommandPool<bindingType> const &)            = delete;
	CommandPool(CommandPool<bindingType> &&other)            = default;
	CommandPool &operator=(CommandPool<bindingType> const &) = delete;
	CommandPool &operator=(CommandPool<bindingType> &&other) = default;
	~CommandPool()                                           = default;

	vkb::core::Device<bindingType>                        &get_device();
	CommandPoolType                                        get_handle() const;
	uint32_t                                               get_queue_family_index() const;
	vkb::rendering::RenderFrame<bindingType>              *get_render_frame();
	vkb::CommandBufferResetMode                            get_reset_mode() const;
	size_t                                                 get_thread_index() const;
	std::shared_ptr<vkb::core::CommandBuffer<bindingType>> request_command_buffer(CommandBufferLevelType level = DefaultCommandBufferLevelValue<CommandBufferLevelType>::value);
	void                                                   reset_pool();
};

using CommandPoolC   = CommandPool<vkb::BindingType::C>;
using CommandPoolCpp = CommandPool<vkb::BindingType::Cpp>;

template <vkb::BindingType bindingType>
inline vkb::core::CommandPool<bindingType>::CommandPool(vkb::core::Device<bindingType>           &device,
                                                        uint32_t                                  queue_family_index,
                                                        vkb::rendering::RenderFrame<bindingType> *render_frame,
                                                        size_t                                    thread_index,
                                                        vkb::CommandBufferResetMode               reset_mode) :
    CommandPoolBase(reinterpret_cast<vkb::core::DeviceCpp &>(device),
                    queue_family_index,
                    reinterpret_cast<vkb::rendering::RenderFrameCpp *>(render_frame),
                    thread_index,
                    reset_mode)
{}

template <vkb::BindingType bindingType>
inline typename vkb::core::Device<bindingType> &CommandPool<bindingType>::get_device()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return CommandPoolBase::get_device();
	}
	else
	{
		return reinterpret_cast<vkb::core::DeviceC &>(CommandPoolBase::get_device());
	}
}

template <vkb::BindingType bindingType>
inline typename vkb::core::CommandPool<bindingType>::CommandPoolType CommandPool<bindingType>::get_handle() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return CommandPoolBase::get_handle();
	}
	else
	{
		return static_cast<VkCommandPool>(CommandPoolBase::get_handle());
	}
}

template <vkb::BindingType bindingType>
inline uint32_t CommandPool<bindingType>::get_queue_family_index() const
{
	return CommandPoolBase::get_queue_family_index();
}

template <vkb::BindingType bindingType>
inline vkb::rendering::RenderFrame<bindingType> *CommandPool<bindingType>::get_render_frame()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return CommandPoolBase::get_render_frame();
	}
	else
	{
		return reinterpret_cast<vkb::rendering::RenderFrameC *>(CommandPoolBase::get_render_frame());
	}
}

template <vkb::BindingType bindingType>
inline vkb::CommandBufferResetMode CommandPool<bindingType>::get_reset_mode() const
{
	return CommandPoolBase::get_reset_mode();
}

template <vkb::BindingType bindingType>
inline size_t CommandPool<bindingType>::get_thread_index() const
{
	return CommandPoolBase::get_thread_index();
}

template <vkb::BindingType bindingType>
std::shared_ptr<vkb::core::CommandBuffer<bindingType>> CommandPool<bindingType>::request_command_buffer(CommandBufferLevelType level)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return CommandPoolBase::request_command_buffer(*this, level);
	}
	else
	{
		std::shared_ptr<vkb::core::CommandBufferCpp> command_buffer =
		    CommandPoolBase::request_command_buffer(reinterpret_cast<vkb::core::CommandPoolCpp &>(*this), static_cast<vk::CommandBufferLevel>(level));
		return *reinterpret_cast<std::shared_ptr<CommandBufferC> *>(&command_buffer);
	}
}

template <vkb::BindingType bindingType>
inline void CommandPool<bindingType>::reset_pool()
{
	CommandPoolBase::reset_pool();
}

}        // namespace core
}        // namespace vkb