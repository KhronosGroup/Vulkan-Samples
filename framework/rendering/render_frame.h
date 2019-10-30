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

#pragma once

#include "buffer_pool.h"
#include "common/helpers.h"
#include "common/vk_common.h"
#include "core/buffer.h"
#include "core/command_pool.h"
#include "core/device.h"
#include "core/image.h"
#include "core/queue.h"
#include "fence_pool.h"
#include "rendering/render_target.h"
#include "semaphore_pool.h"

namespace vkb
{
/**
 * @brief RenderFrame is a container for per-frame data, including BufferPool objects,
 * synchronization primitives (semaphores, fences) and the swapchain RenderTarget.
 *
 * When creating a RenderTarget, we need to provide images that will be used as attachments
 * within a RenderPass. The RenderFrame is responsible for creating a RenderTarget using
 * RenderTarget::CreateFunc. A custom RenderTarget::CreateFunc can be provided if a different
 * render target is required.
 *
 * A RenderFrame cannot be destroyed individually since frames are managed by the RenderContext,
 * the whole context must be destroyed. This is because each RenderFrame holds Vulkan objects
 * such as the swapchain image.
 */
class RenderFrame : public NonCopyable
{
  public:
	/**
	 * @brief Block size of a buffer pool in kilobytes
	 */
	static constexpr uint32_t BUFFER_POOL_BLOCK_SIZE = 256;

	RenderFrame(Device &device, RenderTarget &&render_target, uint16_t command_pool_count = 1);

	void reset();

	Device &get_device()
	{
		return device;
	}

	/**
	 * @brief Retrieve the frame's command pool(s)
	 * @param queue The queue command buffers will be submitted on
	 * @param reset_mode Indicate how the command buffers will be reset after execution,
	 *        may trigger a pool re-creation to set necessary flags
	 * @return The frame's command pool(s)
	 */
	std::vector<std::unique_ptr<CommandPool>> &get_command_pools(const Queue &queue, CommandBuffer::ResetMode reset_mode);

	FencePool &get_fence_pool();

	SemaphorePool &get_semaphore_pool();

	/**
	 * @brief Called when the swapchain changes
	 * @param render_target A new render target with updated images
	 */
	void update_render_target(RenderTarget &&render_target);

	RenderTarget &get_render_target();

	/**
	 * @param usage Usage of the buffer
	 * @param size Amount of memory required
	 * @return The requested allocation, it may be empty
	 */
	BufferAllocation allocate_buffer(VkBufferUsageFlags usage, VkDeviceSize size);

  private:
	Device &device;

	/// Commands pools associated to the frame
	std::map<uint32_t, std::vector<std::unique_ptr<CommandPool>>> command_pools;

	FencePool fence_pool;

	SemaphorePool semaphore_pool;

	uint16_t command_pool_count;

	RenderTarget swapchain_render_target;

	std::map<VkBufferUsageFlags, std::pair<BufferPool, BufferBlock *>> buffer_pools;
};
}        // namespace vkb
