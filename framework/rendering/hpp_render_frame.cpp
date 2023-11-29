/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_render_frame.h"
#include <common/hpp_resource_caching.h>

constexpr uint32_t BUFFER_POOL_BLOCK_SIZE = 256;

namespace vkb
{
namespace rendering
{
HPPRenderFrame::HPPRenderFrame(vkb::core::HPPDevice &device, std::unique_ptr<vkb::rendering::HPPRenderTarget> &&render_target, size_t thread_count) :
    device{device},
    fence_pool{device},
    semaphore_pool{device},
    swapchain_render_target{std::move(render_target)},
    thread_count{thread_count}
{
	for (auto &usage_it : supported_usage_map)
	{
		auto [buffer_pools_it, inserted] = buffer_pools.emplace(usage_it.first, std::vector<std::pair<vkb::HPPBufferPool, vkb::HPPBufferBlock *>>{});
		if (!inserted)
		{
			throw std::runtime_error("Failed to insert buffer pool");
		}

		for (size_t i = 0; i < thread_count; ++i)
		{
			buffer_pools_it->second.push_back(std::make_pair(vkb::HPPBufferPool{device, BUFFER_POOL_BLOCK_SIZE * 1024 * usage_it.second, usage_it.first}, nullptr));
		}
	}
}

vkb::HPPBufferAllocation HPPRenderFrame::allocate_buffer(const vk::BufferUsageFlags usage, const vk::DeviceSize size, size_t thread_index)
{
	assert(thread_index < thread_count && "Thread index is out of bounds");

	// Find a pool for this usage
	auto buffer_pool_it = buffer_pools.find(usage);
	if (buffer_pool_it == buffer_pools.end())
	{
		LOGE("No buffer pool for buffer usage " + vk::to_string(usage));
		return vkb::HPPBufferAllocation{};
	}

	assert(thread_index < buffer_pool_it->second.size());
	auto &buffer_pool  = buffer_pool_it->second[thread_index].first;
	auto &buffer_block = buffer_pool_it->second[thread_index].second;

	bool want_minimal_block = buffer_allocation_strategy == BufferAllocationStrategy::OneAllocationPerBuffer;

	if (want_minimal_block || !buffer_block || !buffer_block->can_allocate(size))
	{
		// If we are creating a buffer for each allocation of there is no block associated with the pool or the current block is too small
		// for this allocation, request a new buffer block
		buffer_block = &buffer_pool.request_buffer_block(size, want_minimal_block);
	}

	return buffer_block->allocate(to_u32(size));
}

void HPPRenderFrame::clear_descriptors()
{
	for (auto &desc_sets_per_thread : descriptor_sets)
	{
		desc_sets_per_thread.second->clear();
	}

	for (auto &desc_pools_per_thread : descriptor_pools)
	{
		desc_pools_per_thread.second->clear();
	}
}

std::vector<uint32_t> HPPRenderFrame::collect_bindings_to_update(const vkb::core::HPPDescriptorSetLayout    &descriptor_set_layout,
                                                                 const BindingMap<vk::DescriptorBufferInfo> &buffer_infos,
                                                                 const BindingMap<vk::DescriptorImageInfo>  &image_infos)
{
	std::set<uint32_t> bindings_to_update;

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

	return {bindings_to_update.begin(), bindings_to_update.end()};
}

std::vector<std::unique_ptr<vkb::core::HPPCommandPool>> &HPPRenderFrame::get_command_pools(const vkb::core::HPPQueue             &queue,
                                                                                           vkb::core::HPPCommandBuffer::ResetMode reset_mode)
{
	auto command_pool_it = command_pools.find(queue.get_family_index());

	if (command_pool_it != command_pools.end())
	{
		assert(!command_pool_it->second.empty());
		if (command_pool_it->second[0]->get_reset_mode() != reset_mode)
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
	std::tie(command_pool_it, inserted) = command_pools.emplace(queue.get_family_index(), std::vector<std::unique_ptr<vkb::core::HPPCommandPool>>{});
	if (!inserted)
	{
		throw std::runtime_error("Failed to insert command pool");
	}

	for (size_t i = 0; i < thread_count; i++)
	{
		command_pool_it->second.push_back(std::make_unique<vkb::core::HPPCommandPool>(device, queue.get_family_index(), this, i, reset_mode));
	}

	return command_pool_it->second;
}

vkb::core::HPPDevice &HPPRenderFrame::get_device()
{
	return device;
}

const vkb::HPPFencePool &HPPRenderFrame::get_fence_pool() const
{
	return fence_pool;
}

vkb::rendering::HPPRenderTarget &HPPRenderFrame::get_render_target()
{
	return *swapchain_render_target;
}

vkb::rendering::HPPRenderTarget const &HPPRenderFrame::get_render_target() const
{
	return *swapchain_render_target;
}

const vkb::HPPSemaphorePool &HPPRenderFrame::get_semaphore_pool() const
{
	return semaphore_pool;
}

void HPPRenderFrame::release_owned_semaphore(vk::Semaphore semaphore)
{
	semaphore_pool.release_owned_semaphore(semaphore);
}

vkb::core::HPPCommandBuffer &HPPRenderFrame::request_command_buffer(const vkb::core::HPPQueue             &queue,
                                                                    vkb::core::HPPCommandBuffer::ResetMode reset_mode,
                                                                    vk::CommandBufferLevel                 level,
                                                                    size_t                                 thread_index)
{
	assert(thread_index < thread_count && "Thread index is out of bounds");

	auto &command_pools = get_command_pools(queue, reset_mode);

	auto command_pool_it =
	    std::find_if(command_pools.begin(),
	                 command_pools.end(),
	                 [&thread_index](std::unique_ptr<vkb::core::HPPCommandPool> &cmd_pool) { return cmd_pool->get_thread_index() == thread_index; });
	assert(command_pool_it != command_pools.end());

	return (*command_pool_it)->request_command_buffer(level);
}

vk::DescriptorSet HPPRenderFrame::request_descriptor_set(const vkb::core::HPPDescriptorSetLayout    &descriptor_set_layout,
                                                         const BindingMap<vk::DescriptorBufferInfo> &buffer_infos,
                                                         const BindingMap<vk::DescriptorImageInfo>  &image_infos,
                                                         bool                                        update_after_bind,
                                                         size_t                                      thread_index)
{
	assert(thread_index < thread_count && "Thread index is out of bounds");

	assert(thread_index < descriptor_pools.size());

	// Cache descriptor pools against thread index
	auto pool_cache_it = descriptor_pools.find_or_insert(thread_index, []() -> std::unique_ptr<CacheMap<std::size_t, core::HPPDescriptorPool>> {
		return std::make_unique<CacheMap<std::size_t, core::HPPDescriptorPool>>();
	});

	auto descriptor_pool_it = pool_cache_it->second->find_or_insert(inline_hash_param(descriptor_set_layout), [&]() -> core::HPPDescriptorPool {
		return core::HPPDescriptorPool{device, descriptor_set_layout};
	});
	// Cache end;

	auto &descriptor_pool = descriptor_pool_it->second;
	if (descriptor_management_strategy == DescriptorManagementStrategy::StoreInCache)
	{
		// The bindings we want to update before binding, if empty we update all bindings
		std::vector<uint32_t> bindings_to_update;
		// If update after bind is enabled, we store the binding index of each binding that need to be updated before being bound
		if (update_after_bind)
		{
			bindings_to_update = collect_bindings_to_update(descriptor_set_layout, buffer_infos, image_infos);
		}

		// Request a descriptor set from the render frame, and write the buffer infos and image infos of all the specified bindings
		assert(thread_index < descriptor_sets.size());

		// Cache descriptor sets against thread index
		auto descriptor_set_cache_it = descriptor_sets.find_or_insert(thread_index, []() -> std::unique_ptr<CacheMap<std::size_t, core::HPPDescriptorSet>> {
			return std::make_unique<CacheMap<std::size_t, core::HPPDescriptorSet>>();
		});

		auto descriptor_set_it = descriptor_set_cache_it->second->find_or_insert(inline_hash_param(descriptor_set_layout, descriptor_pool, buffer_infos, image_infos), [&]() -> core::HPPDescriptorSet {
			return core::HPPDescriptorSet{device, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos};
		});
		// Cache end

		auto &descriptor_set = descriptor_set_it->second;
		descriptor_set.update(bindings_to_update);
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

vk::Fence HPPRenderFrame::request_fence()
{
	return fence_pool.request_fence();
}

vk::Semaphore HPPRenderFrame::request_semaphore()
{
	return semaphore_pool.request_semaphore();
}

vk::Semaphore HPPRenderFrame::request_semaphore_with_ownership()
{
	return semaphore_pool.request_semaphore_with_ownership();
}

void HPPRenderFrame::reset()
{
	VK_CHECK(fence_pool.wait());

	fence_pool.reset();

	for (auto &command_pools_per_queue : command_pools)
	{
		for (auto &command_pool : command_pools_per_queue.second)
		{
			command_pool->reset_pool();
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

void HPPRenderFrame::set_buffer_allocation_strategy(BufferAllocationStrategy new_strategy)
{
	buffer_allocation_strategy = new_strategy;
}

void HPPRenderFrame::set_descriptor_management_strategy(DescriptorManagementStrategy new_strategy)
{
	descriptor_management_strategy = new_strategy;
}

void HPPRenderFrame::update_descriptor_sets(size_t thread_index)
{
	assert(thread_index < descriptor_sets.size());

	auto cache_it = descriptor_sets.find(thread_index);
	if (cache_it == descriptor_sets.end())
	{
		return;
	}

	auto &cache = *cache_it->second;
	for (auto &descriptor_set_it : cache)
	{
		descriptor_set_it.second.update();
	}
}

void HPPRenderFrame::update_render_target(std::unique_ptr<vkb::rendering::HPPRenderTarget> &&render_target)
{
	swapchain_render_target = std::move(render_target);
}

}        // namespace rendering
}        // namespace vkb
