/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "core/vulkan_resource.h"
#include <core/hpp_command_buffer.h>
#include <core/hpp_command_pool.h>
#include <core/hpp_debug.h>
#include <core/hpp_physical_device.h>
#include <core/hpp_queue.h>
#include <hpp_fence_pool.h>
#include <hpp_resource_cache.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
class HPPBuffer;

class HPPDevice : public vkb::core::VulkanResource<vkb::BindingType::Cpp, vk::Device>
{
  public:
	/**
	 * @brief HPPDevice constructor
	 * @param gpu A valid Vulkan physical device and the requested gpu features
	 * @param surface The surface
	 * @param debug_utils The debug utils to be associated to this device
	 * @param requested_extensions (Optional) List of required device extensions and whether support is optional or not
	 */
	HPPDevice(vkb::core::HPPPhysicalDevice               &gpu,
	          vk::SurfaceKHR                              surface,
	          std::unique_ptr<vkb::core::HPPDebugUtils> &&debug_utils,
	          std::unordered_map<const char *, bool>      requested_extensions = {});

	HPPDevice(const HPPDevice &) = delete;

	HPPDevice(HPPDevice &&) = delete;

	~HPPDevice();

	HPPDevice &operator=(const HPPDevice &) = delete;

	HPPDevice &operator=(HPPDevice &&) = delete;

	vkb::core::HPPPhysicalDevice const &get_gpu() const;

	/**
	 * @brief Returns the debug utils associated with this HPPDevice.
	 */
	vkb::core::HPPDebugUtils const &get_debug_utils() const;

	vkb::core::HPPQueue const &get_queue(uint32_t queue_family_index, uint32_t queue_index) const;

	vkb::core::HPPQueue const &get_queue_by_flags(vk::QueueFlags queue_flags, uint32_t queue_index) const;

	vkb::core::HPPQueue const &get_queue_by_present(uint32_t queue_index) const;

	/**
	 * @brief Finds a suitable graphics queue to submit to
	 * @return The first present supported queue, otherwise just any graphics queue
	 */
	vkb::core::HPPQueue const &get_suitable_graphics_queue() const;

	bool is_extension_supported(std::string const &extension) const;

	bool is_enabled(std::string const &extension) const;

	uint32_t get_queue_family_index(vk::QueueFlagBits queue_flag) const;

	vkb::core::HPPCommandPool &get_command_pool();

	/**
	 * @brief Creates a vulkan image and associated device memory
	 * @param format The image format
	 * @param extent The image extent
	 * @param mip_levels The mip levels of the image
	 * @param usage The image usage
	 * @param properties The device memory property flags
	 * @returns A valid vk::Image and a corresponding vk::DeviceMemory
	 */
	std::pair<vk::Image, vk::DeviceMemory> create_image(vk::Format format, vk::Extent2D const &extent, uint32_t mip_levels, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties) const;

	/**
	 * @brief Copies a buffer from one to another
	 * @param src The buffer to copy from
	 * @param dst The buffer to copy to
	 * @param queue The queue to submit the copy command to
	 * @param copy_region The amount to copy, if null copies the entire buffer
	 */
	void copy_buffer(vkb::core::HPPBuffer &src, vkb::core::HPPBuffer &dst, vk::Queue queue, vk::BufferCopy *copy_region = nullptr) const;

	/**
	 * @brief Requests a command buffer from the device's command pool
	 * @param level The command buffer level
	 * @param begin Whether the command buffer should be implicitly started before it's returned
	 * @returns A valid vk::CommandBuffer
	 */
	vk::CommandBuffer create_command_buffer(vk::CommandBufferLevel level, bool begin = false) const;

	/**
	 * @brief Submits and frees up a given command buffer
	 * @param command_buffer The command buffer
	 * @param queue The queue to submit the work to
	 * @param free Whether the command buffer should be implicitly freed up
	 * @param signalSemaphore An optional semaphore to signal when the commands have been executed
	 */
	void flush_command_buffer(vk::CommandBuffer command_buffer, vk::Queue queue, bool free = true, vk::Semaphore signalSemaphore = VK_NULL_HANDLE) const;

	vkb::HPPFencePool &get_fence_pool();

	vkb::HPPResourceCache &get_resource_cache();

  private:
	vkb::core::HPPPhysicalDevice const &gpu;

	vk::SurfaceKHR surface{nullptr};

	std::unique_ptr<vkb::core::HPPDebugUtils> debug_utils;

	std::vector<const char *> enabled_extensions{};

	std::vector<std::vector<vkb::core::HPPQueue>> queues;

	/// A command pool associated to the primary queue
	std::unique_ptr<vkb::core::HPPCommandPool> command_pool;

	/// A fence pool associated to the primary queue
	std::unique_ptr<vkb::HPPFencePool> fence_pool;

	vkb::HPPResourceCache resource_cache;
};
}        // namespace core
}        // namespace vkb
