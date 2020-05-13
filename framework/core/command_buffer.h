/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include <list>

#include "common/helpers.h"
#include "common/vk_common.h"
#include "core/buffer.h"
#include "core/image.h"
#include "core/image_view.h"
#include "core/query_pool.h"
#include "core/sampler.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_target.h"
#include "resource_binding_state.h"

namespace vkb
{
class CommandPool;
class DescriptorSet;
class Framebuffer;
class Pipeline;
class PipelineLayout;
class PipelineState;
class RenderTarget;
class Subpass;

/**
 * @brief Helper class to manage and record a command buffer, building and
 *        keeping track of pipeline state and resource bindings
 */
class CommandBuffer
{
  public:
	enum class ResetMode
	{
		ResetPool,
		ResetIndividually,
		AlwaysAllocate,
	};

	enum class State
	{
		Invalid,
		Initial,
		Recording,
		Executable,
	};

	/**
	 * @brief Helper structure used to track render pass state
	 */
	struct RenderPassBinding
	{
		const RenderPass *render_pass;

		const Framebuffer *framebuffer;
	};

	CommandBuffer(CommandPool &command_pool, VkCommandBufferLevel level);

	CommandBuffer(const CommandBuffer &) = delete;

	CommandBuffer(CommandBuffer &&other);

	~CommandBuffer();

	CommandBuffer &operator=(const CommandBuffer &) = delete;

	CommandBuffer &operator=(CommandBuffer &&) = delete;

	Device &get_device();

	const VkCommandBuffer &get_handle() const;

	bool is_recording() const;

	/**
	 * @brief Flushes the command buffer, pushing the new changes
	 * @param pipeline_bind_point The type of pipeline we want to flush
	 */
	void flush(VkPipelineBindPoint pipeline_bind_point);

	/**
	 * @brief Sets the command buffer so that it is ready for recording
	 *        If it is a secondary command buffer, a pointer to the
	 *        primary command buffer it inherits from must be provided
	 * @param flags Usage behavior for the command buffer
	 * @param primary_cmd_buf (optional)
	 * @return Whether it succeded or not
	 */
	VkResult begin(VkCommandBufferUsageFlags flags, CommandBuffer *primary_cmd_buf = nullptr);

	VkResult end();

	void clear(VkClearAttachment info, VkClearRect rect);

	void begin_render_pass(const RenderTarget &render_target, const std::vector<LoadStoreInfo> &load_store_infos, const std::vector<VkClearValue> &clear_values, const std::vector<std::unique_ptr<Subpass>> &subpasses, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

	void next_subpass();

	void execute_commands(CommandBuffer &secondary_command_buffer);

	void execute_commands(std::vector<CommandBuffer *> &secondary_command_buffers);

	void end_render_pass();

	void bind_pipeline_layout(PipelineLayout &pipeline_layout);

	template <class T>
	void set_specialization_constant(uint32_t constant_id, const T &data);

	void set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t> &data);

	/**
	 * @brief Records byte data into the command buffer to be pushed as push constants to each draw call
	 * @param values The byte data to store
	 */
	void push_constants(const std::vector<uint8_t> &values);

	template <typename T>
	void push_constants(const T &value)
	{
		auto data = to_bytes(value);

		uint32_t size = to_u32(stored_push_constants.size() + data.size());

		if (size > max_push_constants_size)
		{
			LOGE("Push constant limit exceeded ({} / {} bytes)", size, max_push_constants_size);
			throw std::runtime_error("Cannot overflow push constant limit");
		}

		stored_push_constants.insert(stored_push_constants.end(), data.begin(), data.end());
	}

	void bind_buffer(const core::Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_image(const core::ImageView &image_view, const core::Sampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_input(const core::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_vertex_buffers(uint32_t first_binding, const std::vector<std::reference_wrapper<const vkb::core::Buffer>> &buffers, const std::vector<VkDeviceSize> &offsets);

	void bind_index_buffer(const core::Buffer &buffer, VkDeviceSize offset, VkIndexType index_type);

	void set_viewport_state(const ViewportState &state_info);

	void set_vertex_input_state(const VertexInputState &state_info);

	void set_input_assembly_state(const InputAssemblyState &state_info);

	void set_rasterization_state(const RasterizationState &state_info);

	void set_multisample_state(const MultisampleState &state_info);

	void set_depth_stencil_state(const DepthStencilState &state_info);

	void set_color_blend_state(const ColorBlendState &state_info);

	void set_viewport(uint32_t first_viewport, const std::vector<VkViewport> &viewports);

	void set_scissor(uint32_t first_scissor, const std::vector<VkRect2D> &scissors);

	void set_line_width(float line_width);

	void set_depth_bias(float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor);

	void set_blend_constants(const std::array<float, 4> &blend_constants);

	void set_depth_bounds(float min_depth_bounds, float max_depth_bounds);

	void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);

	void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);

	void draw_indexed_indirect(const core::Buffer &buffer, VkDeviceSize offset, uint32_t draw_count, uint32_t stride);

	void dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);

	void dispatch_indirect(const core::Buffer &buffer, VkDeviceSize offset);

	void update_buffer(const core::Buffer &buffer, VkDeviceSize offset, const std::vector<uint8_t> &data);

	void blit_image(const core::Image &src_img, const core::Image &dst_img, const std::vector<VkImageBlit> &regions);

	void resolve_image(const core::Image &src_img, const core::Image &dst_img, const std::vector<VkImageResolve> &regions);

	void copy_buffer(const core::Buffer &src_buffer, const core::Buffer &dst_buffer, VkDeviceSize size);

	void copy_image(const core::Image &src_img, const core::Image &dst_img, const std::vector<VkImageCopy> &regions);

	void copy_buffer_to_image(const core::Buffer &buffer, const core::Image &image, const std::vector<VkBufferImageCopy> &regions);

	void image_memory_barrier(const core::ImageView &image_view, const ImageMemoryBarrier &memory_barrier);

	void buffer_memory_barrier(const core::Buffer &buffer, VkDeviceSize offset, VkDeviceSize size, const BufferMemoryBarrier &memory_barrier);

	const State get_state() const;

	void set_update_after_bind(bool update_after_bind_);

	void reset_query_pool(const QueryPool &query_pool, uint32_t first_query, uint32_t query_count);

	void begin_query(const QueryPool &query_pool, uint32_t query, VkQueryControlFlags flags);

	void end_query(const QueryPool &query_pool, uint32_t query);

	void write_timestamp(VkPipelineStageFlagBits pipeline_stage, const QueryPool &query_pool, uint32_t query);

	/**
	 * @brief Reset the command buffer to a state where it can be recorded to
	 * @param reset_mode How to reset the buffer, should match the one used by the pool to allocate it
	 */
	VkResult reset(ResetMode reset_mode);

	const VkCommandBufferLevel level;

  private:
	State state{State::Initial};

	CommandPool &command_pool;

	VkCommandBuffer handle{VK_NULL_HANDLE};

	RenderPassBinding current_render_pass;

	PipelineState pipeline_state;

	ResourceBindingState resource_binding_state;

	std::vector<uint8_t> stored_push_constants;

	uint32_t max_push_constants_size;

	VkExtent2D last_framebuffer_extent{};

	VkExtent2D last_render_area_extent{};

	// If true, it becomes the responsibility of the caller to update ANY descriptor bindings
	// that contain update after bind, as they wont be implicitly updated
	bool update_after_bind{false};

	std::unordered_map<uint32_t, DescriptorSetLayout *> descriptor_set_layout_binding_state;

	const RenderPassBinding &get_current_render_pass() const;

	const uint32_t get_current_subpass_index() const;

	/**
	 * @brief Check that the render area is an optimal size by comparing to the render area granularity
	 */
	const bool is_render_size_optimal(const VkExtent2D &extent, const VkRect2D &render_area);

	/**
	 * @brief Flush the piplines state
	 */
	void flush_pipeline_state(VkPipelineBindPoint pipeline_bind_point);

	/**
	 * @brief Flush the descriptor set state
	 */
	void flush_descriptor_state(VkPipelineBindPoint pipeline_bind_point);

	/**
	 * @brief Flush the push constant state
	 */
	void flush_push_constants();
};

template <class T>
inline void CommandBuffer::set_specialization_constant(uint32_t constant_id, const T &data)
{
	set_specialization_constant(constant_id, to_bytes(data));
}

template <>
inline void CommandBuffer::set_specialization_constant<bool>(std::uint32_t constant_id, const bool &data)
{
	set_specialization_constant(constant_id, to_bytes(to_u32(data)));
}
}        // namespace vkb
