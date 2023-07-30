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

#include <common/hpp_vk_common.h>
#include <core/hpp_framebuffer.h>
#include <core/hpp_query_pool.h>
#include <hpp_resource_binding_state.h>
#include <rendering/hpp_pipeline_state.h>
#include <rendering/hpp_render_target.h>
#include <rendering/hpp_subpass.h>

namespace vkb
{
namespace core
{
class HPPCommandPool;
class HPPDescriptorSetLayout;

/**
 * @brief Helper class to manage and record a command buffer, building and
 *        keeping track of pipeline state and resource bindings
 */
class HPPCommandBuffer : public core::HPPVulkanResource<vk::CommandBuffer>
{
  public:
	struct RenderPassBinding
	{
		const vkb::core::HPPRenderPass  *render_pass;
		const vkb::core::HPPFramebuffer *framebuffer;
	};

	enum class ResetMode
	{
		ResetPool,
		ResetIndividually,
		AlwaysAllocate,
	};

  public:
	HPPCommandBuffer(vkb::core::HPPCommandPool &command_pool, vk::CommandBufferLevel level);
	HPPCommandBuffer(HPPCommandBuffer &&other);
	~HPPCommandBuffer();

	HPPCommandBuffer(const HPPCommandBuffer &)            = delete;
	HPPCommandBuffer &operator=(const HPPCommandBuffer &) = delete;
	HPPCommandBuffer &operator=(HPPCommandBuffer &&)      = delete;

	/**
	 * @brief Sets the command buffer so that it is ready for recording
	 *        If it is a secondary command buffer, a pointer to the
	 *        primary command buffer it inherits from must be provided
	 * @param flags Usage behavior for the command buffer
	 * @param primary_cmd_buf (optional)
	 * @return Whether it succeeded or not
	 */
	vk::Result begin(vk::CommandBufferUsageFlags flags, HPPCommandBuffer *primary_cmd_buf = nullptr);

	/**
	 * @brief Sets the command buffer so that it is ready for recording
	 *        If it is a secondary command buffer, pointers to the
	 *        render pass and framebuffer as well as subpass index must be provided
	 * @param flags Usage behavior for the command buffer
	 * @param render_pass
	 * @param framebuffer
	 * @param subpass_index
	 * @return Whether it succeeded or not
	 */
	vk::Result begin(vk::CommandBufferUsageFlags flags, const vkb::core::HPPRenderPass *render_pass, const vkb::core::HPPFramebuffer *framebuffer, uint32_t subpass_index);

	void                      begin_query(const vkb::core::HPPQueryPool &query_pool, uint32_t query, vk::QueryControlFlags flags);
	void                      begin_render_pass(const vkb::rendering::HPPRenderTarget                          &render_target,
	                                            const std::vector<vkb::common::HPPLoadStoreInfo>               &load_store_infos,
	                                            const std::vector<vk::ClearValue>                              &clear_values,
	                                            const std::vector<std::unique_ptr<vkb::rendering::HPPSubpass>> &subpasses,
	                                            vk::SubpassContents                                             contents = vk::SubpassContents::eInline);
	void                      begin_render_pass(const vkb::rendering::HPPRenderTarget &render_target,
	                                            const vkb::core::HPPRenderPass        &render_pass,
	                                            const vkb::core::HPPFramebuffer       &framebuffer,
	                                            const std::vector<vk::ClearValue>     &clear_values,
	                                            vk::SubpassContents                    contents = vk::SubpassContents::eInline);
	void                      bind_buffer(const vkb::core::HPPBuffer &buffer, vk::DeviceSize offset, vk::DeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element);
	void                      bind_image(const vkb::core::HPPImageView &image_view, const vkb::core::HPPSampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element);
	void                      bind_image(const vkb::core::HPPImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element);
	void                      bind_index_buffer(const vkb::core::HPPBuffer &buffer, vk::DeviceSize offset, vk::IndexType index_type);
	void                      bind_input(const vkb::core::HPPImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element);
	void                      bind_lighting(vkb::rendering::HPPLightingState &lighting_state, uint32_t set, uint32_t binding);
	void                      bind_pipeline_layout(vkb::core::HPPPipelineLayout &pipeline_layout);
	void                      bind_vertex_buffers(uint32_t                                                               first_binding,
	                                              const std::vector<std::reference_wrapper<const vkb::core::HPPBuffer>> &buffers,
	                                              const std::vector<vk::DeviceSize>                                     &offsets);
	void                      blit_image(const vkb::core::HPPImage &src_img, const vkb::core::HPPImage &dst_img, const std::vector<vk::ImageBlit> &regions);
	void                      buffer_memory_barrier(const vkb::core::HPPBuffer                &buffer,
	                                                vk::DeviceSize                             offset,
	                                                vk::DeviceSize                             size,
	                                                const vkb::common::HPPBufferMemoryBarrier &memory_barrier);
	void                      clear(vk::ClearAttachment info, vk::ClearRect rect);
	void                      copy_buffer(const vkb::core::HPPBuffer &src_buffer, const vkb::core::HPPBuffer &dst_buffer, vk::DeviceSize size);
	void                      copy_buffer_to_image(const vkb::core::HPPBuffer &buffer, const vkb::core::HPPImage &image, const std::vector<vk::BufferImageCopy> &regions);
	void                      copy_image(const vkb::core::HPPImage &src_img, const vkb::core::HPPImage &dst_img, const std::vector<vk::ImageCopy> &regions);
	void                      copy_image_to_buffer(const vkb::core::HPPImage              &image,
	                                               vk::ImageLayout                         image_layout,
	                                               const vkb::core::HPPBuffer             &buffer,
	                                               const std::vector<vk::BufferImageCopy> &regions);
	void                      dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);
	void                      dispatch_indirect(const vkb::core::HPPBuffer &buffer, vk::DeviceSize offset);
	void                      draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
	void                      draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);
	void                      draw_indexed_indirect(const vkb::core::HPPBuffer &buffer, vk::DeviceSize offset, uint32_t draw_count, uint32_t stride);
	vk::Result                end();
	void                      end_query(const vkb::core::HPPQueryPool &query_pool, uint32_t query);
	void                      end_render_pass();
	void                      execute_commands(HPPCommandBuffer &secondary_command_buffer);
	void                      execute_commands(std::vector<HPPCommandBuffer *> &secondary_command_buffers);
	vkb::core::HPPRenderPass &get_render_pass(const vkb::rendering::HPPRenderTarget                          &render_target,
	                                          const std::vector<vkb::common::HPPLoadStoreInfo>               &load_store_infos,
	                                          const std::vector<std::unique_ptr<vkb::rendering::HPPSubpass>> &subpasses);
	void                      image_memory_barrier(const vkb::core::HPPImageView &image_view, const vkb::common::HPPImageMemoryBarrier &memory_barrier) const;
	void                      next_subpass();

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

	/**
	 * @brief Reset the command buffer to a state where it can be recorded to
	 * @param reset_mode How to reset the buffer, should match the one used by the pool to allocate it
	 */
	vk::Result reset(ResetMode reset_mode);

	void reset_query_pool(const vkb::core::HPPQueryPool &query_pool, uint32_t first_query, uint32_t query_count);
	void resolve_image(const vkb::core::HPPImage &src_img, const vkb::core::HPPImage &dst_img, const std::vector<vk::ImageResolve> &regions);
	void set_blend_constants(const std::array<float, 4> &blend_constants);
	void set_color_blend_state(const vkb::rendering::HPPColorBlendState &state_info);
	void set_depth_bias(float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor);
	void set_depth_bounds(float min_depth_bounds, float max_depth_bounds);
	void set_depth_stencil_state(const vkb::rendering::HPPDepthStencilState &state_info);
	void set_input_assembly_state(const vkb::rendering::HPPInputAssemblyState &state_info);
	void set_line_width(float line_width);
	void set_multisample_state(const vkb::rendering::HPPMultisampleState &state_info);
	void set_rasterization_state(const vkb::rendering::HPPRasterizationState &state_info);
	void set_scissor(uint32_t first_scissor, const std::vector<vk::Rect2D> &scissors);

	template <class T>
	void set_specialization_constant(uint32_t constant_id, const T &data);

	void set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t> &data);

	void set_update_after_bind(bool update_after_bind_);
	void set_vertex_input_state(const vkb::rendering::HPPVertexInputState &state_info);
	void set_viewport(uint32_t first_viewport, const std::vector<vk::Viewport> &viewports);
	void set_viewport_state(const vkb::rendering::HPPViewportState &state_info);
	void update_buffer(const vkb::core::HPPBuffer &buffer, vk::DeviceSize offset, const std::vector<uint8_t> &data);
	void write_timestamp(vk::PipelineStageFlagBits pipeline_stage, const vkb::core::HPPQueryPool &query_pool, uint32_t query);

  private:
	/**
	 * @brief Flushes the command buffer, pushing the new changes
	 * @param pipeline_bind_point The type of pipeline we want to flush
	 */
	void flush(vk::PipelineBindPoint pipeline_bind_point);

	/**
	 * @brief Flush the descriptor set state
	 */
	void flush_descriptor_state(vk::PipelineBindPoint pipeline_bind_point);

	/**
	 * @brief Flush the pipeline state
	 */
	void flush_pipeline_state(vk::PipelineBindPoint pipeline_bind_point);

	/**
	 * @brief Flush the push constant state
	 */
	void flush_push_constants();

	const RenderPassBinding &get_current_render_pass() const;
	const uint32_t           get_current_subpass_index() const;

	/**
	 * @brief Check that the render area is an optimal size by comparing to the render area granularity
	 */
	const bool is_render_size_optimal(const vk::Extent2D &extent, const vk::Rect2D &render_area);

  private:
	const vk::CommandBufferLevel     level = {};
	vkb::core::HPPCommandPool       &command_pool;
	RenderPassBinding                current_render_pass     = {};
	vkb::rendering::HPPPipelineState pipeline_state          = {};
	vkb::HPPResourceBindingState     resource_binding_state  = {};
	std::vector<uint8_t>             stored_push_constants   = {};
	uint32_t                         max_push_constants_size = {};
	vk::Extent2D                     last_framebuffer_extent = {};
	vk::Extent2D                     last_render_area_extent = {};

	// If true, it becomes the responsibility of the caller to update ANY descriptor bindings
	// that contain update after bind, as they wont be implicitly updated
	bool update_after_bind = false;

	std::unordered_map<uint32_t, vkb::core::HPPDescriptorSetLayout const *> descriptor_set_layout_binding_state;
};

template <class T>
inline void HPPCommandBuffer::set_specialization_constant(uint32_t constant_id, const T &data)
{
	set_specialization_constant(constant_id, to_bytes(data));
}

template <>
inline void HPPCommandBuffer::set_specialization_constant<bool>(std::uint32_t constant_id, const bool &data)
{
	set_specialization_constant(constant_id, to_bytes(to_u32(data)));
}
}        // namespace core
}        // namespace vkb
