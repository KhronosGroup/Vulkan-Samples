/* Copyright (c) 2020, Arm Limited and Contributors
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

#include "postprocessing_subpass.h"

#include "buffer_pool.h"
#include "rendering/render_context.h"
#include "scene_graph/components/perspective_camera.h"
#include "scene_graph/scene.h"

namespace vkb
{
PostProcessingSubpass::PostProcessingSubpass(RenderContext &render_context, ShaderSource &&vertex_shader, ShaderSource &&fragment_shader, sg::Scene &scene_, sg::Camera &cam) :
    Subpass{render_context, std::move(vertex_shader), std::move(fragment_shader)},
    camera{cam},
    scene{scene_}
{
	// Create texture samplers
	VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	sampler_info.maxAnisotropy = 1.0f;
	sampler_info.magFilter     = VK_FILTER_LINEAR;
	sampler_info.minFilter     = VK_FILTER_LINEAR;
	sampler_info.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_info.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	color_sampler              = std::make_unique<core::Sampler>(get_render_context().get_device(), sampler_info);
	depth_sampler              = std::make_unique<core::Sampler>(get_render_context().get_device(), sampler_info);
}

void PostProcessingSubpass::prepare()
{
	// Build all shaders upfront
	auto &resource_cache = render_context.get_device().get_resource_cache();
	resource_cache.request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader(), postprocessing_variant);
	resource_cache.request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader(), postprocessing_variant);

	postprocessing_variant_ms_depth.add_definitions({"MS_DEPTH"});
	resource_cache.request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader(), postprocessing_variant_ms_depth);
	resource_cache.request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader(), postprocessing_variant_ms_depth);
}

void PostProcessingSubpass::draw(CommandBuffer &command_buffer)
{
	// Get shaders from cache
	auto &resource_cache     = command_buffer.get_device().get_resource_cache();
	auto &variant            = ms_depth ? postprocessing_variant_ms_depth : postprocessing_variant;
	auto &vert_shader_module = resource_cache.request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader(), variant);
	auto &frag_shader_module = resource_cache.request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader(), variant);

	std::vector<ShaderModule *> shader_modules{&vert_shader_module, &frag_shader_module};

	// Create pipeline layout and bind it
	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	// Get image views of the attachments
	auto &render_target = get_render_context().get_active_frame().get_render_target();
	auto &target_views  = render_target.get_views();

	// Bind depth and color to texture samplers
	command_buffer.bind_image(target_views.at(full_screen_depth), *depth_sampler, 0, 0, 0);
	command_buffer.bind_image(target_views.at(full_screen_color), *color_sampler, 0, 1, 0);

	// Disable culling
	RasterizationState rasterization_state;
	rasterization_state.cull_mode = VK_CULL_MODE_NONE;
	command_buffer.set_rasterization_state(rasterization_state);

	// Populate uniform values
	PostprocessingUniform uniform;
	auto &                d_camera = dynamic_cast<sg::PerspectiveCamera &>(camera);
	uniform.near_far               = {d_camera.get_far_plane(), d_camera.get_near_plane()};

	// Allocate a buffer using the buffer pool from the active frame to store uniform values and bind it
	auto &render_frame = get_render_context().get_active_frame();
	auto  allocation   = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(PostprocessingUniform));
	allocation.update(uniform);
	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 2, 0);

	// Draw full screen triangle
	command_buffer.draw(3, 1, 0, 0);
}

void PostProcessingSubpass::set_full_screen_color(uint32_t attachment)
{
	full_screen_color = attachment;
}

void PostProcessingSubpass::set_full_screen_depth(uint32_t attachment)
{
	full_screen_depth = attachment;
}

void PostProcessingSubpass::set_ms_depth(bool enable)
{
	ms_depth = enable;
}
}        // namespace vkb
