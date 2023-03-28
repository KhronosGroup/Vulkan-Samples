/* Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "rendering/render_frame.h"
#include <core/hpp_command_buffer.h>
#include <core/hpp_queue.h>
#include <hpp_buffer_pool.h>
#include <rendering/hpp_render_target.h>

namespace vkb
{
namespace rendering
{
/**
 * @brief facade class around vkb::RenderFrame, providing a vulkan.hpp-based interface
 *
 * See vkb::RenderFrame for documentation
 */
class HPPRenderFrame : private vkb::RenderFrame
{
  public:
	using vkb::RenderFrame::reset;

	HPPRenderFrame(vkb::core::HPPDevice &device, std::unique_ptr<HPPRenderTarget> &&render_target, size_t thread_count = 1) :
	    RenderFrame(reinterpret_cast<vkb::Device &>(device),
	                std::unique_ptr<vkb::RenderTarget>(reinterpret_cast<vkb::RenderTarget *>(render_target.release())),
	                thread_count)
	{}

	vkb::HPPBufferAllocation allocate_buffer(vk::BufferUsageFlags usage, vk::DeviceSize size, size_t thread_index = 0)
	{
		vkb::BufferAllocation allocation = vkb::RenderFrame::allocate_buffer(static_cast<VkBufferUsageFlags>(usage), static_cast<VkDeviceSize>(size), thread_index);
		return std::move(*reinterpret_cast<vkb::HPPBufferAllocation *>(&allocation));
	}

	HPPRenderTarget &get_render_target()
	{
		return reinterpret_cast<HPPRenderTarget &>(vkb::RenderFrame::get_render_target());
	}

	void release_owned_semaphore(vk::Semaphore semaphore)
	{
		vkb::RenderFrame::release_owned_semaphore(static_cast<VkSemaphore>(semaphore));
	}

	vkb::core::HPPCommandBuffer &request_command_buffer(const vkb::core::HPPQueue             &queue,
	                                                    vkb::core::HPPCommandBuffer::ResetMode reset_mode   = vkb::core::HPPCommandBuffer::ResetMode::ResetPool,
	                                                    vk::CommandBufferLevel                 level        = vk::CommandBufferLevel::ePrimary,
	                                                    size_t                                 thread_index = 0)
	{
		return reinterpret_cast<vkb::core::HPPCommandBuffer &>(
		    vkb::RenderFrame::request_command_buffer(reinterpret_cast<vkb::Queue const &>(queue),
		                                             static_cast<vkb::CommandBuffer::ResetMode>(reset_mode),
		                                             static_cast<VkCommandBufferLevel>(level),
		                                             thread_index));
	}

	vk::DescriptorSet request_descriptor_set(const vkb::core::HPPDescriptorSetLayout    &descriptor_set_layout,
	                                         const BindingMap<vk::DescriptorBufferInfo> &buffer_infos,
	                                         const BindingMap<vk::DescriptorImageInfo>  &image_infos,
	                                         bool                                        update_after_bind,
	                                         size_t                                      thread_index = 0)
	{
		return static_cast<vk::DescriptorSet>(vkb::RenderFrame::request_descriptor_set(reinterpret_cast<vkb::DescriptorSetLayout const &>(descriptor_set_layout),
		                                                                               reinterpret_cast<BindingMap<VkDescriptorBufferInfo> const &>(buffer_infos),
		                                                                               reinterpret_cast<BindingMap<VkDescriptorImageInfo> const &>(image_infos),
		                                                                               update_after_bind,
		                                                                               thread_index));
	}

	vk::Fence request_fence()
	{
		return static_cast<vk::Fence>(vkb::RenderFrame::request_fence());
	}

	vk::Semaphore request_semaphore()
	{
		return static_cast<vk::Semaphore>(vkb::RenderFrame::request_semaphore());
	}

	vk::Semaphore request_semaphore_with_ownership()
	{
		return static_cast<vk::Semaphore>(vkb::RenderFrame::request_semaphore_with_ownership());
	}

	void update_render_target(std::unique_ptr<HPPRenderTarget> &&render_target)
	{
		vkb::RenderFrame::update_render_target(std::unique_ptr<vkb::RenderTarget>(reinterpret_cast<vkb::RenderTarget *>(render_target.release())));
	}
};
}        // namespace rendering
}        // namespace vkb
