/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "command_buffer.h"

#include "command_pool.h"
#include "common/error.h"
#include "device.h"
#include "rendering/render_frame.h"
#include "rendering/subpass.h"

#include <glm/gtc/type_ptr.hpp>

namespace vkb
{
CommandBuffer::CommandBuffer(CommandPool &command_pool, VkCommandBufferLevel level) :
    VulkanResource{VK_NULL_HANDLE, &command_pool.get_device()},
    command_pool{command_pool},
    max_push_constants_size{get_device().get_gpu().get_properties().limits.maxPushConstantsSize},
    level{level}
{
	VkCommandBufferAllocateInfo allocate_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};

	allocate_info.commandPool        = command_pool.get_handle();
	allocate_info.commandBufferCount = 1;
	allocate_info.level              = level;

	VkResult result = vkAllocateCommandBuffers(get_device().get_handle(), &allocate_info, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Failed to allocate command buffer"};
	}
}

CommandBuffer::~CommandBuffer()
{
	// Destroy command buffer
	if (handle != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(command_pool.get_device().get_handle(), command_pool.get_handle(), 1, &handle);
	}
}

CommandBuffer::CommandBuffer(CommandBuffer &&other) :
    VulkanResource(std::move(other)),
    level(other.level),
    command_pool(other.command_pool),
    current_render_pass(std::exchange(other.current_render_pass, {})),
    pipeline_state(std::exchange(other.pipeline_state, {})),
    resource_binding_state(std::exchange(other.resource_binding_state, {})),
    stored_push_constants(std::exchange(other.stored_push_constants, {})),
    max_push_constants_size(std::exchange(other.max_push_constants_size, {})),
    last_framebuffer_extent(std::exchange(other.last_framebuffer_extent, {})),
    last_render_area_extent(std::exchange(other.last_render_area_extent, {})),
    update_after_bind(std::exchange(other.update_after_bind, {})),
    descriptor_set_layout_binding_state(std::exchange(other.descriptor_set_layout_binding_state, {}))
{}

void CommandBuffer::clear(VkClearAttachment attachment, VkClearRect rect)
{
	vkCmdClearAttachments(handle, 1, &attachment, 1, &rect);
}

VkResult CommandBuffer::begin(VkCommandBufferUsageFlags flags, CommandBuffer *primary_cmd_buf)
{
	if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
	{
		assert(primary_cmd_buf && "A primary command buffer pointer must be provided when calling begin from a secondary one");
		auto render_pass_binding = primary_cmd_buf->get_current_render_pass();

		return begin(flags, render_pass_binding.render_pass, render_pass_binding.framebuffer, primary_cmd_buf->get_current_subpass_index());
	}

	return begin(flags, nullptr, nullptr, 0);
}

VkResult CommandBuffer::begin(VkCommandBufferUsageFlags flags, const RenderPass *render_pass, const Framebuffer *framebuffer, uint32_t subpass_index)
{
	// Reset state
	pipeline_state.reset();
	resource_binding_state.reset();
	descriptor_set_layout_binding_state.clear();
	stored_push_constants.clear();

	VkCommandBufferBeginInfo       begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	VkCommandBufferInheritanceInfo inheritance = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
	begin_info.flags                           = flags;

	if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
	{
		assert((render_pass && framebuffer) && "Render pass and framebuffer must be provided when calling begin from a secondary one");

		current_render_pass.render_pass = render_pass;
		current_render_pass.framebuffer = framebuffer;

		inheritance.renderPass  = current_render_pass.render_pass->get_handle();
		inheritance.framebuffer = current_render_pass.framebuffer->get_handle();
		inheritance.subpass     = subpass_index;

		begin_info.pInheritanceInfo = &inheritance;
	}

	return vkBeginCommandBuffer(get_handle(), &begin_info);
}

VkResult CommandBuffer::end()
{
	vkEndCommandBuffer(get_handle());

	return VK_SUCCESS;
}

void CommandBuffer::flush(VkPipelineBindPoint pipeline_bind_point)
{
	flush_pipeline_state(pipeline_bind_point);

	flush_push_constants();

	flush_descriptor_state(pipeline_bind_point);
}

void CommandBuffer::begin_render_pass(const RenderTarget &render_target, const std::vector<LoadStoreInfo> &load_store_infos, const std::vector<VkClearValue> &clear_values, const std::vector<std::unique_ptr<Subpass>> &subpasses, VkSubpassContents contents)
{
	// Reset state
	pipeline_state.reset();
	resource_binding_state.reset();
	descriptor_set_layout_binding_state.clear();

	auto &render_pass = get_render_pass(render_target, load_store_infos, subpasses);
	auto &framebuffer = get_device().get_resource_cache().request_framebuffer(render_target, render_pass);

	begin_render_pass(render_target, render_pass, framebuffer, clear_values, contents);
}

void CommandBuffer::begin_render_pass(const RenderTarget &render_target, const RenderPass &render_pass, const Framebuffer &framebuffer, const std::vector<VkClearValue> &clear_values, VkSubpassContents contents)
{
	current_render_pass.render_pass = &render_pass;
	current_render_pass.framebuffer = &framebuffer;

	// Begin render pass
	VkRenderPassBeginInfo begin_info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	begin_info.renderPass        = current_render_pass.render_pass->get_handle();
	begin_info.framebuffer       = current_render_pass.framebuffer->get_handle();
	begin_info.renderArea.extent = render_target.get_extent();
	begin_info.clearValueCount   = to_u32(clear_values.size());
	begin_info.pClearValues      = clear_values.data();

	const auto &framebuffer_extent = current_render_pass.framebuffer->get_extent();

	// Test the requested render area to confirm that it is optimal and could not cause a performance reduction
	if (!is_render_size_optimal(framebuffer_extent, begin_info.renderArea))
	{
		// Only prints the warning if the framebuffer or render area are different since the last time the render size was not optimal
		if (framebuffer_extent.width != last_framebuffer_extent.width || framebuffer_extent.height != last_framebuffer_extent.height ||
		    begin_info.renderArea.extent.width != last_render_area_extent.width || begin_info.renderArea.extent.height != last_render_area_extent.height)
		{
			LOGW("Render target extent is not an optimal size, this may result in reduced performance.");
		}

		last_framebuffer_extent = current_render_pass.framebuffer->get_extent();
		last_render_area_extent = begin_info.renderArea.extent;
	}

	vkCmdBeginRenderPass(get_handle(), &begin_info, contents);

	// Update blend state attachments for first subpass
	auto blend_state = pipeline_state.get_color_blend_state();
	blend_state.attachments.resize(current_render_pass.render_pass->get_color_output_count(pipeline_state.get_subpass_index()));
	pipeline_state.set_color_blend_state(blend_state);
}

void CommandBuffer::next_subpass()
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

	vkCmdNextSubpass(get_handle(), VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::execute_commands(CommandBuffer &secondary_command_buffer)
{
	vkCmdExecuteCommands(get_handle(), 1, &secondary_command_buffer.get_handle());
}

void CommandBuffer::execute_commands(std::vector<CommandBuffer *> &secondary_command_buffers)
{
	std::vector<VkCommandBuffer> sec_cmd_buf_handles(secondary_command_buffers.size(), VK_NULL_HANDLE);
	std::transform(secondary_command_buffers.begin(), secondary_command_buffers.end(), sec_cmd_buf_handles.begin(),
	               [](const vkb::CommandBuffer *sec_cmd_buf) { return sec_cmd_buf->get_handle(); });
	vkCmdExecuteCommands(get_handle(), to_u32(sec_cmd_buf_handles.size()), sec_cmd_buf_handles.data());
}

void CommandBuffer::end_render_pass()
{
	vkCmdEndRenderPass(get_handle());
}

void CommandBuffer::bind_pipeline_layout(PipelineLayout &pipeline_layout)
{
	pipeline_state.set_pipeline_layout(pipeline_layout);
}

void CommandBuffer::set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t> &data)
{
	pipeline_state.set_specialization_constant(constant_id, data);
}

void CommandBuffer::push_constants(const std::vector<uint8_t> &values)
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

void CommandBuffer::bind_buffer(const core::Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element)
{
	resource_binding_state.bind_buffer(buffer, offset, range, set, binding, array_element);
}

void CommandBuffer::bind_image(const core::ImageView &image_view, const core::Sampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element)
{
	resource_binding_state.bind_image(image_view, sampler, set, binding, array_element);
}

void CommandBuffer::bind_image(const core::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element)
{
	resource_binding_state.bind_image(image_view, set, binding, array_element);
}

void CommandBuffer::bind_input(const core::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element)
{
	resource_binding_state.bind_input(image_view, set, binding, array_element);
}

void CommandBuffer::bind_vertex_buffers(uint32_t first_binding, const std::vector<std::reference_wrapper<const vkb::core::Buffer>> &buffers, const std::vector<VkDeviceSize> &offsets)
{
	std::vector<VkBuffer> buffer_handles(buffers.size(), VK_NULL_HANDLE);
	std::transform(buffers.begin(), buffers.end(), buffer_handles.begin(),
	               [](const core::Buffer &buffer) { return buffer.get_handle(); });
	vkCmdBindVertexBuffers(get_handle(), first_binding, to_u32(buffer_handles.size()), buffer_handles.data(), offsets.data());
}

void CommandBuffer::bind_index_buffer(const core::Buffer &buffer, VkDeviceSize offset, VkIndexType index_type)
{
	vkCmdBindIndexBuffer(get_handle(), buffer.get_handle(), offset, index_type);
}

void CommandBuffer::bind_lighting(LightingState &lighting_state, uint32_t set, uint32_t binding)
{
	bind_buffer(lighting_state.light_buffer.get_buffer(), lighting_state.light_buffer.get_offset(), lighting_state.light_buffer.get_size(), set, binding, 0);

	set_specialization_constant(0, to_u32(lighting_state.directional_lights.size()));
	set_specialization_constant(1, to_u32(lighting_state.point_lights.size()));
	set_specialization_constant(2, to_u32(lighting_state.spot_lights.size()));
}

void CommandBuffer::set_viewport_state(const ViewportState &state_info)
{
	pipeline_state.set_viewport_state(state_info);
}

void CommandBuffer::set_vertex_input_state(const VertexInputState &state_info)
{
	pipeline_state.set_vertex_input_state(state_info);
}

void CommandBuffer::set_input_assembly_state(const InputAssemblyState &state_info)
{
	pipeline_state.set_input_assembly_state(state_info);
}

void CommandBuffer::set_rasterization_state(const RasterizationState &state_info)
{
	pipeline_state.set_rasterization_state(state_info);
}

void CommandBuffer::set_multisample_state(const MultisampleState &state_info)
{
	pipeline_state.set_multisample_state(state_info);
}

void CommandBuffer::set_depth_stencil_state(const DepthStencilState &state_info)
{
	pipeline_state.set_depth_stencil_state(state_info);
}

void CommandBuffer::set_color_blend_state(const ColorBlendState &state_info)
{
	pipeline_state.set_color_blend_state(state_info);
}

void CommandBuffer::set_viewport(uint32_t first_viewport, const std::vector<VkViewport> &viewports)
{
	vkCmdSetViewport(get_handle(), first_viewport, to_u32(viewports.size()), viewports.data());
}

void CommandBuffer::set_scissor(uint32_t first_scissor, const std::vector<VkRect2D> &scissors)
{
	vkCmdSetScissor(get_handle(), first_scissor, to_u32(scissors.size()), scissors.data());
}

void CommandBuffer::set_line_width(float line_width)
{
	vkCmdSetLineWidth(get_handle(), line_width);
}

void CommandBuffer::set_depth_bias(float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor)
{
	vkCmdSetDepthBias(get_handle(), depth_bias_constant_factor, depth_bias_clamp, depth_bias_slope_factor);
}

void CommandBuffer::set_blend_constants(const std::array<float, 4> &blend_constants)
{
	vkCmdSetBlendConstants(get_handle(), blend_constants.data());
}

void CommandBuffer::set_depth_bounds(float min_depth_bounds, float max_depth_bounds)
{
	vkCmdSetDepthBounds(get_handle(), min_depth_bounds, max_depth_bounds);
}

void CommandBuffer::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
	flush(VK_PIPELINE_BIND_POINT_GRAPHICS);

	vkCmdDraw(get_handle(), vertex_count, instance_count, first_vertex, first_instance);
}

void CommandBuffer::draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
{
	flush(VK_PIPELINE_BIND_POINT_GRAPHICS);

	vkCmdDrawIndexed(get_handle(), index_count, instance_count, first_index, vertex_offset, first_instance);
}

void CommandBuffer::draw_indexed_indirect(const core::Buffer &buffer, VkDeviceSize offset, uint32_t draw_count, uint32_t stride)
{
	flush(VK_PIPELINE_BIND_POINT_GRAPHICS);

	vkCmdDrawIndexedIndirect(get_handle(), buffer.get_handle(), offset, draw_count, stride);
}

void CommandBuffer::dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
	flush(VK_PIPELINE_BIND_POINT_COMPUTE);

	vkCmdDispatch(get_handle(), group_count_x, group_count_y, group_count_z);
}

void CommandBuffer::dispatch_indirect(const core::Buffer &buffer, VkDeviceSize offset)
{
	flush(VK_PIPELINE_BIND_POINT_COMPUTE);

	vkCmdDispatchIndirect(get_handle(), buffer.get_handle(), offset);
}

void CommandBuffer::update_buffer(const core::Buffer &buffer, VkDeviceSize offset, const std::vector<uint8_t> &data)
{
	vkCmdUpdateBuffer(get_handle(), buffer.get_handle(), offset, data.size(), data.data());
}

void CommandBuffer::blit_image(const core::Image &src_img, const core::Image &dst_img, const std::vector<VkImageBlit> &regions)
{
	vkCmdBlitImage(get_handle(), src_img.get_handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	               dst_img.get_handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	               to_u32(regions.size()), regions.data(), VK_FILTER_NEAREST);
}

void CommandBuffer::resolve_image(const core::Image &src_img, const core::Image &dst_img, const std::vector<VkImageResolve> &regions)
{
	vkCmdResolveImage(get_handle(), src_img.get_handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                  dst_img.get_handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                  to_u32(regions.size()), regions.data());
}

void CommandBuffer::copy_buffer(const core::Buffer &src_buffer, const core::Buffer &dst_buffer, VkDeviceSize size)
{
	VkBufferCopy copy_region = {};
	copy_region.size         = size;
	vkCmdCopyBuffer(get_handle(), src_buffer.get_handle(), dst_buffer.get_handle(), 1, &copy_region);
}

void CommandBuffer::copy_image(const core::Image &src_img, const core::Image &dst_img, const std::vector<VkImageCopy> &regions)
{
	vkCmdCopyImage(get_handle(), src_img.get_handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	               dst_img.get_handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	               to_u32(regions.size()), regions.data());
}

void CommandBuffer::copy_buffer_to_image(const core::Buffer &buffer, const core::Image &image, const std::vector<VkBufferImageCopy> &regions)
{
	vkCmdCopyBufferToImage(get_handle(), buffer.get_handle(),
	                       image.get_handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                       to_u32(regions.size()), regions.data());
}

void CommandBuffer::copy_image_to_buffer(const core::Image &image, VkImageLayout image_layout, const core::Buffer &buffer, const std::vector<VkBufferImageCopy> &regions)
{
	vkCmdCopyImageToBuffer(get_handle(), image.get_handle(), image_layout,
	                       buffer.get_handle(), to_u32(regions.size()), regions.data());
}

void CommandBuffer::image_memory_barrier(const core::ImageView &image_view, const ImageMemoryBarrier &memory_barrier) const
{
	// Adjust barrier's subresource range for depth images
	auto subresource_range = image_view.get_subresource_range();
	auto format            = image_view.get_format();
	if (is_depth_only_format(format))
	{
		subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (is_depth_stencil_format(format))
	{
		subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	vkb::image_layout_transition(get_handle(),
	                             image_view.get_image().get_handle(),
	                             memory_barrier.src_stage_mask,
	                             memory_barrier.dst_stage_mask,
	                             memory_barrier.src_access_mask,
	                             memory_barrier.dst_access_mask,
	                             memory_barrier.old_layout,
	                             memory_barrier.new_layout,
	                             subresource_range);
}

void CommandBuffer::buffer_memory_barrier(const core::Buffer &buffer, VkDeviceSize offset, VkDeviceSize size, const BufferMemoryBarrier &memory_barrier)
{
	VkBufferMemoryBarrier buffer_memory_barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
	buffer_memory_barrier.srcAccessMask = memory_barrier.src_access_mask;
	buffer_memory_barrier.dstAccessMask = memory_barrier.dst_access_mask;
	buffer_memory_barrier.buffer        = buffer.get_handle();
	buffer_memory_barrier.offset        = offset;
	buffer_memory_barrier.size          = size;

	VkPipelineStageFlags src_stage_mask = memory_barrier.src_stage_mask;
	VkPipelineStageFlags dst_stage_mask = memory_barrier.dst_stage_mask;

	vkCmdPipelineBarrier(
	    get_handle(),
	    src_stage_mask,
	    dst_stage_mask,
	    0,
	    0, nullptr,
	    1, &buffer_memory_barrier,
	    0, nullptr);
}

void CommandBuffer::flush_pipeline_state(VkPipelineBindPoint pipeline_bind_point)
{
	// Create a new pipeline only if the graphics state changed
	if (!pipeline_state.is_dirty())
	{
		return;
	}

	pipeline_state.clear_dirty();

	// Create and bind pipeline
	if (pipeline_bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS)
	{
		pipeline_state.set_render_pass(*current_render_pass.render_pass);
		auto &pipeline = get_device().get_resource_cache().request_graphics_pipeline(pipeline_state);

		vkCmdBindPipeline(get_handle(),
		                  pipeline_bind_point,
		                  pipeline.get_handle());
	}
	else if (pipeline_bind_point == VK_PIPELINE_BIND_POINT_COMPUTE)
	{
		auto &pipeline = get_device().get_resource_cache().request_compute_pipeline(pipeline_state);

		vkCmdBindPipeline(get_handle(),
		                  pipeline_bind_point,
		                  pipeline.get_handle());
	}
	else
	{
		throw "Only graphics and compute pipeline bind points are supported now";
	}
}

void CommandBuffer::flush_descriptor_state(VkPipelineBindPoint pipeline_bind_point)
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

			BindingMap<VkDescriptorBufferInfo> buffer_infos;
			BindingMap<VkDescriptorImageInfo>  image_infos;

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
						if (buffer != nullptr && is_buffer_descriptor_type(binding_info->descriptorType))
						{
							VkDescriptorBufferInfo buffer_info{};

							buffer_info.buffer = resource_info.buffer->get_handle();
							buffer_info.offset = resource_info.offset;
							buffer_info.range  = resource_info.range;

							if (is_dynamic_buffer_descriptor_type(binding_info->descriptorType))
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
							VkDescriptorImageInfo image_info{};
							image_info.sampler   = sampler ? sampler->get_handle() : VK_NULL_HANDLE;
							image_info.imageView = image_view->get_handle();

							if (image_view != nullptr)
							{
								// Add image layout info based on descriptor type
								switch (binding_info->descriptorType)
								{
									case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
										image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
										break;
									case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
										if (is_depth_format(image_view->get_format()))
										{
											image_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
										}
										else
										{
											image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
										}
										break;
									case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
										image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
										break;

									default:
										continue;
								}
							}

							image_infos[binding_index][array_element] = image_info;
						}
					}

					assert((!update_after_bind ||
					        (buffer_infos.count(binding_index) > 0 || (image_infos.count(binding_index) > 0))) &&
					       "binding index with no buffer or image infos can't be checked for adding to bindings_to_update");
				}
			}

			VkDescriptorSet descriptor_set_handle =
			    command_pool.get_render_frame()->request_descriptor_set(descriptor_set_layout,
			                                                            buffer_infos,
			                                                            image_infos,
			                                                            update_after_bind,
			                                                            command_pool.get_thread_index());

			// Bind descriptor set
			vkCmdBindDescriptorSets(get_handle(),
			                        pipeline_bind_point,
			                        pipeline_layout.get_handle(),
			                        descriptor_set_id,
			                        1, &descriptor_set_handle,
			                        to_u32(dynamic_offsets.size()),
			                        dynamic_offsets.data());
		}
	}
}

void CommandBuffer::flush_push_constants()
{
	if (stored_push_constants.empty())
	{
		return;
	}

	const PipelineLayout &pipeline_layout = pipeline_state.get_pipeline_layout();

	VkShaderStageFlags shader_stage = pipeline_layout.get_push_constant_range_stage(to_u32(stored_push_constants.size()));

	if (shader_stage)
	{
		vkCmdPushConstants(get_handle(), pipeline_layout.get_handle(), shader_stage, 0, to_u32(stored_push_constants.size()), stored_push_constants.data());
	}
	else
	{
		LOGW("Push constant range [{}, {}] not found", 0, stored_push_constants.size());
	}

	stored_push_constants.clear();
}

void CommandBuffer::set_update_after_bind(bool update_after_bind_)
{
	update_after_bind = update_after_bind_;
}

const CommandBuffer::RenderPassBinding &CommandBuffer::get_current_render_pass() const
{
	return current_render_pass;
}

const uint32_t CommandBuffer::get_current_subpass_index() const
{
	return pipeline_state.get_subpass_index();
}

const bool CommandBuffer::is_render_size_optimal(const VkExtent2D &framebuffer_extent, const VkRect2D &render_area)
{
	auto render_area_granularity = current_render_pass.render_pass->get_render_area_granularity();

	return ((render_area.offset.x % render_area_granularity.width == 0) && (render_area.offset.y % render_area_granularity.height == 0) &&
	        ((render_area.extent.width % render_area_granularity.width == 0) || (render_area.offset.x + render_area.extent.width == framebuffer_extent.width)) &&
	        ((render_area.extent.height % render_area_granularity.height == 0) || (render_area.offset.y + render_area.extent.height == framebuffer_extent.height)));
}

void CommandBuffer::reset_query_pool(const QueryPool &query_pool, uint32_t first_query, uint32_t query_count)
{
	vkCmdResetQueryPool(get_handle(), query_pool.get_handle(), first_query, query_count);
}

void CommandBuffer::begin_query(const QueryPool &query_pool, uint32_t query, VkQueryControlFlags flags)
{
	vkCmdBeginQuery(get_handle(), query_pool.get_handle(), query, flags);
}

void CommandBuffer::end_query(const QueryPool &query_pool, uint32_t query)
{
	vkCmdEndQuery(get_handle(), query_pool.get_handle(), query);
}

void CommandBuffer::write_timestamp(VkPipelineStageFlagBits pipeline_stage,
                                    const QueryPool &query_pool, uint32_t query)
{
	vkCmdWriteTimestamp(get_handle(), pipeline_stage, query_pool.get_handle(), query);
}

VkResult CommandBuffer::reset(ResetMode reset_mode)
{
	VkResult result = VK_SUCCESS;

	assert(reset_mode == command_pool.get_reset_mode() && "Command buffer reset mode must match the one used by the pool to allocate it");

	if (reset_mode == ResetMode::ResetIndividually)
	{
		result = vkResetCommandBuffer(handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	}

	return result;
}

RenderPass &CommandBuffer::get_render_pass(const vkb::RenderTarget &render_target, const std::vector<LoadStoreInfo> &load_store_infos, const std::vector<std::unique_ptr<Subpass>> &subpasses)
{
	// Create render pass
	assert(subpasses.size() > 0 && "Cannot create a render pass without any subpass");

	std::vector<vkb::SubpassInfo> subpass_infos(subpasses.size());
	auto                          subpass_info_it = subpass_infos.begin();
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

	return get_device().get_resource_cache().request_render_pass(render_target.get_attachments(), load_store_infos, subpass_infos);
}
}        // namespace vkb
