/* Copyright (c) 2020-2024, Arm Limited and Contributors
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

#include "postprocessing_computepass.h"

#include "postprocessing_pipeline.h"

namespace vkb
{
PostProcessingComputePass::PostProcessingComputePass(PostProcessingPipeline *parent, const ShaderSource &cs_source, const ShaderVariant &cs_variant,
                                                     std::shared_ptr<core::Sampler> &&default_sampler) :
    PostProcessingPass{parent},
    cs_source{cs_source},
    cs_variant{cs_variant},
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

		this->default_sampler = std::make_shared<vkb::core::Sampler>(get_render_context().get_device(), sampler_info);

		// Also create a nearest filtering version as a fallback
		sampler_info.minFilter = VK_FILTER_NEAREST;
		sampler_info.magFilter = VK_FILTER_NEAREST;

		this->default_sampler_nearest = std::make_shared<vkb::core::Sampler>(get_render_context().get_device(), sampler_info);
	}
}

void PostProcessingComputePass::prepare(CommandBuffer &command_buffer, RenderTarget &default_render_target)
{
	// Build the compute shader upfront
	auto &resource_cache = get_render_context().get_device().get_resource_cache();
	resource_cache.request_shader_module(VK_SHADER_STAGE_COMPUTE_BIT, cs_source, cs_variant);
}

PostProcessingComputePass &PostProcessingComputePass::bind_sampled_image(const std::string &name, core::SampledImage &&new_image)
{
	auto it = sampled_images.find(name);
	if (it != sampled_images.end())
	{
		it->second = new_image;
	}
	else
	{
		sampled_images.emplace(name, new_image);
	}

	return *this;
}

PostProcessingComputePass &PostProcessingComputePass::bind_storage_image(const std::string &name, core::SampledImage &&new_image)
{
	auto it = storage_images.find(name);
	if (it != storage_images.end())
	{
		it->second = new_image;
	}
	else
	{
		storage_images.emplace(name, new_image);
	}

	return *this;
}

void PostProcessingComputePass::transition_images(CommandBuffer &command_buffer, RenderTarget &default_render_target)
{
	BarrierInfo fallback_barrier_src{};
	fallback_barrier_src.pipeline_stage     = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	fallback_barrier_src.image_read_access  = 0;        // For UNDEFINED -> STORAGE in first CP
	fallback_barrier_src.image_write_access = 0;
	const auto prev_pass_barrier_info       = get_predecessor_src_barrier_info(fallback_barrier_src);

	// Get compute shader from cache
	auto &resource_cache  = command_buffer.get_device().get_resource_cache();
	auto &shader_module   = resource_cache.request_shader_module(VK_SHADER_STAGE_COMPUTE_BIT, cs_source, cs_variant);
	auto &pipeline_layout = resource_cache.request_pipeline_layout({&shader_module});

	for (const auto &sampled : sampled_images)
	{
		if (const uint32_t *attachment = sampled.second.get_target_attachment())
		{
			auto *sampled_rt = sampled.second.get_render_target();
			if (sampled_rt == nullptr)
			{
				sampled_rt = &default_render_target;
			}

			if (sampled_rt->get_layout(*attachment) == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				// No-op
				continue;
			}

			vkb::ImageMemoryBarrier barrier;
			barrier.old_layout      = sampled_rt->get_layout(*attachment);
			barrier.new_layout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.src_access_mask = prev_pass_barrier_info.image_write_access;
			barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
			barrier.src_stage_mask  = prev_pass_barrier_info.pipeline_stage;
			barrier.dst_stage_mask  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

			assert(*attachment < sampled_rt->get_views().size());
			command_buffer.image_memory_barrier(sampled_rt->get_views()[*attachment], barrier);
			sampled_rt->set_layout(*attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}

	const auto &bindings = pipeline_layout.get_descriptor_set_layout(0);

	for (const auto &storage : storage_images)
	{
		if (const uint32_t *attachment = storage.second.get_target_attachment())
		{
			auto *storage_rt = storage.second.get_render_target();
			if (storage_rt == nullptr)
			{
				storage_rt = &default_render_target;
			}

			// A storage image is either readonly or writeonly;
			// use shader reflection to figure out which case, then transition
			// NOTE: Could add a <name -> readonly?> cache to make this faster?
			auto resource = std::find_if(pipeline_layout.get_resources().begin(), pipeline_layout.get_resources().end(),
			                             [&storage](const auto &res) {
				                             return res.set == 0 && res.name == storage.first;
			                             });
			if (resource == pipeline_layout.get_resources().end())
			{
				// No such storage image to bind
				continue;
			}

			const bool readable = !(resource->qualifiers & ShaderResourceQualifiers::NonReadable);
			const bool writable = !(resource->qualifiers & ShaderResourceQualifiers::NonReadable);

			vkb::ImageMemoryBarrier barrier;
			barrier.old_layout = storage_rt->get_layout(*attachment);
			barrier.new_layout = (readable && !writable) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

			if (storage_rt->get_layout(*attachment) == barrier.new_layout)
			{
				// No-op
				continue;
			}

			barrier.src_stage_mask = prev_pass_barrier_info.pipeline_stage;
			barrier.dst_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

			barrier.src_access_mask = prev_pass_barrier_info.image_write_access;
			barrier.dst_access_mask = 0;
			if (readable)
			{
				barrier.dst_access_mask |= VK_ACCESS_SHADER_READ_BIT;
			}
			if (writable)
			{
				barrier.dst_access_mask |= VK_ACCESS_SHADER_WRITE_BIT;
			}

			assert(*attachment < storage_rt->get_views().size());
			command_buffer.image_memory_barrier(storage_rt->get_views()[*attachment], barrier);
			storage_rt->set_layout(*attachment, barrier.new_layout);
		}
	}
}

void PostProcessingComputePass::draw(CommandBuffer &command_buffer, RenderTarget &default_render_target)
{
	transition_images(command_buffer, default_render_target);

	// Get compute shader from cache
	auto &resource_cache = command_buffer.get_device().get_resource_cache();
	auto &shader_module  = resource_cache.request_shader_module(VK_SHADER_STAGE_COMPUTE_BIT, cs_source, cs_variant);

	// Create pipeline layout and bind it
	auto &pipeline_layout = resource_cache.request_pipeline_layout({&shader_module});
	command_buffer.bind_pipeline_layout(pipeline_layout);

	const auto &bindings = pipeline_layout.get_descriptor_set_layout(0);

	// Bind samplers to set = 0, binding = <according to name>
	for (const auto &it : sampled_images)
	{
		if (auto layout_binding = bindings.get_layout_binding(it.first))
		{
			const auto &view = it.second.get_image_view(default_render_target);

			// Get the properties for the image format. We need to check whether a linear sampler is valid.
			const VkFormatProperties fmtProps          = get_render_context().get_device().get_gpu().get_format_properties(view.get_format());
			bool                     has_linear_filter = (fmtProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

			const auto &sampler = it.second.get_sampler() ? *it.second.get_sampler() :
			                                                (has_linear_filter ? *default_sampler : *default_sampler_nearest);

			command_buffer.bind_image(view, sampler, 0, layout_binding->binding, 0);
		}
	}

	// Bind storage images to set = 0, binding = <according to name>
	for (const auto &it : storage_images)
	{
		if (auto layout_binding = bindings.get_layout_binding(it.first))
		{
			const auto &view = it.second.get_image_view(default_render_target);
			command_buffer.bind_image(view, 0, layout_binding->binding, 0);
		}
	}

	if (!uniform_data.empty())
	{
		auto &render_frame = parent->get_render_context().get_active_frame();

		uniform_alloc = std::make_unique<BufferAllocation>(render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uniform_data.size()));
		uniform_alloc->update(uniform_data);

		// Bind buffer to set = 0, binding = 0
		command_buffer.bind_buffer(uniform_alloc->get_buffer(), uniform_alloc->get_offset(), uniform_alloc->get_size(), 0, 0, 0);
	}

	if (!push_constants_data.empty())
	{
		command_buffer.push_constants(push_constants_data);
	}

	// Dispatch compute
	command_buffer.dispatch(n_workgroups.x, n_workgroups.y, n_workgroups.z);
}

PostProcessingComputePass::BarrierInfo PostProcessingComputePass::get_src_barrier_info() const
{
	BarrierInfo info{};
	info.pipeline_stage     = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	info.image_read_access  = VK_ACCESS_SHADER_READ_BIT;
	info.image_write_access = VK_ACCESS_SHADER_WRITE_BIT;
	return info;
}

PostProcessingComputePass::BarrierInfo PostProcessingComputePass::get_dst_barrier_info() const
{
	BarrierInfo info{};
	info.pipeline_stage     = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	info.image_read_access  = VK_ACCESS_SHADER_READ_BIT;
	info.image_write_access = VK_ACCESS_SHADER_WRITE_BIT;
	return info;
}

}        // namespace vkb