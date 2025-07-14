/* Copyright (c) 2019-2025, Arm Limited and Contributors
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
#include "common/hpp_resource_caching.h"
#include "core/command_pool.h"
#include "core/hpp_queue.h"
#include "core/queue.h"
#include "hpp_semaphore_pool.h"

namespace vkb
{
namespace rendering
{
enum BufferAllocationStrategy
{
	OneAllocationPerBuffer,
	MultipleAllocationsPerBuffer
};

enum DescriptorManagementStrategy
{
	StoreInCache,
	CreateDirectly
};

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
template <vkb::BindingType bindingType>
class RenderFrame
{
  public:
	using BufferUsageFlagsType     = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::BufferUsageFlags, VkBufferUsageFlags>::type;
	using CommandBufferLevelType   = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandBufferLevel, VkCommandBufferLevel>::type;
	using DescriptorBufferInfoType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DescriptorBufferInfo, VkDescriptorBufferInfo>::type;
	using DescriptorImageInfoType  = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DescriptorImageInfo, VkDescriptorImageInfo>::type;
	using DescriptorSetType        = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DescriptorSet, VkDescriptorSet>::type;
	using DeviceSizeType           = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DeviceSize, VkDeviceSize>::type;
	using FenceType                = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Fence, VkFence>::type;
	using SemaphoreType            = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Semaphore, VkSemaphore>::type;

	using DescriptorSetLayoutType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPDescriptorSetLayout, vkb::DescriptorSetLayout>::type;
	using FencePoolType           = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::HPPFencePool, vkb::FencePool>::type;
	using QueueType               = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPQueue, vkb::Queue>::type;
	using RenderTargetType        = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPRenderTarget, vkb::RenderTarget>::type;
	using SemaphorePoolType       = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::HPPSemaphorePool, vkb::SemaphorePool>::type;

  public:
	RenderFrame(vkb::core::Device<bindingType> &device, std::unique_ptr<RenderTargetType> &&render_target, size_t thread_count = 1);
	RenderFrame(RenderFrame<bindingType> const &)            = delete;
	RenderFrame(RenderFrame<bindingType> &&)                 = default;
	RenderFrame &operator=(RenderFrame<bindingType> const &) = delete;
	RenderFrame &operator=(RenderFrame<bindingType> &&)      = default;

	/**
	 * @param usage Usage of the buffer
	 * @param size Amount of memory required
	 * @param thread_index Index of the buffer pool to be used by the current thread
	 * @return The requested allocation, it may be empty
	 */
	vkb::BufferAllocation<bindingType> allocate_buffer(BufferUsageFlagsType usage, DeviceSizeType size, size_t thread_index = 0);

	void clear_descriptors();

	/**
	 * @brief Get the command pool of the active frame
	 *        A frame should be active at the moment of requesting it
	 * @param queue The queue command buffers will be submitted on
	 * @param reset_mode Indicate how the command buffer will be used, may trigger a pool re-creation to set necessary flags
	 * @param thread_index Selects the thread's command pool used to manage the buffer
	 * @return The command pool related to the current active frame
	 */
	vkb::core::CommandPool<bindingType> &get_command_pool(
	    QueueType const &queue, vkb::CommandBufferResetMode reset_mode = vkb::CommandBufferResetMode::ResetPool, size_t thread_index = 0);

	vkb::core::Device<bindingType> &get_device();
	FencePoolType                  &get_fence_pool();
	FencePoolType const            &get_fence_pool() const;
	RenderTargetType               &get_render_target();
	RenderTargetType const         &get_render_target() const;
	SemaphorePoolType              &get_semaphore_pool();
	SemaphorePoolType const        &get_semaphore_pool() const;
	DescriptorSetType               request_descriptor_set(DescriptorSetLayoutType const              &descriptor_set_layout,
	                                                       BindingMap<DescriptorBufferInfoType> const &buffer_infos,
	                                                       BindingMap<DescriptorImageInfoType> const  &image_infos,
	                                                       bool                                        update_after_bind,
	                                                       size_t                                      thread_index = 0);
	void                            reset();

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
	 * @brief Updates all the descriptor sets in the current frame at a specific thread index
	 */
	void update_descriptor_sets(size_t thread_index = 0);

	/**
	 * @brief Called when the swapchain changes
	 * @param render_target A new render target with updated images
	 */
	void update_render_target(std::unique_ptr<RenderTargetType> &&render_target);

  private:
	vkb::BufferAllocationCpp   allocate_buffer_impl(vk::BufferUsageFlags usage, vk::DeviceSize size, size_t thread_index);
	vkb::core::CommandPoolCpp &get_command_pool_impl(vkb::core::HPPQueue const &queue, vkb::CommandBufferResetMode reset_mode, size_t thread_index);

	/**
	 * @brief Retrieve the frame's command pool(s)
	 * @param queue The queue command buffers will be submitted on
	 * @param reset_mode Indicate how the command buffers will be reset after execution,
	 *        may trigger a pool re-creation to set necessary flags
	 * @return The frame's command pool(s)
	 */
	std::vector<vkb::core::CommandPoolCpp> &get_command_pools(const vkb::core::HPPQueue &queue, vkb::CommandBufferResetMode reset_mode);

	vk::DescriptorSet request_descriptor_set_impl(vkb::core::HPPDescriptorSetLayout const    &descriptor_set_layout,
	                                              BindingMap<vk::DescriptorBufferInfo> const &buffer_infos,
	                                              BindingMap<vk::DescriptorImageInfo> const  &image_infos,
	                                              bool                                        update_after_bind,
	                                              size_t                                      thread_index = 0);

  private:
	vkb::core::DeviceCpp                                                                             &device;
	std::map<vk::BufferUsageFlags, std::vector<std::pair<vkb::BufferPoolCpp, vkb::BufferBlockCpp *>>> buffer_pools;
	std::map<uint32_t, std::vector<vkb::core::CommandPoolCpp>>                                        command_pools;           // Commands pools per queue family index
	std::vector<std::unordered_map<std::size_t, vkb::core::HPPDescriptorPool>>                        descriptor_pools;        // Descriptor pools per thread
	std::vector<std::unordered_map<std::size_t, vkb::core::HPPDescriptorSet>>                         descriptor_sets;         // Descriptor sets per thread
	vkb::HPPFencePool                                                                                 fence_pool;
	vkb::HPPSemaphorePool                                                                             semaphore_pool;
	std::unique_ptr<vkb::rendering::HPPRenderTarget>                                                  swapchain_render_target;
	size_t                                                                                            thread_count;
	BufferAllocationStrategy                                                                          buffer_allocation_strategy     = BufferAllocationStrategy::MultipleAllocationsPerBuffer;
	DescriptorManagementStrategy                                                                      descriptor_management_strategy = DescriptorManagementStrategy::StoreInCache;
};

using RenderFrameC   = RenderFrame<vkb::BindingType::C>;
using RenderFrameCpp = RenderFrame<vkb::BindingType::Cpp>;

template <vkb::BindingType bindingType>
inline RenderFrame<bindingType>::RenderFrame(vkb::core::Device<bindingType>     &device_,
                                             std::unique_ptr<RenderTargetType> &&render_target,
                                             size_t                              thread_count) :
    device(reinterpret_cast<vkb::core::DeviceCpp &>(device_)), fence_pool{device}, semaphore_pool{device}, thread_count{thread_count}, descriptor_pools(thread_count), descriptor_sets(thread_count)
{
	static constexpr uint32_t BUFFER_POOL_BLOCK_SIZE = 256;        // Block size of a buffer pool in kilobytes

	// A map of the supported usages to a multiplier for the BUFFER_POOL_BLOCK_SIZE
	static const std::unordered_map<vk::BufferUsageFlags, uint32_t> supported_usage_map = {
	    {vk::BufferUsageFlagBits::eUniformBuffer, 1},
	    {vk::BufferUsageFlagBits::eStorageBuffer, 2},        // x2 the size of BUFFER_POOL_BLOCK_SIZE since SSBOs are normally much larger than other types of buffers
	    {vk::BufferUsageFlagBits::eVertexBuffer, 1},
	    {vk::BufferUsageFlagBits::eIndexBuffer, 1}};

	update_render_target(std::move(render_target));
	for (auto &usage_it : supported_usage_map)
	{
		auto [buffer_pools_it, inserted] = buffer_pools.emplace(usage_it.first, std::vector<std::pair<vkb::BufferPoolCpp, vkb::BufferBlockCpp *>>{});
		if (!inserted)
		{
			throw std::runtime_error("Failed to insert buffer pool");
		}

		for (size_t i = 0; i < thread_count; ++i)
		{
			buffer_pools_it->second.push_back(
			    std::make_pair(vkb::BufferPoolCpp{device, BUFFER_POOL_BLOCK_SIZE * 1024 * usage_it.second, usage_it.first}, nullptr));
		}
	}
}

template <vkb::BindingType bindingType>
inline BufferAllocation<bindingType> RenderFrame<bindingType>::allocate_buffer(BufferUsageFlagsType usage, DeviceSizeType size, size_t thread_index)
{
	assert(thread_index < thread_count && "Thread index is out of bounds");

	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return allocate_buffer_impl(usage, size, thread_index);
	}
	else
	{
		vkb::BufferAllocationCpp buffer_allocation =
		    allocate_buffer_impl(static_cast<vk::BufferUsageFlags>(usage), static_cast<vk::DeviceSize>(size), thread_index);
		return *reinterpret_cast<vkb::BufferAllocationC *>(&buffer_allocation);
	}
}

template <vkb::BindingType bindingType>
inline vkb::BufferAllocationCpp RenderFrame<bindingType>::allocate_buffer_impl(vk::BufferUsageFlags usage, vk::DeviceSize size, size_t thread_index)
{
	// Find a pool for this usage
	auto buffer_pool_it = buffer_pools.find(usage);
	if (buffer_pool_it == buffer_pools.end())
	{
		LOGE("No buffer pool for buffer usage {} ", vk::to_string(usage));
		return vkb::BufferAllocationCpp{};
	}

	assert(thread_index < buffer_pool_it->second.size());
	auto &buffer_pool  = buffer_pool_it->second[thread_index].first;
	auto &buffer_block = buffer_pool_it->second[thread_index].second;

	bool want_minimal_block = (buffer_allocation_strategy == BufferAllocationStrategy::OneAllocationPerBuffer);

	if (want_minimal_block || !buffer_block || !buffer_block->can_allocate(size))
	{
		// If we are creating a buffer for each allocation of there is no block associated with the pool or the current block is too small
		// for this allocation, request a new buffer block
		buffer_block = &buffer_pool.request_buffer_block(size, want_minimal_block);
	}

	return buffer_block->allocate(to_u32(size));
}

template <vkb::BindingType bindingType>
inline void RenderFrame<bindingType>::clear_descriptors()
{
	for (auto &desc_sets_per_thread : descriptor_sets)
	{
		desc_sets_per_thread.clear();
	}

	for (auto &desc_pools_per_thread : descriptor_pools)
	{
		for (auto &desc_pool : desc_pools_per_thread)
		{
			desc_pool.second.reset();
		}
	}
}

template <vkb::BindingType bindingType>
inline std::vector<vkb::core::CommandPoolCpp> &RenderFrame<bindingType>::get_command_pools(const vkb::core::HPPQueue  &queue,
                                                                                           vkb::CommandBufferResetMode reset_mode)
{
	auto command_pool_it = command_pools.find(queue.get_family_index());

	if (command_pool_it != command_pools.end())
	{
		assert(!command_pool_it->second.empty());
		if (command_pool_it->second[0].get_reset_mode() != reset_mode)
		{
			device.get_handle().waitIdle();

			// Delete pools
			command_pools.erase(command_pool_it);
		}
		else
		{
			return command_pool_it->second;
		}
	}

	bool inserted                       = false;
	std::tie(command_pool_it, inserted) = command_pools.emplace(queue.get_family_index(), std::vector<vkb::core::CommandPoolCpp>{});
	if (!inserted)
	{
		throw std::runtime_error("Failed to insert command pool");
	}

	for (size_t i = 0; i < thread_count; i++)
	{
		command_pool_it->second.emplace_back(device, queue.get_family_index(), reinterpret_cast<vkb::rendering::RenderFrameCpp *>(this), i, reset_mode);
	}

	return command_pool_it->second;
}

template <vkb::BindingType bindingType>
inline vkb::core::CommandPool<bindingType> &
    RenderFrame<bindingType>::get_command_pool(QueueType const &queue, vkb::CommandBufferResetMode reset_mode, size_t thread_index)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return get_command_pool_impl(queue, reset_mode, thread_index);
	}
	else
	{
		return reinterpret_cast<vkb::core::CommandPoolC &>(
		    get_command_pool_impl(reinterpret_cast<vkb::core::HPPQueue const &>(queue), reset_mode, thread_index));
	}
}

template <vkb::BindingType bindingType>
inline vkb::core::CommandPoolCpp &
    RenderFrame<bindingType>::get_command_pool_impl(vkb::core::HPPQueue const &queue, vkb::CommandBufferResetMode reset_mode, size_t thread_index)
{
	assert(thread_index < thread_count && "Thread index is out of bounds");

	auto &command_pools = get_command_pools(queue, reset_mode);

	auto command_pool_it = std::ranges::find_if(
	    command_pools, [&thread_index](vkb::core::CommandPoolCpp &cmd_pool) { return cmd_pool.get_thread_index() == thread_index; });
	assert(command_pool_it != command_pools.end());

	return *command_pool_it;
}

template <vkb::BindingType bindingType>
inline typename vkb::core::Device<bindingType> &RenderFrame<bindingType>::get_device()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return device;
	}
	else
	{
		return reinterpret_cast<vkb::core::DeviceC &>(device);
	}
}

template <vkb::BindingType bindingType>
inline typename RenderFrame<bindingType>::FencePoolType &RenderFrame<bindingType>::get_fence_pool()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return fence_pool;
	}
	else
	{
		return reinterpret_cast<vkb::FencePool &>(fence_pool);
	}
}

template <vkb::BindingType bindingType>
inline typename RenderFrame<bindingType>::FencePoolType const &RenderFrame<bindingType>::get_fence_pool() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return fence_pool;
	}
	else
	{
		return reinterpret_cast<vkb::FencePool const &>(fence_pool);
	}
}

template <vkb::BindingType bindingType>
inline typename RenderFrame<bindingType>::RenderTargetType &RenderFrame<bindingType>::get_render_target()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return *swapchain_render_target;
	}
	else
	{
		return reinterpret_cast<vkb::RenderTarget &>(*swapchain_render_target);
	}
}

template <vkb::BindingType bindingType>
inline typename RenderFrame<bindingType>::RenderTargetType const &RenderFrame<bindingType>::get_render_target() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return *swapchain_render_target;
	}
	else
	{
		return reinterpret_cast<vkb::RenderTarget const &>(*swapchain_render_target);
	}
}

template <vkb::BindingType bindingType>
inline typename RenderFrame<bindingType>::SemaphorePoolType &RenderFrame<bindingType>::get_semaphore_pool()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return semaphore_pool;
	}
	else
	{
		return reinterpret_cast<vkb::SemaphorePool &>(semaphore_pool);
	}
}

template <vkb::BindingType bindingType>
inline typename RenderFrame<bindingType>::SemaphorePoolType const &RenderFrame<bindingType>::get_semaphore_pool() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return semaphore_pool;
	}
	else
	{
		return reinterpret_cast<vkb::SemaphorePool const &>(semaphore_pool);
	}
}

template <vkb::BindingType bindingType>
inline typename RenderFrame<bindingType>::DescriptorSetType RenderFrame<bindingType>::request_descriptor_set(DescriptorSetLayoutType const              &descriptor_set_layout,
                                                                                                             BindingMap<DescriptorBufferInfoType> const &buffer_infos,
                                                                                                             BindingMap<DescriptorImageInfoType> const  &image_infos,
                                                                                                             bool                                        update_after_bind,
                                                                                                             size_t                                      thread_index)
{
	assert(thread_index < thread_count && "Thread index is out of bounds");
	assert(thread_index < descriptor_pools.size());

	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return request_descriptor_set_impl(descriptor_set_layout, buffer_infos, image_infos, update_after_bind, thread_index);
	}
	else
	{
		return static_cast<VkDescriptorSet>(request_descriptor_set_impl(reinterpret_cast<vkb::core::HPPDescriptorSetLayout const &>(descriptor_set_layout),
		                                                                reinterpret_cast<BindingMap<vk::DescriptorBufferInfo> const &>(buffer_infos),
		                                                                reinterpret_cast<BindingMap<vk::DescriptorImageInfo> const &>(image_infos),
		                                                                update_after_bind,
		                                                                thread_index));
	}
}

template <vkb::BindingType bindingType>
inline vk::DescriptorSet RenderFrame<bindingType>::request_descriptor_set_impl(vkb::core::HPPDescriptorSetLayout const    &descriptor_set_layout,
                                                                               BindingMap<vk::DescriptorBufferInfo> const &buffer_infos,
                                                                               BindingMap<vk::DescriptorImageInfo> const  &image_infos,
                                                                               bool                                        update_after_bind,
                                                                               size_t                                      thread_index)
{
	auto &descriptor_pool = vkb::common::request_resource(device, nullptr, descriptor_pools[thread_index], descriptor_set_layout);
	if (descriptor_management_strategy == DescriptorManagementStrategy::StoreInCache)
	{
		// The bindings we want to update before binding, if empty we update all bindings
		std::set<uint32_t> bindings_to_update;
		// If update after bind is enabled, we store the binding index of each binding that need to be updated before being bound
		if (update_after_bind)
		{
			auto aggregate_binding_to_update = [&bindings_to_update, &descriptor_set_layout](const auto &infos_map) {
				for (const auto &[binding_index, ignored] : infos_map)
				{
					if (!(descriptor_set_layout.get_layout_binding_flag(binding_index) & vk::DescriptorBindingFlagBits::eUpdateAfterBind))
					{
						bindings_to_update.insert(binding_index);
					}
				}
			};
			aggregate_binding_to_update(buffer_infos);
			aggregate_binding_to_update(image_infos);
		}

		// Request a descriptor set from the render frame, and write the buffer infos and image infos of all the specified bindings
		assert(thread_index < descriptor_sets.size());
		auto &descriptor_set =
		    vkb::common::request_resource(device, nullptr, descriptor_sets[thread_index], descriptor_set_layout, descriptor_pool, buffer_infos, image_infos);
		descriptor_set.update({bindings_to_update.begin(), bindings_to_update.end()});
		return descriptor_set.get_handle();
	}
	else
	{
		// Request a descriptor pool, allocate a descriptor set, write buffer and image data to it
		vkb::core::HPPDescriptorSet descriptor_set{device, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos};
		descriptor_set.apply_writes();
		return descriptor_set.get_handle();
	}
}

template <vkb::BindingType bindingType>
inline void RenderFrame<bindingType>::reset()
{
	VK_CHECK(fence_pool.wait());

	fence_pool.reset();

	for (auto &command_pools_per_queue : command_pools)
	{
		for (auto &command_pool : command_pools_per_queue.second)
		{
			command_pool.reset_pool();
		}
	}

	for (auto &buffer_pools_per_usage : buffer_pools)
	{
		for (auto &buffer_pool : buffer_pools_per_usage.second)
		{
			buffer_pool.first.reset();
			buffer_pool.second = nullptr;
		}
	}

	semaphore_pool.reset();

	if (descriptor_management_strategy == DescriptorManagementStrategy::CreateDirectly)
	{
		clear_descriptors();
	}
}

template <vkb::BindingType bindingType>
inline void RenderFrame<bindingType>::set_buffer_allocation_strategy(BufferAllocationStrategy new_strategy)
{
	buffer_allocation_strategy = new_strategy;
}

template <vkb::BindingType bindingType>
inline void RenderFrame<bindingType>::set_descriptor_management_strategy(DescriptorManagementStrategy new_strategy)
{
	descriptor_management_strategy = new_strategy;
}

template <vkb::BindingType bindingType>
inline void RenderFrame<bindingType>::update_descriptor_sets(size_t thread_index)
{
	assert(thread_index < descriptor_sets.size());
	auto &thread_descriptor_sets = descriptor_sets[thread_index];
	for (auto &descriptor_set_it : thread_descriptor_sets)
	{
		descriptor_set_it.second.update();
	}
}

template <vkb::BindingType bindingType>
inline void RenderFrame<bindingType>::update_render_target(std::unique_ptr<RenderTargetType> &&render_target)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		swapchain_render_target = std::move(render_target);
	}
	else
	{
		swapchain_render_target.reset(reinterpret_cast<vkb::rendering::HPPRenderTarget *>(render_target.release()));
	}
}
}        // namespace rendering
}        // namespace vkb