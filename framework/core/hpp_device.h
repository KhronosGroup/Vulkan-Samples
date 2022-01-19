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

#include <core/device.h>

#include <core/hpp_debug.h>
#include <core/hpp_physical_device.h>
#include <core/hpp_queue.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
/**
 * @brief facade class around vkb::Device, providing a vulkan.hpp-based interface
 *
 * See vkb::Device for documentation
 */
class HPPDevice : protected vkb::Device
{
  public:
	using vkb::Device::get_driver_version;
	using vkb::Device::get_memory_allocator;

	HPPDevice(vkb::core::HPPPhysicalDevice &              gpu,
	          vk::SurfaceKHR                              surface,
	          std::unique_ptr<vkb::core::HPPDebugUtils> &&debug_utils,
	          std::unordered_map<const char *, bool>      requested_extensions = {}) :
	    vkb::Device(reinterpret_cast<vkb::PhysicalDevice &>(gpu),
	                static_cast<VkSurfaceKHR>(surface),
	                std::unique_ptr<vkb::DebugUtils>(reinterpret_cast<vkb::DebugUtils *>(debug_utils.release())),
	                requested_extensions)
	{}

	vk::CommandBuffer create_command_buffer(vk::CommandBufferLevel level, bool begin = false) const
	{
		return vkb::Device::create_command_buffer(static_cast<VkCommandBufferLevel>(level), begin);
	}

	void flush_command_buffer(vk::CommandBuffer command_buffer, vk::Queue queue, bool free = true, vk::Semaphore signalSemaphore = nullptr) const
	{
		vkb::Device::flush_command_buffer(command_buffer, queue, free, signalSemaphore);
	}

	vk::Device get_handle() const
	{
		return vkb::Device::get_handle();
	}

	vkb::core::HPPPhysicalDevice const &get_gpu() const
	{
		return reinterpret_cast<vkb::core::HPPPhysicalDevice const &>(vkb::Device::get_gpu());
	}

	uint32_t get_memory_type(uint32_t bits, vk::MemoryPropertyFlags properties, vk::Bool32 *memory_type_found = nullptr) const
	{
		return vkb::Device::get_memory_type(bits, static_cast<VkMemoryPropertyFlags>(properties), memory_type_found);
	}

	vkb::core::HPPQueue const &get_queue_by_flags(vk::QueueFlags queue_flags, uint32_t queue_index) const
	{
		return reinterpret_cast<vkb::core::HPPQueue const &>(vkb::Device::get_queue_by_flags(static_cast<VkQueueFlags>(queue_flags), queue_index));
	}

	vkb::core::HPPQueue const &get_queue_by_present(uint32_t queue_index) const
	{
		return reinterpret_cast<vkb::core::HPPQueue const &>(vkb::Device::get_queue_by_present(queue_index));
	}

	vkb::core::HPPQueue const &get_suitable_graphics_queue() const
	{
		return reinterpret_cast<vkb::core::HPPQueue const &>(vkb::Device::get_suitable_graphics_queue());
	}

	vk::Result wait_idle() const
	{
		return static_cast<vk::Result>(vkb::Device::wait_idle());
	}
};
}        // namespace core
}        // namespace vkb
