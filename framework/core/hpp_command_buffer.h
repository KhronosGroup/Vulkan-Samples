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

#include <core/command_buffer.h>

#include <common/hpp_vk_common.h>
#include <core/hpp_buffer.h>
#include <core/hpp_image_view.h>
#include <core/hpp_pipeline_layout.h>
#include <core/hpp_sampler.h>
#include <rendering/hpp_pipeline_state.h>

namespace vkb
{
namespace core
{
class HPPCommandPool;

/**
 * @brief facade class around vkb::CommandBuffer, providing a vulkan.hpp-based interface
 *
 * See vkb::CommandBuffer for documentation
 */
class HPPCommandBuffer : private vkb::CommandBuffer
{
  public:
	using vkb::CommandBuffer::draw_indexed;
	using vkb::CommandBuffer::end;
	using vkb::CommandBuffer::end_render_pass;
	using vkb::CommandBuffer::push_constants;

	enum class ResetMode
	{
		ResetPool,
		ResetIndividually,
		AlwaysAllocate,
	};

	HPPCommandBuffer(HPPCommandPool &command_pool, vk::CommandBufferLevel level) :
	    vkb::CommandBuffer(reinterpret_cast<vkb::CommandPool &>(command_pool), static_cast<VkCommandBufferLevel>(level))
	{}

	vk::Result begin(vk::CommandBufferUsageFlags flags, HPPCommandBuffer *primary_cmd_buf = nullptr)
	{
		return static_cast<vk::Result>(vkb::CommandBuffer::begin(static_cast<VkCommandBufferUsageFlags>(flags), reinterpret_cast<CommandBuffer *>(primary_cmd_buf)));
	}

	void bind_image(const vkb::core::HPPImageView &image_view, const vkb::core::HPPSampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element)
	{
		vkb::CommandBuffer::bind_image(
		    reinterpret_cast<vkb::core::ImageView const &>(image_view), reinterpret_cast<vkb::core::Sampler const &>(sampler), set, binding, array_element);
	}

	void bind_index_buffer(const vkb::core::HPPBuffer &buffer, vk::DeviceSize offset, vk::IndexType index_type)
	{
		vkb::CommandBuffer::bind_index_buffer(
		    reinterpret_cast<vkb::core::Buffer const &>(buffer), static_cast<VkDeviceSize>(offset), static_cast<VkIndexType>(index_type));
	}

	void bind_pipeline_layout(vkb::core::HPPPipelineLayout &pipeline_layout)
	{
		vkb::CommandBuffer::bind_pipeline_layout(reinterpret_cast<vkb::PipelineLayout &>(pipeline_layout));
	}

	void bind_vertex_buffers(uint32_t                                                               first_binding,
	                         const std::vector<std::reference_wrapper<const vkb::core::HPPBuffer>> &buffers,
	                         const std::vector<vk::DeviceSize>                                     &offsets)
	{
		vkb::CommandBuffer::bind_vertex_buffers(first_binding,
		                                        reinterpret_cast<std::vector<std::reference_wrapper<vkb::core::Buffer const>> const &>(buffers),
		                                        reinterpret_cast<std::vector<VkDeviceSize> const &>(offsets));
	}

	void copy_buffer_to_image(const vkb::core::HPPBuffer &buffer, const vkb::core::HPPImage &image, const std::vector<vk::BufferImageCopy> &regions)
	{
		vkb::CommandBuffer::copy_buffer_to_image(reinterpret_cast<vkb::core::Buffer const &>(buffer),
		                                         reinterpret_cast<vkb::core::Image const &>(image),
		                                         reinterpret_cast<std::vector<VkBufferImageCopy> const &>(regions));
	}

	vk::CommandBuffer get_handle() const
	{
		return static_cast<vk::CommandBuffer>(vkb::CommandBuffer::get_handle());
	}

	void image_memory_barrier(const vkb::core::HPPImageView &image_view, const vkb::common::HPPImageMemoryBarrier &memory_barrier) const
	{
		vkb::CommandBuffer::image_memory_barrier(reinterpret_cast<vkb::core::ImageView const &>(image_view),
		                                         reinterpret_cast<vkb::ImageMemoryBarrier const &>(memory_barrier));
	}

	void reset(ResetMode reset_mode)
	{
		VkResult result = vkb::CommandBuffer::reset(static_cast<CommandBuffer::ResetMode>(reset_mode));
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("vkCommandBufferReset failed with errorCode " + vk::to_string(static_cast<vk::Result>(result)));
		}
	}

	void set_color_blend_state(const vkb::rendering::HPPColorBlendState &state_info)
	{
		vkb::CommandBuffer::set_color_blend_state(reinterpret_cast<vkb::ColorBlendState const &>(state_info));
	}

	void set_depth_stencil_state(const vkb::rendering::HPPDepthStencilState &state_info)
	{
		vkb::CommandBuffer::set_depth_stencil_state(reinterpret_cast<vkb::DepthStencilState const &>(state_info));
	}

	void set_rasterization_state(const vkb::rendering::HPPRasterizationState &state_info)
	{
		vkb::CommandBuffer::set_rasterization_state(reinterpret_cast<vkb::RasterizationState const &>(state_info));
	}

	void set_scissor(uint32_t first_scissor, const std::vector<vk::Rect2D> &scissors)
	{
		vkb::CommandBuffer::set_scissor(first_scissor, reinterpret_cast<std::vector<VkRect2D> const &>(scissors));
	}

	void set_vertex_input_state(const vkb::rendering::HPPVertexInputState &state_info)
	{
		vkb::CommandBuffer::set_vertex_input_state(reinterpret_cast<vkb::VertexInputState const &>(state_info));
	}
};
}        // namespace core
}        // namespace vkb
