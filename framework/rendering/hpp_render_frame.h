/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_buffer_pool.h"
#include <core/hpp_device.h>
#include <hpp_semaphore_pool.h>
#include <vulkan/vulkan_hash.hpp>

namespace vkb
{
namespace rendering
{
enum class BufferAllocationStrategy
{
	OneAllocationPerBuffer,
	MultipleAllocationsPerBuffer
};

enum class DescriptorManagementStrategy
{
	StoreInCache,
	CreateDirectly
};

/**
 * @brief HPPRenderFrame is a transcoded version of vkb::RenderFrame from vulkan to vulkan-hpp.
 *
 * See vkb::HPPRenderFrame for documentation
 */
/**
 * @brief HPPRenderFrame is a container for per-frame data, including BufferPool objects,
 * synchronization primitives (semaphores, fences) and the swapchain RenderTarget.
 *
 * When creating a RenderTarget, we need to provide images that will be used as attachments
 * within a RenderPass. The HPPRenderFrame is responsible for creating a RenderTarget using
 * RenderTarget::CreateFunc. A custom RenderTarget::CreateFunc can be provided if a different
 * render target is required.
 *
 * A HPPRenderFrame cannot be destroyed individually since frames are managed by the RenderContext,
 * the whole context must be destroyed. This is because each HPPRenderFrame holds Vulkan objects
 * such as the swapchain image.
 */
class HPPRenderFrame
{
  public:
	HPPRenderFrame(vkb::core::HPPDevice &device, std::unique_ptr<vkb::rendering::HPPRenderTarget> &&render_target, size_t thread_count = 1);

	HPPRenderFrame(const HPPRenderFrame &)            = delete;
	HPPRenderFrame(HPPRenderFrame &&)                 = delete;
	HPPRenderFrame &operator=(const HPPRenderFrame &) = delete;
	HPPRenderFrame &operator=(HPPRenderFrame &&)      = delete;

	void                                   clear_descriptors();
	vkb::core::HPPDevice                  &get_device();
	const vkb::HPPFencePool               &get_fence_pool() const;
	vkb::rendering::HPPRenderTarget       &get_render_target();
	vkb::rendering::HPPRenderTarget const &get_render_target() const;
	const vkb::HPPSemaphorePool           &get_semaphore_pool() const;
	void                                   release_owned_semaphore(vk::Semaphore semaphore);
	vk::DescriptorSet                      request_descriptor_set(const vkb::core::HPPDescriptorSetLayout    &descriptor_set_layout,
	                                                              const BindingMap<vk::DescriptorBufferInfo> &buffer_infos,
	                                                              const BindingMap<vk::DescriptorImageInfo>  &image_infos,
	                                                              bool                                        update_after_bind,
	                                                              size_t                                      thread_index = 0);
	vk::Fence                              request_fence();
	vk::Semaphore                          request_semaphore();
	vk::Semaphore                          request_semaphore_with_ownership();
	void                                   reset();

	/**
	 * @param usage Usage of the buffer
	 * @param size Amount of memory required
	 * @param thread_index Index of the buffer pool to be used by the current thread
	 * @return The requested allocation, it may be empty
	 */
	vkb::HPPBufferAllocation allocate_buffer(vk::BufferUsageFlags usage, vk::DeviceSize size, size_t thread_index = 0);

	/**
	 * @brief Requests a command buffer to the command pool of the active frame
	 *        A frame should be active at the moment of requesting it
	 * @param queue The queue command buffers will be submitted on
	 * @param reset_mode Indicate how the command buffer will be used, may trigger a
	 *        pool re-creation to set necessary flags
	 * @param level Command buffer level, either primary or secondary
	 * @param thread_index Selects the thread's command pool used to manage the buffer
	 * @return A command buffer related to the current active frame
	 */
	vkb::core::HPPCommandBuffer &request_command_buffer(const vkb::core::HPPQueue             &queue,
	                                                    vkb::core::HPPCommandBuffer::ResetMode reset_mode   = vkb::core::HPPCommandBuffer::ResetMode::ResetPool,
	                                                    vk::CommandBufferLevel                 level        = vk::CommandBufferLevel::ePrimary,
	                                                    size_t                                 thread_index = 0);

	/**
	 * @brief Sets a new buffer allocation strategy
	 * @param new_strategy The new buffer allocation strategy
	 */
	void set_buffer_allocation_strategy(BufferAllocationStrategy new_strategy);

	/**
	 * @brief Sets a new descriptor set management strategy
	 * @param new_strategy The new descriptor set management strategy
	 */
	void set_descriptor_management_strategy(DescriptorManagementStrategy new_strategy);

	/**
	 * @brief Called when the swapchain changes
	 * @param render_target A new render target with updated images
	 */
	void update_render_target(std::unique_ptr<vkb::rendering::HPPRenderTarget> &&render_target);

	/**
	 * @brief Updates all the descriptor sets in the current frame at a specific thread index
	 */
	void update_descriptor_sets(size_t thread_index = 0);

  private:
	/**
	 * @brief Retrieve the frame's command pool(s)
	 * @param queue The queue command buffers will be submitted on
	 * @param reset_mode Indicate how the command buffers will be reset after execution,
	 *        may trigger a pool re-creation to set necessary flags
	 * @return The frame's command pool(s)
	 */
	std::vector<std::unique_ptr<vkb::core::HPPCommandPool>> &get_command_pools(const vkb::core::HPPQueue             &queue,
	                                                                           vkb::core::HPPCommandBuffer::ResetMode reset_mode);

	static std::vector<uint32_t> collect_bindings_to_update(const vkb::core::HPPDescriptorSetLayout    &descriptor_set_layout,
	                                                        const BindingMap<vk::DescriptorBufferInfo> &buffer_infos,
	                                                        const BindingMap<vk::DescriptorImageInfo>  &image_infos);

  private:
	// A map of the supported usages to a multiplier for the BUFFER_POOL_BLOCK_SIZE
	const std::unordered_map<vk::BufferUsageFlags, uint32_t> supported_usage_map = {
	    {vk::BufferUsageFlagBits::eUniformBuffer, 1},
	    {vk::BufferUsageFlagBits::eStorageBuffer, 2},        // x2 the size of BUFFER_POOL_BLOCK_SIZE since SSBOs are normally much larger than other types of buffers
	    {vk::BufferUsageFlagBits::eVertexBuffer, 1},
	    {vk::BufferUsageFlagBits::eIndexBuffer, 1}};

	vkb::core::HPPDevice &device;

	/// Commands pools associated to the frame
	std::map<uint32_t, std::vector<std::unique_ptr<vkb::core::HPPCommandPool>>> command_pools;

	/// Descriptor pools for the frame
	std::vector<std::unique_ptr<std::unordered_map<std::size_t, vkb::core::HPPDescriptorPool>>> descriptor_pools;

	/// Descriptor sets for the frame
	std::vector<std::unique_ptr<std::unordered_map<std::size_t, vkb::core::HPPDescriptorSet>>> descriptor_sets;

	vkb::HPPFencePool fence_pool;

	vkb::HPPSemaphorePool semaphore_pool;

	size_t thread_count;

	std::unique_ptr<vkb::rendering::HPPRenderTarget> swapchain_render_target;

	BufferAllocationStrategy buffer_allocation_strategy{BufferAllocationStrategy::MultipleAllocationsPerBuffer};

	DescriptorManagementStrategy descriptor_management_strategy{DescriptorManagementStrategy::StoreInCache};

	std::map<vk::BufferUsageFlags, std::vector<std::pair<vkb::HPPBufferPool, vkb::HPPBufferBlock *>>> buffer_pools;
};
}        // namespace rendering
}        // namespace vkb
