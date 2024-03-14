/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "core/vulkan_resource.h"
#include "hpp_resource_binding_state.h"
#include "rendering/hpp_pipeline_state.h"

namespace vkb
{
class CommandPool;
class Framebuffer;
struct LightingState;
class QueryPool;
class RenderTarget;
class Subpass;

namespace common
{
struct HPPBufferMemoryBarrier;
struct HPPImageMemoryBarrier;
struct HPPLoadStoreInfo;
}        // namespace common

namespace rendering
{
struct HPPLightingState;
class HPPRenderTarget;
class HPPSubpass;
}        // namespace rendering

namespace core
{
class Buffer;
class HPPBuffer;
class HPPCommandPool;
class HPPDescriptorSetLayout;
class HPPFramebuffer;
class HPPImage;
class HPPImageView;
class HPPPipelineLayout;
class HPPQueryPool;
class HPPRenderPass;
class HPPSampler;
class Image;
class ImageView;
class Sampler;

/**
 * @brief Helper class to manage and record a command buffer, building and
 *        keeping track of pipeline state and resource bindings
 */
template <vkb::BindingType bindingType>
class CommandBuffer
    : public vkb::core::VulkanResource<bindingType, typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandBuffer, VkCommandBuffer>::type>
{
	using Parent =
	    vkb::core::VulkanResource<bindingType, typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandBuffer, VkCommandBuffer>::type>;

  public:
	using BufferImageCopyType    = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::BufferImageCopy, VkBufferImageCopy>::type;
	using ClearAttachmentType    = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ClearAttachment, VkClearAttachment>::type;
	using ClearRectType          = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ClearRect, VkClearRect>::type;
	using ClearValueType         = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ClearValue, VkClearValue>::type;
	using CommandBufferLevelType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandBufferLevel, VkCommandBufferLevel>::type;
	using CommandBufferType      = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandBuffer, VkCommandBuffer>::type;
	using CommandBufferUsageFlagsType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandBufferUsageFlags, VkCommandBufferUsageFlags>::type;
	using DeviceSizeType        = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DeviceSize, VkDeviceSize>::type;
	using ImageBlitType         = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ImageBlit, VkImageBlit>::type;
	using ImageCopyType         = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ImageCopy, VkImageCopy>::type;
	using ImageLayoutType       = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ImageLayout, VkImageLayout>::type;
	using ImageResolveType      = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ImageResolve, VkImageResolve>::type;
	using IndexTypeType         = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::IndexType, VkIndexType>::type;
	using PipelineBindPointType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::PipelineBindPoint, VkPipelineBindPoint>::type;
	using PipelineStagFlagBitsType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::PipelineStageFlagBits, VkPipelineStageFlagBits>::type;
	using QueryControlFlagsType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::QueryControlFlags, VkQueryControlFlags>::type;
	using Rect2DType            = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Rect2D, VkRect2D>::type;
	using ResultType            = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Result, VkResult>::type;
	using SubpassContentsType   = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::SubpassContents, VkSubpassContents>::type;
	using ViewportType          = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Viewport, VkViewport>::type;

	using BufferMemoryBarrierType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::common::HPPBufferMemoryBarrier, vkb::BufferMemoryBarrier>::type;
	using BufferType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPBuffer, vkb::core::Buffer>::type;
	using ColorBlendStateType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPColorBlendState, vkb::ColorBlendState>::type;
	using CommandPoolType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPCommandPool, vkb::CommandPool>::type;
	using DepthStencilStateType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPDepthStencilState, vkb::DepthStencilState>::type;
	using FramebufferType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPFramebuffer, vkb::Framebuffer>::type;
	using ImageMemoryBarrierType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::common::HPPImageMemoryBarrier, vkb::ImageMemoryBarrier>::type;
	using ImageType     = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPImage, vkb::core::Image>::type;
	using ImageViewType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPImageView, vkb::core::ImageView>::type;
	using InputAssemblyStateType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPInputAssemblyState, vkb::InputAssemblyState>::type;
	using LightingStateType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPLightingState, vkb::LightingState>::type;
	using LoadStoreInfoType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::common::HPPLoadStoreInfo, vkb::LoadStoreInfo>::type;
	using MultisampleStateType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPMultisampleState, vkb::MultisampleState>::type;
	using PipelineLayoutType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPPipelineLayout, vkb::PipelineLayout>::type;
	using QueryPoolType      = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPQueryPool, vkb::QueryPool>::type;
	using RasterizationStateType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPRasterizationState, vkb::RasterizationState>::type;
	using RenderPassType   = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPRenderPass, vkb::RenderPass>::type;
	using RenderTargetType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPRenderTarget, vkb::RenderTarget>::type;
	using SamplerType      = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPSampler, vkb::core::Sampler>::type;
	using SubpassType      = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPSubpass, vkb::Subpass>::type;
	using VertexInputStateType =
	    typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPVertexInputState, vkb::VertexInputState>::type;
	using ViewportStateType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::rendering::HPPViewportState, vkb::ViewportState>::type;

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
	CommandBuffer(CommandPoolType &command_pool, CommandBufferLevelType level);
	CommandBuffer(CommandBuffer<bindingType> &&other);
	~CommandBuffer();

	CommandBuffer(CommandBuffer<bindingType> const &)            = delete;
	CommandBuffer &operator=(CommandBuffer<bindingType> const &) = delete;
	CommandBuffer &operator=(CommandBuffer<bindingType> &&)      = delete;

	/**
	 * @brief Sets the command buffer so that it is ready for recording
	 *        If it is a secondary command buffer, a pointer to the
	 *        primary command buffer it inherits from must be provided
	 * @param flags Usage behavior for the command buffer
	 * @param primary_cmd_buf (optional)
	 */
	void begin(CommandBufferUsageFlagsType flags, CommandBuffer<bindingType> *primary_cmd_buf = nullptr);

	/**
	 * @brief Sets the command buffer so that it is ready for recording
	 *        If it is a secondary command buffer, pointers to the
	 *        render pass and framebuffer as well as subpass index must be provided
	 * @param flags Usage behavior for the command buffer
	 * @param render_pass
	 * @param framebuffer
	 * @param subpass_index
	 */
	void begin(CommandBufferUsageFlagsType flags, const RenderPassType *render_pass, const FramebufferType *framebuffer, uint32_t subpass_index);

	void                   begin_query(QueryPoolType const &query_pool, uint32_t query, QueryControlFlagsType flags);
	void                   begin_render_pass(RenderTargetType const                          &render_target,
	                                         std::vector<LoadStoreInfoType> const            &load_store_infos,
	                                         std::vector<ClearValueType> const               &clear_values,
	                                         std::vector<std::unique_ptr<SubpassType>> const &subpasses,
	                                         SubpassContentsType                              contents = VK_SUBPASS_CONTENTS_INLINE);
	void                   begin_render_pass(RenderTargetType const            &render_target,
	                                         RenderPassType const              &render_pass,
	                                         FramebufferType const             &framebuffer,
	                                         std::vector<ClearValueType> const &clear_values,
	                                         SubpassContentsType                contents = vk::SubpassContents::eInline);
	void                   bind_buffer(BufferType const &buffer, DeviceSizeType offset, DeviceSizeType range, uint32_t set, uint32_t binding, uint32_t array_element);
	void                   bind_image(ImageViewType const &image_view, SamplerType const &sampler, uint32_t set, uint32_t binding, uint32_t array_element);
	void                   bind_image(ImageViewType const &image_view, uint32_t set, uint32_t binding, uint32_t array_element);
	void                   bind_index_buffer(BufferType const &buffer, DeviceSizeType offset, IndexTypeType index_type);
	void                   bind_input(ImageViewType const &image_view, uint32_t set, uint32_t binding, uint32_t array_element);
	void                   bind_lighting(LightingStateType &lighting_state, uint32_t set, uint32_t binding);
	void                   bind_pipeline_layout(PipelineLayoutType &pipeline_layout);
	void                   bind_vertex_buffers(uint32_t                                                     first_binding,
	                                           std::vector<std::reference_wrapper<const BufferType>> const &buffers,
	                                           std::vector<DeviceSizeType> const                           &offsets);
	void                   blit_image(ImageType const &src_img, ImageType const &dst_img, std::vector<ImageBlitType> const &regions);
	void                   buffer_memory_barrier(BufferType const &buffer, DeviceSizeType offset, DeviceSizeType size, BufferMemoryBarrierType const &memory_barrier);
	void                   clear(ClearAttachmentType const &info, ClearRectType const &rect);
	void                   copy_buffer(BufferType const &src_buffer, BufferType const &dst_buffer, DeviceSizeType size);
	void                   copy_buffer_to_image(BufferType const &buffer, ImageType const &image, std::vector<BufferImageCopyType> const &regions);
	void                   copy_image(ImageType const &src_img, ImageType const &dst_img, std::vector<ImageCopyType> const &regions);
	void                   copy_image_to_buffer(ImageType const                        &image,
	                                            ImageLayoutType                         image_layout,
	                                            BufferType const                       &buffer,
	                                            std::vector<BufferImageCopyType> const &regions);
	void                   dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);
	void                   dispatch_indirect(BufferType const &buffer, DeviceSizeType offset);
	void                   draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
	void                   draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);
	void                   draw_indexed_indirect(BufferType const &buffer, DeviceSizeType offset, uint32_t draw_count, uint32_t stride);
	void                   end();
	void                   end_query(QueryPoolType const &query_pool, uint32_t query);
	void                   end_render_pass();
	void                   execute_commands(vkb::core::CommandBuffer<bindingType> &secondary_command_buffer);
	void                   execute_commands(std::vector<vkb::core::CommandBuffer<bindingType> *> &secondary_command_buffers);
	CommandBufferLevelType get_level() const;
	RenderPassType        &get_render_pass(RenderTargetType const                          &render_target,
	                                       std::vector<LoadStoreInfoType> const            &load_store_infos,
	                                       std::vector<std::unique_ptr<SubpassType>> const &subpasses);
	void                   image_memory_barrier(ImageViewType const &image_view, ImageMemoryBarrierType const &memory_barrier) const;
	void                   next_subpass();

	/**
	 * @brief Records byte data into the command buffer to be pushed as push constants to each draw call
	 * @param values The byte data to store
	 */
	void push_constants(const std::vector<uint8_t> &values);
	template <typename T>
	void push_constants(const T &value);

	/**
	 * @brief Reset the command buffer to a state where it can be recorded to
	 * @param reset_mode How to reset the buffer, should match the one used by the pool to allocate it
	 */
	ResultType reset(ResetMode reset_mode);

	void reset_query_pool(QueryPoolType const &query_pool, uint32_t first_query, uint32_t query_count);
	void resolve_image(ImageType const &src_img, ImageType const &dst_img, std::vector<ImageResolveType> const &regions);
	void set_blend_constants(std::array<float, 4> const &blend_constants);
	void set_color_blend_state(ColorBlendStateType const &state_info);
	void set_depth_bias(float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor);
	void set_depth_bounds(float min_depth_bounds, float max_depth_bounds);
	void set_depth_stencil_state(DepthStencilStateType const &state_info);
	void set_input_assembly_state(InputAssemblyStateType const &state_info);
	void set_line_width(float line_width);
	void set_multisample_state(MultisampleStateType const &state_info);
	void set_rasterization_state(RasterizationStateType const &state_info);
	void set_scissor(uint32_t first_scissor, std::vector<Rect2DType> const &scissors);

	template <class T>
	void set_specialization_constant(uint32_t constant_id, T const &data);
	void set_specialization_constant(uint32_t constant_id, std::vector<uint8_t> const &data);

	void set_update_after_bind(bool update_after_bind_);
	void set_vertex_input_state(VertexInputStateType const &state_info);
	void set_viewport(uint32_t first_viewport, std::vector<ViewportType> const &viewports);
	void set_viewport_state(ViewportStateType const &state_info);
	void update_buffer(BufferType const &buffer, DeviceSizeType offset, std::vector<uint8_t> const &data);
	void write_timestamp(PipelineStagFlagBitsType pipeline_stage, QueryPoolType const &query_pool, uint32_t query);

  private:
	/**
	 * @brief Flushes the command buffer, pushing the new changes
	 */
	void flush(vk::PipelineBindPoint pipeline_bind_point);

	/**
	 * @brief Flush the push constant state
	 */
	void flush_push_constants();

	RenderPassBinding const &get_current_render_pass() const;
	uint32_t                 get_current_subpass_index() const;

	/**
	 * @brief Check that the render area is an optimal size by comparing to the render area granularity
	 */
	bool is_render_size_optimal(const vk::Extent2D &extent, const vk::Rect2D &render_area);

  private:
	void                      begin_impl(vk::CommandBufferUsageFlags flags, vkb::core::CommandBuffer<bindingType> *primary_cmd_buf);
	void                      begin_impl(vk::CommandBufferUsageFlags      flags,
	                                     vkb::core::HPPRenderPass const  *render_pass,
	                                     vkb::core::HPPFramebuffer const *framebuffer,
	                                     uint32_t                         subpass_index);
	void                      begin_render_pass_impl(vkb::rendering::HPPRenderTarget const &render_target,
	                                                 vkb::core::HPPRenderPass const        &render_pass,
	                                                 vkb::core::HPPFramebuffer const       &framebuffer,
	                                                 std::vector<vk::ClearValue> const     &clear_values,
	                                                 vk::SubpassContents                    contents);
	void                      bind_vertex_buffers_impl(uint32_t                                                               first_binding,
	                                                   std::vector<std::reference_wrapper<const vkb::core::HPPBuffer>> const &buffers,
	                                                   std::vector<vk::DeviceSize> const                                     &offsets);
	void                      buffer_memory_barrier_impl(vkb::core::HPPBuffer const                &buffer,
	                                                     vk::DeviceSize                             offset,
	                                                     vk::DeviceSize                             size,
	                                                     vkb::common::HPPBufferMemoryBarrier const &memory_barrier);
	void                      copy_buffer_impl(vkb::core::HPPBuffer const &src_buffer, vkb::core::HPPBuffer const &dst_buffer, vk::DeviceSize size);
	void                      execute_commands_impl(std::vector<vkb::core::CommandBuffer<vkb::BindingType::Cpp> *> &secondary_command_buffers);
	void                      flush_impl(vkb::core::HPPDevice &device, vk::PipelineBindPoint pipeline_bind_point);
	void                      flush_descriptor_state_impl(vk::PipelineBindPoint pipeline_bind_point);
	void                      flush_pipeline_state_impl(vkb::core::HPPDevice &device, vk::PipelineBindPoint pipeline_bind_point);
	vkb::core::HPPRenderPass &get_render_pass_impl(vkb::core::HPPDevice                                           &device,
	                                               vkb::rendering::HPPRenderTarget const                          &render_target,
	                                               std::vector<vkb::common::HPPLoadStoreInfo> const               &load_store_infos,
	                                               std::vector<std::unique_ptr<vkb::rendering::HPPSubpass>> const &subpasses);
	void                      image_memory_barrier_impl(vkb::core::HPPImageView const &image_view, vkb::common::HPPImageMemoryBarrier const &memory_barrier) const;
	vk::Result                reset_impl(ResetMode reset_mode);

  private:
	vkb::core::HPPCommandPool                                              &command_pool;
	RenderPassBinding                                                       current_render_pass = {};
	std::unordered_map<uint32_t, vkb::core::HPPDescriptorSetLayout const *> descriptor_set_layout_binding_state;
	vk::Extent2D                                                            last_framebuffer_extent = {};
	vk::Extent2D                                                            last_render_area_extent = {};
	const vk::CommandBufferLevel                                            level                   = {};
	const uint32_t                                                          max_push_constants_size = {};
	vkb::rendering::HPPPipelineState                                        pipeline_state          = {};
	vkb::HPPResourceBindingState                                            resource_binding_state  = {};
	std::vector<uint8_t>                                                    stored_push_constants   = {};

	// If true, it becomes the responsibility of the caller to update ANY descriptor bindings
	// that contain update after bind, as they wont be implicitly updated
	bool update_after_bind = false;
};
}        // namespace core
}        // namespace vkb

#include "common/hpp_vk_common.h"
#include "core/hpp_command_pool.h"
#include "core/hpp_device.h"
#include "rendering/hpp_render_frame.h"
#include "rendering/hpp_render_target.h"
#include "rendering/hpp_subpass.h"

namespace vkb
{
namespace core
{
template <vkb::BindingType bindingType>
inline vkb::core::CommandBuffer<bindingType>::CommandBuffer(CommandPoolType &command_pool_, CommandBufferLevelType level_) :
    vkb::core::VulkanResource<bindingType, CommandBufferType>(nullptr, &command_pool_.get_device()), level(static_cast<vk::CommandBufferLevel>(level_)), command_pool(reinterpret_cast<vkb::core::HPPCommandPool &>(command_pool_)), max_push_constants_size(this->get_device().get_gpu().get_properties().limits.maxPushConstantsSize)
{
	vk::CommandBufferAllocateInfo allocate_info(command_pool.get_handle(), level, 1);

	set_handle(this->get_device().get_resource().allocateCommandBuffers(allocate_info).front());
}

template <vkb::BindingType bindingType>
inline CommandBuffer<bindingType>::CommandBuffer(CommandBuffer<bindingType> &&other) :
    vkb::core::VulkanResource<bindingType, CommandBufferType>(std::move(other)), level(other.level), command_pool(other.command_pool), current_render_pass(std::exchange(other.current_render_pass, {})), pipeline_state(std::exchange(other.pipeline_state, {})), resource_binding_state(std::exchange(other.resource_binding_state, {})), stored_push_constants(std::exchange(other.stored_push_constants, {})), max_push_constants_size(std::exchange(other.max_push_constants_size, {})), last_framebuffer_extent(std::exchange(other.last_framebuffer_extent, {})), last_render_area_extent(std::exchange(other.last_render_area_extent, {})), update_after_bind(std::exchange(other.update_after_bind, {})), descriptor_set_layout_binding_state(std::exchange(other.descriptor_set_layout_binding_state, {}))
{
}

template <vkb::BindingType bindingType>
inline CommandBuffer<bindingType>::~CommandBuffer()
{
	// Destroy command buffer
	if (this->has_handle())
	{
		this->get_device().get_resource().freeCommandBuffers(command_pool.get_handle(), this->get_resource());
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::begin(CommandBufferUsageFlagsType            flags,
                                              vkb::core::CommandBuffer<bindingType> *primary_cmd_buf)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		begin_impl(flags, primary_cmd_buf);
	}
	else
	{
		begin_impl(static_cast<vk::CommandBufferUsageFlags>(flags), primary_cmd_buf);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::begin_impl(vk::CommandBufferUsageFlags flags, vkb::core::CommandBuffer<bindingType> *primary_cmd_buf)
{
	if (level == vk::CommandBufferLevel::eSecondary)
	{
		assert(primary_cmd_buf && "A primary command buffer pointer must be provided when calling begin from a secondary one");
		auto const &render_pass_binding = primary_cmd_buf->get_current_render_pass();

		return begin_impl(flags, render_pass_binding.render_pass, render_pass_binding.framebuffer, primary_cmd_buf->get_current_subpass_index());
	}
	else
	{
		return begin_impl(flags, nullptr, nullptr, 0);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::begin(CommandBufferUsageFlagsType flags,
                                              const RenderPassType       *render_pass,
                                              const FramebufferType      *framebuffer,
                                              uint32_t                    subpass_index)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		begin_impl(flags, render_pass, framebuffer, subpass_index);
	}
	else
	{
		begin_impl(static_cast<vk::CommandBufferUsageFlags>(flags),
		           reinterpret_cast<vkb::core::HPPRenderPass const *>(render_pass),
		           reinterpret_cast<vkb::core::HPPFramebuffer const *>(framebuffer),
		           subpass_index);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::begin_impl(vk::CommandBufferUsageFlags      flags,
                                                   vkb::core::HPPRenderPass const  *render_pass,
                                                   vkb::core::HPPFramebuffer const *framebuffer,
                                                   uint32_t                         subpass_index)
{
	// Reset state
	pipeline_state.reset();
	resource_binding_state.reset();
	descriptor_set_layout_binding_state.clear();
	stored_push_constants.clear();

	vk::CommandBufferBeginInfo       begin_info(flags);
	vk::CommandBufferInheritanceInfo inheritance;

	if (level == vk::CommandBufferLevel::eSecondary)
	{
		assert((render_pass && framebuffer) && "Render pass and framebuffer must be provided when calling begin from a secondary one");

		current_render_pass.render_pass = render_pass;
		current_render_pass.framebuffer = framebuffer;

		inheritance.renderPass  = current_render_pass.render_pass->get_handle();
		inheritance.framebuffer = current_render_pass.framebuffer->get_handle();
		inheritance.subpass     = subpass_index;

		begin_info.pInheritanceInfo = &inheritance;
	}

	this->get_resource().begin(begin_info);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::begin_query(QueryPoolType const &query_pool, uint32_t query, QueryControlFlagsType flags)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().beginQuery(query_pool.get_resource(), query, flags);
	}
	else
	{
		this->get_resource().beginQuery(static_cast<vk::QueryPool const>(query_pool.get_handle()), query, static_cast<vk::QueryControlFlags>(flags));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::begin_render_pass(RenderTargetType const                          &render_target,
                                                          std::vector<LoadStoreInfoType> const            &load_store_infos,
                                                          std::vector<ClearValueType> const               &clear_values,
                                                          std::vector<std::unique_ptr<SubpassType>> const &subpasses,
                                                          SubpassContentsType                              contents)
{
	// Reset state
	pipeline_state.reset();
	resource_binding_state.reset();
	descriptor_set_layout_binding_state.clear();

	auto &render_pass = get_render_pass(render_target, load_store_infos, subpasses);
	auto &framebuffer = this->get_device().get_resource_cache().request_framebuffer(render_target, render_pass);

	begin_render_pass(render_target, render_pass, framebuffer, clear_values, contents);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::begin_render_pass(RenderTargetType const            &render_target,
                                                          RenderPassType const              &render_pass,
                                                          FramebufferType const             &framebuffer,
                                                          std::vector<ClearValueType> const &clear_values,
                                                          SubpassContentsType                contents)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		begin_render_pass_impl(render_target, render_pass, framebuffer, clear_values, contents);
	}
	else
	{
		begin_render_pass_impl(reinterpret_cast<vkb::rendering::HPPRenderTarget const &>(render_target),
		                       reinterpret_cast<vkb::core::HPPRenderPass const &>(render_pass),
		                       reinterpret_cast<vkb::core::HPPFramebuffer const &>(framebuffer),
		                       reinterpret_cast<std::vector<vk::ClearValue> const &>(clear_values),
		                       static_cast<vk::SubpassContents>(contents));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::begin_render_pass_impl(vkb::rendering::HPPRenderTarget const &render_target,
                                                               vkb::core::HPPRenderPass const        &render_pass,
                                                               vkb::core::HPPFramebuffer const       &framebuffer,
                                                               std::vector<vk::ClearValue> const     &clear_values,
                                                               vk::SubpassContents                    contents)
{
	current_render_pass.render_pass = &render_pass;
	current_render_pass.framebuffer = &framebuffer;

	// Begin render pass
	vk::RenderPassBeginInfo begin_info(
	    current_render_pass.render_pass->get_handle(), current_render_pass.framebuffer->get_handle(), {{}, render_target.get_extent()}, clear_values);

	const auto &framebuffer_extent = current_render_pass.framebuffer->get_extent();

	// Test the requested render area to confirm that it is optimal and could not cause a performance reduction
	if (!is_render_size_optimal(framebuffer_extent, begin_info.renderArea))
	{
		// Only prints the warning if the framebuffer or render area are different since the last time the render size was not optimal
		if ((framebuffer_extent != last_framebuffer_extent) || (begin_info.renderArea.extent != last_render_area_extent))
		{
			LOGW("Render target extent is not an optimal size, this may result in reduced performance.");
		}

		last_framebuffer_extent = framebuffer_extent;
		last_render_area_extent = begin_info.renderArea.extent;
	}

	this->get_resource().beginRenderPass(begin_info, contents);

	// Update blend state attachments for first subpass
	auto blend_state = pipeline_state.get_color_blend_state();
	blend_state.attachments.resize(current_render_pass.render_pass->get_color_output_count(pipeline_state.get_subpass_index()));
	pipeline_state.set_color_blend_state(blend_state);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::bind_buffer(
    BufferType const &buffer, DeviceSizeType offset, DeviceSizeType range, uint32_t set, uint32_t binding, uint32_t array_element)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		resource_binding_state.bind_buffer(buffer, offset, range, set, binding, array_element);
	}
	else
	{
		resource_binding_state.bind_buffer(reinterpret_cast<vkb::core::HPPBuffer const &>(buffer), offset, range, set, binding, array_element);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::bind_image(
    ImageViewType const &image_view, SamplerType const &sampler, uint32_t set, uint32_t binding, uint32_t array_element)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		resource_binding_state.bind_image(image_view, sampler, set, binding, array_element);
	}
	else
	{
		resource_binding_state.bind_image(reinterpret_cast<vkb::core::HPPImageView const &>(image_view),
		                                  reinterpret_cast<vkb::core::HPPSampler const &>(sampler),
		                                  set,
		                                  binding,
		                                  array_element);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::bind_image(ImageViewType const &image_view, uint32_t set, uint32_t binding, uint32_t array_element)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		resource_binding_state.bind_image(image_view, set, binding, array_element);
	}
	else
	{
		resource_binding_state.bind_image(reinterpret_cast<vkb::core::HPPImageView const &>(image_view), set, binding, array_element);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::bind_index_buffer(BufferType const &buffer, DeviceSizeType offset, IndexTypeType index_type)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().bindIndexBuffer(buffer.get_handle(), offset, index_type);
	}
	else
	{
		this->get_resource().bindIndexBuffer(buffer.get_resource(), static_cast<vk::DeviceSize>(offset), static_cast<vk::IndexType>(index_type));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::bind_input(ImageViewType const &image_view, uint32_t set, uint32_t binding, uint32_t array_element)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		resource_binding_state.bind_input(image_view, set, binding, array_element);
	}
	else
	{
		resource_binding_state.bind_input(reinterpret_cast<vkb::core::HPPImageView const &>(image_view), set, binding, array_element);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::bind_lighting(LightingStateType &lighting_state, uint32_t set, uint32_t binding)
{
	bind_buffer(
	    lighting_state.light_buffer.get_buffer(), lighting_state.light_buffer.get_offset(), lighting_state.light_buffer.get_size(), set, binding, 0);

	set_specialization_constant(0, to_u32(lighting_state.directional_lights.size()));
	set_specialization_constant(1, to_u32(lighting_state.point_lights.size()));
	set_specialization_constant(2, to_u32(lighting_state.spot_lights.size()));
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::bind_pipeline_layout(PipelineLayoutType &pipeline_layout)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		pipeline_state.set_pipeline_layout(pipeline_layout);
	}
	else
	{
		pipeline_state.set_pipeline_layout(reinterpret_cast<vkb::core::HPPPipelineLayout &>(pipeline_layout));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::bind_vertex_buffers(uint32_t                                                     first_binding,
                                                            std::vector<std::reference_wrapper<const BufferType>> const &buffers,
                                                            std::vector<DeviceSizeType> const                           &offsets)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		bind_vertex_buffers_impl(first_binding, buffers, offsets);
	}
	else
	{
		bind_vertex_buffers_impl(first_binding,
		                         reinterpret_cast<std::vector<std::reference_wrapper<vkb::core::HPPBuffer const>> const &>(buffers),
		                         reinterpret_cast<std::vector<vk::DeviceSize> const &>(offsets));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::bind_vertex_buffers_impl(uint32_t                                                               first_binding,
                                                                 std::vector<std::reference_wrapper<const vkb::core::HPPBuffer>> const &buffers,
                                                                 std::vector<vk::DeviceSize> const                                     &offsets)
{
	std::vector<vk::Buffer> buffer_handles(buffers.size(), nullptr);
	std::transform(buffers.begin(), buffers.end(), buffer_handles.begin(), [](auto const &buffer) { return buffer.get().get_handle(); });
	this->get_resource().bindVertexBuffers(first_binding, buffer_handles, offsets);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::blit_image(ImageType const &src_img, ImageType const &dst_img, std::vector<ImageBlitType> const &regions)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().blitImage(src_img.get_handle(),
		                             vk::ImageLayout::eTransferSrcOptimal,
		                             dst_img.get_handle(),
		                             vk::ImageLayout::eTransferDstOptimal,
		                             regions,
		                             vk::Filter::eNearest);
	}
	else
	{
		this->get_resource().blitImage(src_img.get_resource(),
		                               vk::ImageLayout::eTransferSrcOptimal,
		                               dst_img.get_resource(),
		                               vk::ImageLayout::eTransferDstOptimal,
		                               reinterpret_cast<std::vector<vk::ImageBlit> const &>(regions),
		                               vk::Filter::eNearest);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::buffer_memory_barrier(BufferType const              &buffer,
                                                              DeviceSizeType                 offset,
                                                              DeviceSizeType                 size,
                                                              BufferMemoryBarrierType const &memory_barrier)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		buffer_memory_barrier_impl(buffer, offset, size, memory_barrier);
	}
	else
	{
		buffer_memory_barrier_impl(reinterpret_cast<vkb::core::HPPBuffer const &>(buffer),
		                           static_cast<vk::DeviceSize>(offset),
		                           static_cast<vk::DeviceSize>(size),
		                           reinterpret_cast<vkb::common::HPPBufferMemoryBarrier const &>(memory_barrier));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::buffer_memory_barrier_impl(vkb::core::HPPBuffer const                &buffer,
                                                                   vk::DeviceSize                             offset,
                                                                   vk::DeviceSize                             size,
                                                                   vkb::common::HPPBufferMemoryBarrier const &memory_barrier)
{
	vk::BufferMemoryBarrier buffer_memory_barrier(
	    memory_barrier.src_access_mask, memory_barrier.dst_access_mask, {}, {}, buffer.get_handle(), offset, size);

	this->get_resource().pipelineBarrier(memory_barrier.src_stage_mask, memory_barrier.dst_stage_mask, {}, {}, buffer_memory_barrier, {});
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::clear(ClearAttachmentType const &attachment, ClearRectType const &rect)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().clearAttachments(attachment, rect);
	}
	else
	{
		this->get_resource().clearAttachments(reinterpret_cast<vk::ClearAttachment const &>(attachment), reinterpret_cast<vk::ClearRect const &>(rect));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::copy_buffer(BufferType const &src_buffer, BufferType const &dst_buffer, DeviceSizeType size)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		copy_buffer_impl(src_buffer, dst_buffer, size);
	}
	else
	{
		copy_buffer_impl(reinterpret_cast<vkb::core::HPPBuffer const &>(src_buffer),
		                 reinterpret_cast<vkb::core::HPPBuffer const &>(dst_buffer),
		                 static_cast<vk::DeviceSize>(size));
	}
}

template <vkb::BindingType bindingType>
inline void
    CommandBuffer<bindingType>::copy_buffer_impl(vkb::core::HPPBuffer const &src_buffer, vkb::core::HPPBuffer const &dst_buffer, vk::DeviceSize size)
{
	vk::BufferCopy copy_region({}, {}, size);
	this->get_resource().copyBuffer(src_buffer.get_handle(), dst_buffer.get_handle(), copy_region);
}

template <vkb::BindingType bindingType>
inline void
    CommandBuffer<bindingType>::copy_buffer_to_image(BufferType const &buffer, ImageType const &image, std::vector<BufferImageCopyType> const &regions)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().copyBufferToImage(buffer.get_handle(), image.get_handle(), vk::ImageLayout::eTransferDstOptimal, regions);
	}
	else
	{
		this->get_resource().copyBufferToImage(buffer.get_resource(),
		                                       image.get_resource(),
		                                       vk::ImageLayout::eTransferDstOptimal,
		                                       reinterpret_cast<std::vector<vk::BufferImageCopy> const &>(regions));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::copy_image(ImageType const &src_img, ImageType const &dst_img, std::vector<ImageCopyType> const &regions)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().copyImage(
		    src_img.get_handle(), vk::ImageLayout::eTransferSrcOptimal, dst_img.get_handle(), vk::ImageLayout::eTransferDstOptimal, regions);
	}
	else
	{
		this->get_resource().copyImage(src_img.get_resource(),
		                               vk::ImageLayout::eTransferSrcOptimal,
		                               dst_img.get_resource(),
		                               vk::ImageLayout::eTransferDstOptimal,
		                               reinterpret_cast<std::vector<vk::ImageCopy> const &>(regions));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::copy_image_to_buffer(ImageType const                        &image,
                                                             ImageLayoutType                         image_layout,
                                                             BufferType const                       &buffer,
                                                             std::vector<BufferImageCopyType> const &regions)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().copyImageToBuffer(image.get_handle(), image_layout, buffer.get_handle(), regions);
	}
	else
	{
		this->get_resource().copyImageToBuffer(image.get_resource(),
		                                       static_cast<vk::ImageLayout>(image_layout),
		                                       buffer.get_resource(),
		                                       reinterpret_cast<std::vector<vk::BufferImageCopy> const &>(regions));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
	flush(vk::PipelineBindPoint::eCompute);
	this->get_resource().dispatch(group_count_x, group_count_y, group_count_z);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::dispatch_indirect(BufferType const &buffer, DeviceSizeType offset)
{
	flush(vk::PipelineBindPoint::eCompute);
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().dispatchIndirect(buffer.get_handle(), offset);
	}
	else
	{
		this->get_resource().dispatchIndirect(buffer.get_resource(), static_cast<vk::DeviceSize>(offset));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
	flush(vk::PipelineBindPoint::eGraphics);
	this->get_resource().draw(vertex_count, instance_count, first_vertex, first_instance);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::draw_indexed(
    uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
{
	flush(vk::PipelineBindPoint::eGraphics);
	this->get_resource().drawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::draw_indexed_indirect(BufferType const &buffer, DeviceSizeType offset, uint32_t draw_count, uint32_t stride)
{
	flush(vk::PipelineBindPoint::eGraphics);
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().drawIndexedIndirect(buffer.get_handle(), offset, draw_count, stride);
	}
	else
	{
		this->get_resource().drawIndexedIndirect(buffer.get_resource(), static_cast<vk::DeviceSize>(offset), draw_count, stride);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::end()
{
	this->get_resource().end();
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::end_query(QueryPoolType const &query_pool, uint32_t query)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().endQuery(query_pool.get_handle(), query);
	}
	else
	{
		this->get_resource().endQuery(static_cast<vk::QueryPool>(query_pool.get_handle()), query);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::end_render_pass()
{
	this->get_resource().endRenderPass();
}

template <vkb::BindingType bindingType>
inline typename CommandBuffer<bindingType>::CommandBufferLevelType CommandBuffer<bindingType>::get_level() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return level;
	}
	else
	{
		return static_cast<VkCommandBufferLevel>(level);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::execute_commands(vkb::core::CommandBuffer<bindingType> &secondary_command_buffer)
{
	this->get_resource().executeCommands(secondary_command_buffer.get_resource());
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::execute_commands(std::vector<vkb::core::CommandBuffer<bindingType> *> &secondary_command_buffers)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		execute_commands_impl(secondary_command_buffers);
	}
	else
	{
		execute_commands_impl(reinterpret_cast<std::vector<vkb::core::CommandBuffer<vkb::BindingType::Cpp> *> &>(secondary_command_buffers));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::execute_commands_impl(std::vector<vkb::core::CommandBuffer<vkb::BindingType::Cpp> *> &secondary_command_buffers)
{
	std::vector<vk::CommandBuffer> sec_cmd_buf_handles(secondary_command_buffers.size(), nullptr);
	std::transform(secondary_command_buffers.begin(),
	               secondary_command_buffers.end(),
	               sec_cmd_buf_handles.begin(),
	               [](const vkb::core::CommandBuffer<vkb::BindingType::Cpp> *sec_cmd_buf) { return sec_cmd_buf->get_handle(); });
	this->get_resource().executeCommands(sec_cmd_buf_handles);
}

template <vkb::BindingType bindingType>
inline typename vkb::core::CommandBuffer<bindingType>::RenderPassType &
    CommandBuffer<bindingType>::get_render_pass(RenderTargetType const                          &render_target,
                                                std::vector<LoadStoreInfoType> const            &load_store_infos,
                                                std::vector<std::unique_ptr<SubpassType>> const &subpasses)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return get_render_pass_impl(this->get_device(), render_target, load_store_infos, subpasses);
	}
	else
	{
		return reinterpret_cast<vkb::RenderPass &>(
		    get_render_pass_impl(reinterpret_cast<vkb::core::HPPDevice &>(this->get_device()),
		                         reinterpret_cast<vkb::rendering::HPPRenderTarget const &>(render_target),
		                         reinterpret_cast<std::vector<vkb::common::HPPLoadStoreInfo> const &>(load_store_infos),
		                         reinterpret_cast<std::vector<std::unique_ptr<vkb::rendering::HPPSubpass>> const &>(subpasses)));
	}
}

template <vkb::BindingType bindingType>
inline vkb::core::HPPRenderPass &
    CommandBuffer<bindingType>::get_render_pass_impl(vkb::core::HPPDevice                                           &device,
                                                     vkb::rendering::HPPRenderTarget const                          &render_target,
                                                     std::vector<vkb::common::HPPLoadStoreInfo> const               &load_store_infos,
                                                     std::vector<std::unique_ptr<vkb::rendering::HPPSubpass>> const &subpasses)
{
	// Create render pass
	assert(subpasses.size() > 0 && "Cannot create a render pass without any subpass");

	std::vector<vkb::core::HPPSubpassInfo> subpass_infos(subpasses.size());
	auto                                   subpass_info_it = subpass_infos.begin();
	for (auto &subpass : subpasses)
	{
		subpass_info_it->input_attachments                = subpass->get_input_attachments();
		subpass_info_it->output_attachments               = subpass->get_output_attachments();
		subpass_info_it->color_resolve_attachments        = subpass->get_color_resolve_attachments();
		subpass_info_it->disable_depth_stencil_attachment = subpass->get_disable_depth_stencil_attachment();
		subpass_info_it->depth_stencil_resolve_mode       = subpass->get_depth_stencil_resolve_mode();
		subpass_info_it->depth_stencil_resolve_attachment = subpass->get_depth_stencil_resolve_attachment();
		subpass_info_it->debug_name                       = subpass->get_debug_name();

		++subpass_info_it;
	}

	return device.get_resource_cache().request_render_pass(render_target.get_attachments(), load_store_infos, subpass_infos);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::image_memory_barrier(ImageViewType const &image_view, ImageMemoryBarrierType const &memory_barrier) const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		image_memory_barrier_impl(image_view, memory_barrier);
	}
	else
	{
		image_memory_barrier_impl(reinterpret_cast<vkb::core::HPPImageView const &>(image_view),
		                          reinterpret_cast<vkb::common::HPPImageMemoryBarrier const &>(memory_barrier));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::image_memory_barrier_impl(vkb::core::HPPImageView const            &image_view,
                                                                  vkb::common::HPPImageMemoryBarrier const &memory_barrier) const
{
	// Adjust barrier's subresource range for depth images
	auto subresource_range = image_view.get_subresource_range();
	auto format            = image_view.get_format();
	if (vkb::common::is_depth_only_format(format))
	{
		subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
	}
	else if (vkb::common::is_depth_stencil_format(format))
	{
		subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
	}

	vk::ImageMemoryBarrier image_memory_barrier(memory_barrier.src_access_mask,
	                                            memory_barrier.dst_access_mask,
	                                            memory_barrier.old_layout,
	                                            memory_barrier.new_layout,
	                                            memory_barrier.old_queue_family,
	                                            memory_barrier.new_queue_family,
	                                            image_view.get_image().get_handle(),
	                                            subresource_range);

	vk::PipelineStageFlags src_stage_mask = memory_barrier.src_stage_mask;
	vk::PipelineStageFlags dst_stage_mask = memory_barrier.dst_stage_mask;

	this->get_resource().pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, {}, image_memory_barrier);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::next_subpass()
{
	// Increment subpass index
	pipeline_state.set_subpass_index(pipeline_state.get_subpass_index() + 1);

	// Update blend state attachments
	auto blend_state = pipeline_state.get_color_blend_state();
	blend_state.attachments.resize(current_render_pass.render_pass->get_color_output_count(pipeline_state.get_subpass_index()));
	pipeline_state.set_color_blend_state(blend_state);

	// Reset descriptor sets
	resource_binding_state.reset();
	descriptor_set_layout_binding_state.clear();

	// Clear stored push constants
	stored_push_constants.clear();

	this->get_resource().nextSubpass(vk::SubpassContents::eInline);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::push_constants(const std::vector<uint8_t> &values)
{
	uint32_t push_constant_size = to_u32(stored_push_constants.size() + values.size());

	if (push_constant_size > max_push_constants_size)
	{
		LOGE("Push constant limit of {} exceeded (pushing {} bytes for a total of {} bytes)", max_push_constants_size, values.size(), push_constant_size);
		throw std::runtime_error("Push constant limit exceeded.");
	}
	else
	{
		stored_push_constants.insert(stored_push_constants.end(), values.begin(), values.end());
	}
}

template <vkb::BindingType bindingType>
template <typename T>
inline void CommandBuffer<bindingType>::push_constants(const T &value)
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

template <vkb::BindingType bindingType>
inline typename CommandBuffer<bindingType>::ResultType CommandBuffer<bindingType>::reset(ResetMode reset_mode)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		assert(reset_mode == command_pool.get_reset_mode() && "Command buffer reset mode must match the one used by the pool to allocate it");
		return reset_impl(reset_mode);
	}
	else
	{
		assert(static_cast<vkb::core::CommandBuffer<vkb::BindingType::Cpp>::ResetMode>(reset_mode) == command_pool.get_reset_mode() &&
		       "Command buffer reset mode must match the one used by the pool to allocate it");
		return static_cast<VkResult>(reset_impl(reset_mode));
	}
}

template <vkb::BindingType bindingType>
inline vk::Result CommandBuffer<bindingType>::reset_impl(vkb::core::CommandBuffer<bindingType>::ResetMode reset_mode)
{
	if (reset_mode == ResetMode::ResetIndividually)
	{
		this->get_resource().reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	}

	return vk::Result::eSuccess;
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::reset_query_pool(QueryPoolType const &query_pool, uint32_t first_query, uint32_t query_count)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().resetQueryPool(query_pool.get_handle(), first_query, query_count);
	}
	else
	{
		this->get_resource().resetQueryPool(static_cast<vk::QueryPool>(query_pool.get_handle()), first_query, query_count);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::resolve_image(ImageType const &src_img, ImageType const &dst_img, std::vector<ImageResolveType> const &regions)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().resolveImage(
		    src_img.get_handle(), vk::ImageLayout::eTransferSrcOptimal, dst_img.get_handle(), vk::ImageLayout::eTransferDstOptimal, regions);
	}
	else
	{
		this->get_resource().resolveImage(src_img.get_resource(),
		                                  vk::ImageLayout::eTransferSrcOptimal,
		                                  dst_img.get_resource(),
		                                  vk::ImageLayout::eTransferDstOptimal,
		                                  reinterpret_cast<std::vector<vk::ImageResolve> const &>(regions));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_blend_constants(std::array<float, 4> const &blend_constants)
{
	this->get_resource().setBlendConstants(blend_constants.data());
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_color_blend_state(ColorBlendStateType const &state_info)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		pipeline_state.set_color_blend_state(state_info);
	}
	else
	{
		pipeline_state.set_color_blend_state(reinterpret_cast<vkb::rendering::HPPColorBlendState const &>(state_info));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_depth_bias(float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor)
{
	this->get_resource().setDepthBias(depth_bias_constant_factor, depth_bias_clamp, depth_bias_slope_factor);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_depth_bounds(float min_depth_bounds, float max_depth_bounds)
{
	this->get_handle().setDepthBounds(min_depth_bounds, max_depth_bounds);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_depth_stencil_state(DepthStencilStateType const &state_info)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		pipeline_state.set_depth_stencil_state(state_info);
	}
	else
	{
		pipeline_state.set_depth_stencil_state(reinterpret_cast<vkb::rendering::HPPDepthStencilState const &>(state_info));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_input_assembly_state(InputAssemblyStateType const &state_info)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		pipeline_state.set_input_assembly_state(state_info);
	}
	else
	{
		pipeline_state.set_input_assembly_state(reinterpret_cast<vkb::rendering::HPPInputAssemblyState const &>(state_info));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_line_width(float line_width)
{
	this->get_resource().setLineWidth(line_width);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_multisample_state(MultisampleStateType const &state_info)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		pipeline_state.set_multisample_state(state_info);
	}
	else
	{
		pipeline_state.set_multisample_state(reinterpret_cast<vkb::rendering::HPPMultisampleState const &>(state_info));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_rasterization_state(RasterizationStateType const &state_info)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		pipeline_state.set_rasterization_state(state_info);
	}
	else
	{
		pipeline_state.set_rasterization_state(reinterpret_cast<vkb::rendering::HPPRasterizationState const &>(state_info));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_scissor(uint32_t first_scissor, std::vector<Rect2DType> const &scissors)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_resource().setScissor(first_scissor, scissors);
	}
	else
	{
		this->get_resource().setScissor(first_scissor, reinterpret_cast<std::vector<vk::Rect2D> const &>(scissors));
	}
}

template <vkb::BindingType bindingType>
template <class T>
inline void CommandBuffer<bindingType>::set_specialization_constant(uint32_t constant_id, T const &data)
{
	if constexpr (std::is_same<T, bool>::value)
	{
		set_specialization_constant(constant_id, to_bytes(to_u32(data)));
	}
	else
	{
		set_specialization_constant(constant_id, to_bytes(data));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_specialization_constant(uint32_t constant_id, std::vector<uint8_t> const &data)
{
	pipeline_state.set_specialization_constant(constant_id, data);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_update_after_bind(bool update_after_bind_)
{
	update_after_bind = update_after_bind_;
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_vertex_input_state(VertexInputStateType const &state_info)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		pipeline_state.set_vertex_input_state(state_info);
	}
	else
	{
		pipeline_state.set_vertex_input_state(reinterpret_cast<vkb::rendering::HPPVertexInputState const &>(state_info));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_viewport(uint32_t first_viewport, std::vector<ViewportType> const &viewports)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().setViewport(first_viewport, viewports);
	}
	else
	{
		this->get_resource().setViewport(first_viewport, reinterpret_cast<std::vector<vk::Viewport> const &>(viewports));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::set_viewport_state(ViewportStateType const &state_info)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		pipeline_state.set_viewport_state(state_info);
	}
	else
	{
		pipeline_state.set_viewport_state(reinterpret_cast<vkb::rendering::HPPViewportState const &>(state_info));
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::update_buffer(BufferType const &buffer, DeviceSizeType offset, std::vector<uint8_t> const &data)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().updateBuffer<uint8_t>(buffer.get_handle(), offset, data);
	}
	else
	{
		this->get_resource().updateBuffer<uint8_t>(buffer.get_resource(), static_cast<vk::DeviceSize>(offset), data);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::write_timestamp(PipelineStagFlagBitsType pipeline_stage, QueryPoolType const &query_pool, uint32_t query)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().writeTimestamp(pipeline_stage, query_pool.get_handle(), query);
	}
	else
	{
		this->get_resource().writeTimestamp(static_cast<vk::PipelineStageFlagBits>(pipeline_stage), query_pool.get_handle(), query);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::flush(vk::PipelineBindPoint pipeline_bind_point)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		flush_impl(this->get_device(), pipeline_bind_point);
	}
	else
	{
		flush_impl(reinterpret_cast<vkb::core::HPPDevice &>(this->get_device()), pipeline_bind_point);
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::flush_impl(vkb::core::HPPDevice &device, vk::PipelineBindPoint pipeline_bind_point)
{
	flush_pipeline_state_impl(device, pipeline_bind_point);
	flush_push_constants();
	flush_descriptor_state_impl(pipeline_bind_point);
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::flush_descriptor_state_impl(vk::PipelineBindPoint pipeline_bind_point)
{
	assert(command_pool.get_render_frame() && "The command pool must be associated to a render frame");

	const auto &pipeline_layout = pipeline_state.get_pipeline_layout();

	std::unordered_set<uint32_t> update_descriptor_sets;

	// Iterate over the shader sets to check if they have already been bound
	// If they have, add the set so that the command buffer later updates it
	for (auto &set_it : pipeline_layout.get_shader_sets())
	{
		uint32_t descriptor_set_id = set_it.first;

		auto descriptor_set_layout_it = descriptor_set_layout_binding_state.find(descriptor_set_id);

		if (descriptor_set_layout_it != descriptor_set_layout_binding_state.end())
		{
			if (descriptor_set_layout_it->second->get_handle() != pipeline_layout.get_descriptor_set_layout(descriptor_set_id).get_handle())
			{
				update_descriptor_sets.emplace(descriptor_set_id);
			}
		}
	}

	// Validate that the bound descriptor set layouts exist in the pipeline layout
	for (auto set_it = descriptor_set_layout_binding_state.begin(); set_it != descriptor_set_layout_binding_state.end();)
	{
		if (!pipeline_layout.has_descriptor_set_layout(set_it->first))
		{
			set_it = descriptor_set_layout_binding_state.erase(set_it);
		}
		else
		{
			++set_it;
		}
	}

	// Check if a descriptor set needs to be created
	if (resource_binding_state.is_dirty() || !update_descriptor_sets.empty())
	{
		resource_binding_state.clear_dirty();

		// Iterate over all of the resource sets bound by the command buffer
		for (auto &resource_set_it : resource_binding_state.get_resource_sets())
		{
			uint32_t descriptor_set_id = resource_set_it.first;
			auto    &resource_set      = resource_set_it.second;

			// Don't update resource set if it's not in the update list OR its state hasn't changed
			if (!resource_set.is_dirty() && (update_descriptor_sets.find(descriptor_set_id) == update_descriptor_sets.end()))
			{
				continue;
			}

			// Clear dirty flag for resource set
			resource_binding_state.clear_dirty(descriptor_set_id);

			// Skip resource set if a descriptor set layout doesn't exist for it
			if (!pipeline_layout.has_descriptor_set_layout(descriptor_set_id))
			{
				continue;
			}

			auto &descriptor_set_layout = pipeline_layout.get_descriptor_set_layout(descriptor_set_id);

			// Make descriptor set layout bound for current set
			descriptor_set_layout_binding_state[descriptor_set_id] = &descriptor_set_layout;

			BindingMap<vk::DescriptorBufferInfo> buffer_infos;
			BindingMap<vk::DescriptorImageInfo>  image_infos;

			std::vector<uint32_t> dynamic_offsets;

			// Iterate over all resource bindings
			for (auto &binding_it : resource_set.get_resource_bindings())
			{
				auto  binding_index     = binding_it.first;
				auto &binding_resources = binding_it.second;

				// Check if binding exists in the pipeline layout
				if (auto binding_info = descriptor_set_layout.get_layout_binding(binding_index))
				{
					// Iterate over all binding resources
					for (auto &element_it : binding_resources)
					{
						auto  array_element = element_it.first;
						auto &resource_info = element_it.second;

						// Pointer references
						auto &buffer     = resource_info.buffer;
						auto &sampler    = resource_info.sampler;
						auto &image_view = resource_info.image_view;

						// Get buffer info
						if (buffer != nullptr && vkb::common::is_buffer_descriptor_type(binding_info->descriptorType))
						{
							vk::DescriptorBufferInfo buffer_info(resource_info.buffer->get_handle(), resource_info.offset, resource_info.range);

							if (vkb::common::is_dynamic_buffer_descriptor_type(binding_info->descriptorType))
							{
								dynamic_offsets.push_back(to_u32(buffer_info.offset));
								buffer_info.offset = 0;
							}

							buffer_infos[binding_index][array_element] = buffer_info;
						}

						// Get image info
						else if (image_view != nullptr || sampler != nullptr)
						{
							// Can be null for input attachments
							vk::DescriptorImageInfo image_info(sampler ? sampler->get_handle() : nullptr, image_view->get_handle());

							if (image_view != nullptr)
							{
								// Add image layout info based on descriptor type
								switch (binding_info->descriptorType)
								{
									case vk::DescriptorType::eCombinedImageSampler:
										image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
										break;
									case vk::DescriptorType::eInputAttachment:
										image_info.imageLayout = vkb::common::is_depth_format(image_view->get_format()) ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
										break;
									case vk::DescriptorType::eStorageImage:
										image_info.imageLayout = vk::ImageLayout::eGeneral;
										break;
									default:
										continue;
								}
							}

							image_infos[binding_index][array_element] = image_info;
						}
					}

					assert((!update_after_bind || (buffer_infos.count(binding_index) > 0 || (image_infos.count(binding_index) > 0))) &&
					       "binding index with no buffer or image infos can't be checked for adding to bindings_to_update");
				}
			}

			vk::DescriptorSet descriptor_set_handle = command_pool.get_render_frame()->request_descriptor_set(
			    descriptor_set_layout, buffer_infos, image_infos, update_after_bind, command_pool.get_thread_index());

			// Bind descriptor set
			this->get_resource().bindDescriptorSets(pipeline_bind_point, pipeline_layout.get_handle(), descriptor_set_id, descriptor_set_handle, dynamic_offsets);
		}
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::flush_pipeline_state_impl(vkb::core::HPPDevice &device, vk::PipelineBindPoint pipeline_bind_point)
{
	// Create a new pipeline only if the graphics state changed
	if (!pipeline_state.is_dirty())
	{
		return;
	}

	pipeline_state.clear_dirty();

	// Create and bind pipeline
	if (pipeline_bind_point == vk::PipelineBindPoint::eGraphics)
	{
		pipeline_state.set_render_pass(*current_render_pass.render_pass);
		auto &pipeline = device.get_resource_cache().request_graphics_pipeline(pipeline_state);

		this->get_resource().bindPipeline(pipeline_bind_point, pipeline.get_handle());
	}
	else if (pipeline_bind_point == vk::PipelineBindPoint::eCompute)
	{
		auto &pipeline = device.get_resource_cache().request_compute_pipeline(pipeline_state);

		this->get_resource().bindPipeline(pipeline_bind_point, pipeline.get_handle());
	}
	else
	{
		throw "Only graphics and compute pipeline bind points are supported now";
	}
}

template <vkb::BindingType bindingType>
inline void CommandBuffer<bindingType>::flush_push_constants()
{
	if (stored_push_constants.empty())
	{
		return;
	}

	auto const &pipeline_layout = pipeline_state.get_pipeline_layout();

	vk::ShaderStageFlags shader_stage = pipeline_layout.get_push_constant_range_stage(to_u32(stored_push_constants.size()));

	if (shader_stage)
	{
		this->get_resource().pushConstants<uint8_t>(pipeline_layout.get_handle(), shader_stage, 0, stored_push_constants);
	}
	else
	{
		LOGW("Push constant range [{}, {}] not found", 0, stored_push_constants.size());
	}

	stored_push_constants.clear();
}

template <vkb::BindingType bindingType>
inline typename CommandBuffer<bindingType>::RenderPassBinding const &CommandBuffer<bindingType>::get_current_render_pass() const
{
	return current_render_pass;
}

template <vkb::BindingType bindingType>
inline uint32_t CommandBuffer<bindingType>::get_current_subpass_index() const
{
	return pipeline_state.get_subpass_index();
}

template <vkb::BindingType bindingType>
inline bool CommandBuffer<bindingType>::is_render_size_optimal(const vk::Extent2D &framebuffer_extent, const vk::Rect2D &render_area)
{
	auto render_area_granularity = current_render_pass.render_pass->get_render_area_granularity();

	return ((render_area.offset.x % render_area_granularity.width == 0) && (render_area.offset.y % render_area_granularity.height == 0) &&
	        ((render_area.extent.width % render_area_granularity.width == 0) ||
	         (render_area.offset.x + render_area.extent.width == framebuffer_extent.width)) &&
	        ((render_area.extent.height % render_area_granularity.height == 0) ||
	         (render_area.offset.y + render_area.extent.height == framebuffer_extent.height)));
}
}        // namespace core
}        // namespace vkb
