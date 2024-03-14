/* Copyright (c) 2021-2024, Arm Limited and Contributors
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

#include "postprocessing_renderpass.h"

#include "postprocessing_pipeline.h"

namespace vkb
{
constexpr uint32_t DEPTH_RESOLVE_BITMASK = 0x80000000;
constexpr uint32_t ATTACHMENT_BITMASK    = 0x7FFFFFFF;

PostProcessingSubpass::PostProcessingSubpass(PostProcessingRenderPass *parent, RenderContext &render_context, ShaderSource &&triangle_vs,
                                             ShaderSource &&fs, ShaderVariant &&fs_variant) :
    Subpass(render_context, std::move(triangle_vs), std::move(fs)),
    parent{parent},
    fs_variant{std::move(fs_variant)}
{
	set_disable_depth_stencil_attachment(true);

	std::vector<uint32_t> input_attachments{};
	for (const auto &it : this->input_attachments)
	{
		input_attachments.push_back(it.second);
	}
	set_input_attachments(input_attachments);
}

PostProcessingSubpass::PostProcessingSubpass(PostProcessingSubpass &&to_move) :
    Subpass{std::move(to_move)},
    parent{std::move(to_move.parent)},
    fs_variant{std::move(to_move.fs_variant)},
    input_attachments{std::move(to_move.input_attachments)},
    sampled_images{std::move(to_move.sampled_images)}
{}

PostProcessingSubpass &PostProcessingSubpass::bind_input_attachment(const std::string &name, uint32_t new_input_attachment)
{
	input_attachments[name] = new_input_attachment;

	std::vector<uint32_t> input_attachments{};
	for (const auto &it : this->input_attachments)
	{
		input_attachments.push_back(it.second);
	}
	set_input_attachments(input_attachments);

	parent->load_stores_dirty = true;
	return *this;
}

void PostProcessingSubpass::unbind_sampled_image(const std::string &name)
{
	sampled_images.erase(name);
}

PostProcessingSubpass &PostProcessingSubpass::bind_sampled_image(const std::string &name, core::SampledImage &&new_image)
{
	auto it = sampled_images.find(name);
	if (it != sampled_images.end())
	{
		it->second = std::move(new_image);
	}
	else
	{
		sampled_images.emplace(name, std::move(new_image));
	}

	parent->load_stores_dirty = true;
	return *this;
}

PostProcessingSubpass &PostProcessingSubpass::bind_storage_image(const std::string &name, const core::ImageView &new_image)
{
	auto it = storage_images.find(name);
	if (it != storage_images.end())
	{
		it->second = &new_image;
	}
	else
	{
		storage_images.emplace(name, &new_image);
	}

	return *this;
}

PostProcessingSubpass &PostProcessingSubpass::set_push_constants(const std::vector<uint8_t> &data)
{
	push_constants_data = data;
	return *this;
}

PostProcessingSubpass &PostProcessingSubpass::set_draw_func(DrawFunc &&new_func)
{
	draw_func = std::move(new_func);
	return *this;
}

void PostProcessingSubpass::prepare()
{
	// Build all shaders upfront
	auto &resource_cache = render_context.get_device().get_resource_cache();
	resource_cache.request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader());
	resource_cache.request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader(), fs_variant);
}

void PostProcessingSubpass::draw(CommandBuffer &command_buffer)
{
	// Get shaders from cache
	auto &resource_cache     = command_buffer.get_device().get_resource_cache();
	auto &vert_shader_module = resource_cache.request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader());
	auto &frag_shader_module = resource_cache.request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader(), fs_variant);

	std::vector<ShaderModule *> shader_modules{&vert_shader_module, &frag_shader_module};

	// Create pipeline layout and bind it
	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	// Disable culling
	RasterizationState rasterization_state;
	rasterization_state.cull_mode = VK_CULL_MODE_NONE;
	command_buffer.set_rasterization_state(rasterization_state);

	auto          &render_target       = *parent->draw_render_target;
	const auto    &target_views        = render_target.get_views();
	const uint32_t n_input_attachments = static_cast<uint32_t>(get_input_attachments().size());

	if (parent->uniform_buffer_alloc != nullptr)
	{
		// Bind buffer to set = 0, binding = 0
		auto &uniform_alloc = *parent->uniform_buffer_alloc;
		command_buffer.bind_buffer(uniform_alloc.get_buffer(), uniform_alloc.get_offset(), uniform_alloc.get_size(), 0, 0, 0);
	}

	const auto &bindings = pipeline_layout.get_descriptor_set_layout(0);

	// Bind subpass inputs to set = 0, binding = <according to name>
	for (const auto &it : input_attachments)
	{
		if (auto layout_binding = bindings.get_layout_binding(it.first))
		{
			assert(it.second < target_views.size());
			command_buffer.bind_input(target_views[it.second], 0, layout_binding->binding, 0);
		}
	}

	// Bind samplers to set = 0, binding = <according to name>
	for (const auto &it : sampled_images)
	{
		if (auto layout_binding = bindings.get_layout_binding(it.first))
		{
			const auto &view = it.second.get_image_view(render_target);

			// Get the properties for the image format. We need to check whether a linear sampler is valid.
			const VkFormatProperties fmtProps          = get_render_context().get_device().get_gpu().get_format_properties(view.get_format());
			bool                     has_linear_filter = (fmtProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

			const auto &sampler = it.second.get_sampler() ? *it.second.get_sampler() :
			                                                (has_linear_filter ? *parent->default_sampler : *parent->default_sampler_nearest);

			command_buffer.bind_image(view, sampler, 0, layout_binding->binding, 0);
		}
	}

	// Bind storage images to set = 0, binding = <according to name>
	for (const auto &it : storage_images)
	{
		if (auto layout_binding = bindings.get_layout_binding(it.first))
		{
			command_buffer.bind_image(*it.second, 0, layout_binding->binding, 0);
		}
	}

	// Per-draw push constants
	command_buffer.push_constants(push_constants_data);

	// draw full screen triangle
	draw_func(command_buffer, render_target);
}

void PostProcessingSubpass::default_draw_func(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &)
{
	command_buffer.draw(3, 1, 0, 0);
}

PostProcessingRenderPass::PostProcessingRenderPass(PostProcessingPipeline *parent, std::unique_ptr<core::Sampler> &&default_sampler) :
    PostProcessingPass{parent},
    default_sampler{std::move(default_sampler)}
{
	if (this->default_sampler == nullptr)
	{
		// Setup a sane default sampler if none was passed
		VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		sampler_info.minFilter        = VK_FILTER_LINEAR;
		sampler_info.magFilter        = VK_FILTER_LINEAR;
		sampler_info.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		sampler_info.addressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_info.addressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_info.addressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_info.mipLodBias       = 0.0f;
		sampler_info.compareOp        = VK_COMPARE_OP_NEVER;
		sampler_info.minLod           = 0.0f;
		sampler_info.maxLod           = 0.0f;
		sampler_info.anisotropyEnable = VK_FALSE;
		sampler_info.maxAnisotropy    = 0.0f;
		sampler_info.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		this->default_sampler = std::make_unique<vkb::core::Sampler>(get_render_context().get_device(), sampler_info);

		// Also create a nearest filtering version as a fallback
		sampler_info.minFilter = VK_FILTER_NEAREST;
		sampler_info.magFilter = VK_FILTER_NEAREST;

		this->default_sampler_nearest = std::make_unique<vkb::core::Sampler>(get_render_context().get_device(), sampler_info);
	}
}

void PostProcessingRenderPass::update_load_stores(
    const AttachmentSet        &input_attachments,
    const SampledAttachmentSet &sampled_attachments,
    const AttachmentSet        &output_attachments,
    const RenderTarget         &fallback_render_target)
{
	if (!load_stores_dirty)
	{
		return;
	}

	const auto &render_target = this->render_target ? *this->render_target : fallback_render_target;

	// Update load/stores accordingly
	load_stores.clear();

	for (uint32_t j = 0; j < static_cast<uint32_t>(render_target.get_attachments().size()); j++)
	{
		const bool is_input   = input_attachments.find(j) != input_attachments.end();
		const bool is_sampled = std::find_if(sampled_attachments.begin(), sampled_attachments.end(),
		                                     [&render_target, j](auto &pair) {
			                                     // NOTE: if RT not set, default is the currently-active one
			                                     auto *sampled_rt = pair.first ? pair.first : &render_target;
			                                     // unpack attachment
			                                     uint32_t attachment = pair.second & ATTACHMENT_BITMASK;
			                                     return attachment == j && sampled_rt == &render_target;
		                                     }) != sampled_attachments.end();
		const bool is_output  = output_attachments.find(j) != output_attachments.end();

		VkAttachmentLoadOp load;
		if (is_input || is_sampled)
		{
			load = VK_ATTACHMENT_LOAD_OP_LOAD;
		}
		else if (is_output)
		{
			load = VK_ATTACHMENT_LOAD_OP_CLEAR;
		}
		else
		{
			load = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		}

		VkAttachmentStoreOp store;
		if (is_output)
		{
			store = VK_ATTACHMENT_STORE_OP_STORE;
		}
		else
		{
			store = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}

		load_stores.push_back({load, store});
	}

	pipeline.set_load_store(load_stores);
	load_stores_dirty = false;
}

PostProcessingRenderPass::BarrierInfo PostProcessingRenderPass::get_src_barrier_info() const
{
	BarrierInfo info{};
	info.pipeline_stage     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	info.image_read_access  = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	info.image_write_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	return info;
}

PostProcessingRenderPass::BarrierInfo PostProcessingRenderPass::get_dst_barrier_info() const
{
	BarrierInfo info{};
	info.pipeline_stage     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	info.image_read_access  = VK_ACCESS_SHADER_READ_BIT;
	info.image_write_access = VK_ACCESS_SHADER_WRITE_BIT;
	return info;
}

// If the passed `src_access` is zero, guess it - and the corresponding source stage - from the src_access_mask
// of the image
static void ensure_src_access(uint32_t &src_access, uint32_t &src_stage, VkImageLayout layout)
{
	if (src_access == 0)
	{
		switch (layout)
		{
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				src_stage  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				src_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				src_access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
				break;
			default:
				src_stage  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				src_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
		}
	}
}

void PostProcessingRenderPass::transition_attachments(
    const AttachmentSet        &input_attachments,
    const SampledAttachmentSet &sampled_attachments,
    const AttachmentSet        &output_attachments,
    CommandBuffer              &command_buffer,
    RenderTarget               &fallback_render_target)
{
	auto       &render_target = this->render_target ? *this->render_target : fallback_render_target;
	const auto &views         = render_target.get_views();

	BarrierInfo fallback_barrier_src{};
	fallback_barrier_src.pipeline_stage     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	fallback_barrier_src.image_read_access  = 0;        // For UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL in first RP
	fallback_barrier_src.image_write_access = 0;
	auto prev_pass_barrier_info             = get_predecessor_src_barrier_info(fallback_barrier_src);

	for (uint32_t input : input_attachments)
	{
		const VkImageLayout prev_layout = render_target.get_layout(input);
		if (prev_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			// No-op
			continue;
		}

		ensure_src_access(prev_pass_barrier_info.image_write_access, prev_pass_barrier_info.pipeline_stage,
		                  prev_layout);

		vkb::ImageMemoryBarrier barrier;
		barrier.old_layout      = render_target.get_layout(input);
		barrier.new_layout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.src_access_mask = prev_pass_barrier_info.image_write_access;
		barrier.dst_access_mask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		barrier.src_stage_mask  = prev_pass_barrier_info.pipeline_stage;
		barrier.dst_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		assert(input < views.size());
		command_buffer.image_memory_barrier(views[input], barrier);
		render_target.set_layout(input, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	for (const auto &sampled : sampled_attachments)
	{
		auto *sampled_rt = sampled.first ? sampled.first : &render_target;

		// unpack depth resolve flag and attachment
		bool     is_depth_resolve = sampled.second & DEPTH_RESOLVE_BITMASK;
		uint32_t attachment       = sampled.second & ATTACHMENT_BITMASK;

		const auto prev_layout = sampled_rt->get_layout(attachment);

		if (prev_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			// No-op
			continue;
		}

		if (prev_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			// Synchronize with previous pass writes as barrier below might do image transition
			prev_pass_barrier_info.pipeline_stage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			prev_pass_barrier_info.image_read_access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			// The resolving depth occurs in the COLOR_ATTACHMENT_OUT stage, not in the EARLY\LATE_FRAGMENT_TESTS stage
			// and the corresponding access mask is COLOR_ATTACHMENT_WRITE_BIT, not DEPTH_STENCIL_ATTACHMENT_WRITE_BIT.
			if (is_depth_resolve)
			{
				prev_pass_barrier_info.pipeline_stage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				prev_pass_barrier_info.image_read_access |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			}
		}
		else
		{
			ensure_src_access(prev_pass_barrier_info.image_read_access, prev_pass_barrier_info.pipeline_stage,
			                  prev_layout);
		}

		vkb::ImageMemoryBarrier barrier;
		barrier.old_layout      = prev_layout;
		barrier.new_layout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.src_access_mask = prev_pass_barrier_info.image_read_access;
		barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
		barrier.src_stage_mask  = prev_pass_barrier_info.pipeline_stage;
		barrier.dst_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		assert(attachment < sampled_rt->get_views().size());
		command_buffer.image_memory_barrier(sampled_rt->get_views()[attachment], barrier);
		sampled_rt->set_layout(attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	for (uint32_t output : output_attachments)
	{
		assert(output < views.size());
		const VkFormat      attachment_format = views[output].get_format();
		const bool          is_depth_stencil  = vkb::is_depth_format(attachment_format);
		const VkImageLayout output_layout     = is_depth_stencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		if (render_target.get_layout(output) == output_layout)
		{
			// No-op
			continue;
		}

		vkb::ImageMemoryBarrier barrier;
		barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;        // = don't care about previous contents
		barrier.new_layout      = output_layout;
		barrier.src_access_mask = 0;
		if (is_depth_stencil)
		{
			barrier.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			barrier.src_stage_mask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			barrier.dst_stage_mask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}
		else
		{
			barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			barrier.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}

		command_buffer.image_memory_barrier(views[output], barrier);
		render_target.set_layout(output, output_layout);
	}

	// NOTE: Unused attachments might be carried over to other render passes,
	//       so we don't want to transition them to UNDEFINED layout here
}

void PostProcessingRenderPass::prepare_draw(CommandBuffer &command_buffer, RenderTarget &fallback_render_target)
{
	// Collect all input, output, and sampled-from attachments from all subpasses (steps)
	AttachmentSet        input_attachments, output_attachments;
	SampledAttachmentSet sampled_attachments;

	for (auto &step_ptr : pipeline.get_subpasses())
	{
		auto &step = *dynamic_cast<PostProcessingSubpass *>(step_ptr.get());

		for (auto &it : step.get_input_attachments())
		{
			input_attachments.insert(it.second);
		}

		for (auto &it : step.get_sampled_images())
		{
			if (const uint32_t *sampled_attachment = it.second.get_target_attachment())
			{
				auto *image_rt                  = it.second.get_render_target();
				auto  packed_sampled_attachment = *sampled_attachment;

				// pack sampled attachment
				if (it.second.is_depth_resolve())
				{
					packed_sampled_attachment |= DEPTH_RESOLVE_BITMASK;
				}

				sampled_attachments.insert({image_rt, packed_sampled_attachment});
			}
		}

		for (uint32_t it : step.get_output_attachments())
		{
			output_attachments.insert(it);
		}
	}

	transition_attachments(input_attachments, sampled_attachments, output_attachments,
	                       command_buffer, fallback_render_target);
	update_load_stores(input_attachments, sampled_attachments, output_attachments,
	                   fallback_render_target);
}

void PostProcessingRenderPass::draw(CommandBuffer &command_buffer, RenderTarget &default_render_target)
{
	prepare_draw(command_buffer, default_render_target);

	if (!uniform_data.empty())
	{
		// Allocate a buffer (using the buffer pool from the active frame to store uniform values) and bind it
		auto &render_frame   = parent->get_render_context().get_active_frame();
		uniform_buffer_alloc = std::make_shared<BufferAllocation>(render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uniform_data.size()));
		uniform_buffer_alloc->update(uniform_data);
	}

	// Update render target for this draw
	draw_render_target = render_target ? render_target : &default_render_target;

	// Set appropriate viewport & scissor for this RT
	{
		auto &extent = draw_render_target->get_extent();

		VkViewport viewport{};
		viewport.width    = static_cast<float>(extent.width);
		viewport.height   = static_cast<float>(extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		command_buffer.set_viewport(0, {viewport});

		VkRect2D scissor{};
		scissor.extent = extent;
		command_buffer.set_scissor(0, {scissor});
	}

	// Finally draw all subpasses
	pipeline.draw(command_buffer, *draw_render_target);

	if (parent->get_current_pass_index() < (parent->get_passes().size() - 1))
	{
		// Leave the last renderpass open for user modification (e.g., drawing GUI)
		command_buffer.end_render_pass();
	}
}

}        // namespace vkb