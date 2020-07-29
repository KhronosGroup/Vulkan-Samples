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

#include "lighting_subpass.h"

#include "buffer_pool.h"
#include "rendering/render_context.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/scene.h"

namespace vkb
{
LightingSubpass::LightingSubpass(RenderContext &render_context, ShaderSource &&vertex_shader, ShaderSource &&fragment_shader, sg::Camera &cam, sg::Scene &scene_) :
    Subpass{render_context, std::move(vertex_shader), std::move(fragment_shader)},
    camera{cam},
    scene{scene_}
{
}

void LightingSubpass::prepare()
{
	lighting_variant.add_definitions({"MAX_LIGHT_COUNT " + std::to_string(MAX_DEFERRED_LIGHT_COUNT)});

	lighting_variant.add_definitions(light_type_definitions);
	// Build all shaders upfront
	auto &resource_cache = render_context.get_device().get_resource_cache();
	resource_cache.request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader(), lighting_variant);
	resource_cache.request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader(), lighting_variant);
}

void LightingSubpass::draw(CommandBuffer &command_buffer)
{
	allocate_lights<DeferredLights>(scene.get_components<sg::Light>(), MAX_DEFERRED_LIGHT_COUNT);
	command_buffer.bind_lighting(get_lighting_state(), 0, 4);

	// Get shaders from cache
	auto &resource_cache     = command_buffer.get_device().get_resource_cache();
	auto &vert_shader_module = resource_cache.request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader(), lighting_variant);
	auto &frag_shader_module = resource_cache.request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader(), lighting_variant);

	std::vector<ShaderModule *> shader_modules{&vert_shader_module, &frag_shader_module};

	// Create pipeline layout and bind it
	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	// Get image views of the attachments
	auto &render_target = get_render_context().get_active_frame().get_render_target();
	auto &target_views  = render_target.get_views();

	// Bind depth, albedo, and normal as input attachments
	auto &depth_view = target_views.at(1);
	command_buffer.bind_input(depth_view, 0, 0, 0);

	auto &albedo_view = target_views.at(2);
	command_buffer.bind_input(albedo_view, 0, 1, 0);

	auto &normal_view = target_views.at(3);
	command_buffer.bind_input(normal_view, 0, 2, 0);

	// Set cull mode to front as full screen triangle is clock-wise
	RasterizationState rasterization_state;
	rasterization_state.cull_mode = VK_CULL_MODE_FRONT_BIT;
	command_buffer.set_rasterization_state(rasterization_state);

	// Populate uniform values
	LightUniform light_uniform;

	// Inverse resolution
	light_uniform.inv_resolution.x = 1.0f / render_target.get_extent().width;
	light_uniform.inv_resolution.y = 1.0f / render_target.get_extent().height;

	// Inverse view projection
	light_uniform.inv_view_proj = glm::inverse(vulkan_style_projection(camera.get_projection()) * camera.get_view());

	// Allocate a buffer using the buffer pool from the active frame to store uniform values and bind it
	auto &render_frame = get_render_context().get_active_frame();
	auto  allocation   = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(LightUniform));
	allocation.update(light_uniform);
	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 3, 0);

	// Draw full screen triangle triangle
	command_buffer.draw(3, 1, 0, 0);
}
}        // namespace vkb
