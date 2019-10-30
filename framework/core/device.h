/* Copyright (c) 2019, Arm Limited and Contributors
 * Copyright (c) 2019, Sascha Willems
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

#include "common/helpers.h"
#include "common/logging.h"
#include "common/vk_common.h"
#include "core/command_buffer.h"
#include "core/command_pool.h"
#include "core/descriptor_set.h"
#include "core/descriptor_set_layout.h"
#include "core/framebuffer.h"
#include "core/pipeline.h"
#include "core/pipeline_layout.h"
#include "core/queue.h"
#include "core/render_pass.h"
#include "core/shader_module.h"
#include "core/swapchain.h"
#include "fence_pool.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_target.h"
#include "resource_cache.h"

namespace vkb
{
class Device : public NonCopyable
{
  public:
	Device(VkPhysicalDevice physical_device, VkSurfaceKHR surface, std::vector<const char *> requested_extensions = {}, VkPhysicalDeviceFeatures features = {});

	~Device();

	const VkFormatProperties get_format_properties(VkFormat format) const;

	const Queue &get_queue(uint32_t queue_family_index, uint32_t queue_index);

	const Queue &get_queue_by_flags(VkQueueFlags queue_flags, uint32_t queue_index);

	const Queue &get_queue_by_present(uint32_t queue_index);

	/**
	 * @return Whether an image format is supported by the GPU
	 */
	bool is_image_format_supported(VkFormat format) const;

	bool is_extension_supported(const std::string &extension);

	uint32_t get_queue_family_index(VkQueueFlagBits queue_flag);

	/**
	 * @brief Checks that a given memory type is supported by the GPU
	 * @param bits The memory requirement type bits
	 * @param properties The memory property to search for
	 * @param memory_type_found True if found, false if not found
	 * @returns The memory type index of the found memory type
	 */
	uint32_t get_memory_type(uint32_t bits, VkMemoryPropertyFlags properties, VkBool32 *memory_type_found = nullptr);

	/**
	* @brief Creates a vulkan buffer
	* @param usage The buffer usage
	* @param properties The memory properties
	* @param size The size of the buffer
	* @param memory The pointer to the buffer memory
	* @param data The data to place inside the buffer
	* @returns A valid VkBuffer
	*/
	VkBuffer create_buffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceSize size, VkDeviceMemory *memory, void *data = nullptr);

	/**
	* @brief Copies a buffer from one to another
	* @param src The buffer to copy from
	* @param dst The buffer to copy to
	* @param queue The queue to submit the copy command to
	* @param copy_region The amount to copy, if null copies the entire buffer
	*/
	void copy_buffer(vkb::core::Buffer &src, vkb::core::Buffer &dst, VkQueue queue, VkBufferCopy *copy_region = nullptr);

	/**
	 * @brief Creates a command pool
	 * @param queue_index The queue index this command pool is associated with
	 * @param flags The command pool flags
	 * @returns A valid VkCommandPool
	 */
	VkCommandPool create_command_pool(uint32_t queue_index, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	/**
	 * @brief Requests a command buffer from the device's command pool
	 * @param level The command buffer level
	 * @param begin Whether the command buffer should be implictly started before it's returned
	 * @returns A valid VkCommandBuffer
	 */
	VkCommandBuffer create_command_buffer(VkCommandBufferLevel level, bool begin = false);

	/**
	 * @brief Submits and frees up a given command buffer
	 * @param command_buffer The command buffer
	 * @param queue The queue to submit the work to
	 * @param free Whether the command buffer should be implictly freed up
	 */
	void flush_command_buffer(VkCommandBuffer command_buffer, VkQueue queue, bool free = true);

	/**
	 * @brief Requests a command buffer from the general command_pool
	 * @return A new command buffer
	 */
	CommandBuffer &request_command_buffer();

	/**
	 * @brief Requests a fence to the fence pool
	 * @return A vulkan fence
	 */
	VkFence request_fence();

	VkResult wait_idle();

	VkPhysicalDevice get_physical_device() const;

	const VkPhysicalDeviceFeatures &get_features() const;

	VkDevice get_handle() const;

	VmaAllocator get_memory_allocator() const;

	const VkPhysicalDeviceProperties &get_properties() const;

	CommandPool &get_command_pool();

	FencePool &get_fence_pool();

	ResourceCache &get_resource_cache();

  private:
	std::vector<VkExtensionProperties> device_extensions;

	VkPhysicalDevice physical_device{VK_NULL_HANDLE};

	VkPhysicalDeviceFeatures features{};

	VkSurfaceKHR surface{VK_NULL_HANDLE};

	uint32_t queue_family_count{0};

	std::vector<VkQueueFamilyProperties> queue_family_properties;

	VkDevice handle{VK_NULL_HANDLE};

	VmaAllocator memory_allocator{VK_NULL_HANDLE};

	VkPhysicalDeviceProperties properties;

	VkPhysicalDeviceMemoryProperties memory_properties;

	std::vector<std::vector<Queue>> queues;

	/// A command pool associated to the primary queue
	std::unique_ptr<CommandPool> command_pool;

	/// A fence pool associated to the primary queue
	std::unique_ptr<FencePool> fence_pool;

	ResourceCache resource_cache;
};
}        // namespace vkb
