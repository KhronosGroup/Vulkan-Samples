/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include "render_frame.h"

#include "common/logging.h"
#include "common/utils.h"

namespace vkb
{
RenderFrame::RenderFrame(Device &device, std::unique_ptr<RenderTarget> &&render_target, size_t thread_count) :
    device{device},
    fence_pool{device},
    semaphore_pool{device},
    swapchain_render_target{std::move(render_target)},
    thread_count{thread_count}
{
	for (auto &usage_it : supported_usage_map)
	{
		std::vector<std::pair<BufferPool, BufferBlock *>> usage_buffer_pools;
		for (size_t i = 0; i < thread_count; ++i)
		{
			usage_buffer_pools.push_back(std::make_pair(BufferPool{device, BUFFER_POOL_BLOCK_SIZE * 1024 * usage_it.second, usage_it.first}, nullptr));
		}

		auto res_ins_it = buffer_pools.emplace(usage_it.first, std::move(usage_buffer_pools));

		if (!res_ins_it.second)
		{
			throw std::runtime_error("Failed to insert buffer pool");
		}
	}

	for (size_t i = 0; i < thread_count; ++i)
	{
		descriptor_pools.push_back(std::make_unique<std::unordered_map<std::size_t, DescriptorPool>>());
		descriptor_sets.push_back(std::make_unique<std::unordered_map<std::size_t, DescriptorSet>>());
	}
}

Device &RenderFrame::get_device()
{
	return device;
}

void RenderFrame::update_render_target(std::unique_ptr<RenderTarget> &&render_target)
{
	swapchain_render_target = std::move(render_target);
}

void RenderFrame::reset()
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

	if (descriptor_management_strategy == vkb::DescriptorManagementStrategy::CreateDirectly)
	{
		clear_descriptors();
	}
}

std::vector<std::unique_ptr<CommandPool>> &RenderFrame::get_command_pools(const Queue &queue, CommandBuffer::ResetMode reset_mode)
{
	auto command_pool_it = command_pools.find(queue.get_family_index());

	if (command_pool_it != command_pools.end())
	{
		assert(!command_pool_it->second.empty());
		if (command_pool_it->second[0]->get_reset_mode() != reset_mode)
		{
			device.wait_idle();

			// Delete pools
			command_pools.erase(command_pool_it);
		}
		else
		{
			return command_pool_it->second;
		}
	}

	std::vector<std::unique_ptr<CommandPool>> queue_command_pools;
	for (size_t i = 0; i < thread_count; i++)
	{
		queue_command_pools.push_back(std::make_unique<CommandPool>(device, queue.get_family_index(), this, i, reset_mode));
	}

	auto res_ins_it = command_pools.emplace(queue.get_family_index(), std::move(queue_command_pools));

	if (!res_ins_it.second)
	{
		throw std::runtime_error("Failed to insert command pool");
	}

	command_pool_it = res_ins_it.first;

	return command_pool_it->second;
}

std::vector<uint32_t> RenderFrame::collect_bindings_to_update(const DescriptorSetLayout &descriptor_set_layout, const BindingMap<VkDescriptorBufferInfo> &buffer_infos, const BindingMap<VkDescriptorImageInfo> &image_infos)
{
	std::vector<uint32_t> bindings_to_update;

	bindings_to_update.reserve(buffer_infos.size() + image_infos.size());
	auto aggregate_binding_to_update = [&bindings_to_update, &descriptor_set_layout](const auto &infos_map) {
		for (const auto &pair : infos_map)
		{
			uint32_t binding_index = pair.first;
			if (!(descriptor_set_layout.get_layout_binding_flag(binding_index) & VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT) &&
			    std::find(bindings_to_update.begin(), bindings_to_update.end(), binding_index) == bindings_to_update.end())
			{
				bindings_to_update.push_back(binding_index);
			}
		}
	};
	aggregate_binding_to_update(buffer_infos);
	aggregate_binding_to_update(image_infos);

	return bindings_to_update;
}

const FencePool &RenderFrame::get_fence_pool() const
{
	return fence_pool;
}

VkFence RenderFrame::request_fence()
{
	return fence_pool.request_fence();
}

const SemaphorePool &RenderFrame::get_semaphore_pool() const
{
	return semaphore_pool;
}

VkSemaphore RenderFrame::request_semaphore()
{
	return semaphore_pool.request_semaphore();
}

VkSemaphore RenderFrame::request_semaphore_with_ownership()
{
	return semaphore_pool.request_semaphore_with_ownership();
}

void RenderFrame::release_owned_semaphore(VkSemaphore semaphore)
{
	semaphore_pool.release_owned_semaphore(semaphore);
}

RenderTarget &RenderFrame::get_render_target()
{
	return *swapchain_render_target;
}

const RenderTarget &RenderFrame::get_render_target_const() const
{
	return *swapchain_render_target;
}

CommandBuffer &RenderFrame::request_command_buffer(const Queue &queue, CommandBuffer::ResetMode reset_mode, VkCommandBufferLevel level, size_t thread_index)
{
	assert(thread_index < thread_count && "Thread index is out of bounds");

	auto &command_pools = get_command_pools(queue, reset_mode);

	auto command_pool_it = std::find_if(command_pools.begin(), command_pools.end(), [&thread_index](std::unique_ptr<CommandPool> &cmd_pool) { return cmd_pool->get_thread_index() == thread_index; });

	return (*command_pool_it)->request_command_buffer(level);
}

VkDescriptorSet RenderFrame::request_descriptor_set(const DescriptorSetLayout &descriptor_set_layout, const BindingMap<VkDescriptorBufferInfo> &buffer_infos, const BindingMap<VkDescriptorImageInfo> &image_infos, bool update_after_bind, size_t thread_index)
{
	assert(thread_index < thread_count && "Thread index is out of bounds");

	assert(thread_index < descriptor_pools.size());
	auto &descriptor_pool = request_resource(device, nullptr, *descriptor_pools[thread_index], descriptor_set_layout);
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
		auto &descriptor_set = request_resource(device, nullptr, *descriptor_sets[thread_index], descriptor_set_layout, descriptor_pool, buffer_infos, image_infos);
		descriptor_set.update(bindings_to_update);
		return descriptor_set.get_handle();
	}
	else
	{
		// Request a descriptor pool, allocate a descriptor set, write buffer and image data to it
		DescriptorSet descriptor_set{device, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos};
		descriptor_set.apply_writes();
		return descriptor_set.get_handle();
	}
}

void RenderFrame::update_descriptor_sets(size_t thread_index)
{
	assert(thread_index < descriptor_sets.size());
	auto &thread_descriptor_sets = *descriptor_sets[thread_index];
	for (auto &descriptor_set_it : thread_descriptor_sets)
	{
		descriptor_set_it.second.update();
	}
}

void RenderFrame::clear_descriptors()
{
	for (auto &desc_sets_per_thread : descriptor_sets)
	{
		desc_sets_per_thread->clear();
	}

	for (auto &desc_pools_per_thread : descriptor_pools)
	{
		for (auto &desc_pool : *desc_pools_per_thread)
		{
			desc_pool.second.reset();
		}
	}
}

void RenderFrame::set_buffer_allocation_strategy(BufferAllocationStrategy new_strategy)
{
	buffer_allocation_strategy = new_strategy;
}

void RenderFrame::set_descriptor_management_strategy(DescriptorManagementStrategy new_strategy)
{
	descriptor_management_strategy = new_strategy;
}

BufferAllocation RenderFrame::allocate_buffer(const VkBufferUsageFlags usage, const VkDeviceSize size, size_t thread_index)
{
	assert(thread_index < thread_count && "Thread index is out of bounds");

	// Find a pool for this usage
	auto buffer_pool_it = buffer_pools.find(usage);
	if (buffer_pool_it == buffer_pools.end())
	{
		LOGE("No buffer pool for buffer usage {}", usage);
		return BufferAllocation{};
	}

	assert(thread_index < buffer_pool_it->second.size());
	auto &buffer_pool  = buffer_pool_it->second[thread_index].first;
	auto &buffer_block = buffer_pool_it->second[thread_index].second;

	bool want_minimal_block = buffer_allocation_strategy == BufferAllocationStrategy::OneAllocationPerBuffer;

	if (want_minimal_block || !buffer_block)
	{
		// If there is no block associated with the pool or we are creating a buffer for each allocation,
		// request a new buffer block
		buffer_block = &buffer_pool.request_buffer_block(to_u32(size), want_minimal_block);
	}

	auto data = buffer_block->allocate(to_u32(size));

	// Check if the buffer block can allocate the requested size
	if (data.empty())
	{
		buffer_block = &buffer_pool.request_buffer_block(to_u32(size), want_minimal_block);

		data = buffer_block->allocate(to_u32(size));
	}

	return data;
}
}        // namespace vkb
