/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "command_buffer_usage.h"

#include <algorithm>
#include <numeric>

#include "core/device.h"
#include "core/pipeline_layout.h"
#include "core/shader_module.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "stats.h"

CommandBufferUsage::CommandBufferUsage()
{
	auto &config = get_configuration();

	config.insert<vkb::IntSetting>(0, gui_secondary_cmd_buf_count, 0);
	config.insert<vkb::BoolSetting>(0, gui_multi_threading, false);
	config.insert<vkb::IntSetting>(0, gui_command_buffer_reset_mode, 0);

	config.insert<vkb::IntSetting>(1, gui_secondary_cmd_buf_count, 2);
	config.insert<vkb::BoolSetting>(1, gui_multi_threading, true);
	config.insert<vkb::IntSetting>(1, gui_command_buffer_reset_mode, 0);

	config.insert<vkb::IntSetting>(2, gui_secondary_cmd_buf_count, 2);
	config.insert<vkb::BoolSetting>(2, gui_multi_threading, true);
	config.insert<vkb::IntSetting>(2, gui_command_buffer_reset_mode, 1);

	config.insert<vkb::IntSetting>(3, gui_secondary_cmd_buf_count, 2);
	config.insert<vkb::BoolSetting>(3, gui_multi_threading, true);
	config.insert<vkb::IntSetting>(3, gui_command_buffer_reset_mode, 2);
}

bool CommandBufferUsage::prepare(vkb::Platform &platform)
{
	if (!VulkanSample::prepare(platform))
	{
		return false;
	}

	load_scene("scenes/bonza/Bonza4X.gltf");

	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = dynamic_cast<vkb::sg::PerspectiveCamera *>(&camera_node.get_component<vkb::sg::Camera>());

	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_subpass = std::make_unique<ForwardSubpassSecondary>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, *camera);

	auto render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_subpass));

	set_render_pipeline(std::move(render_pipeline));

	stats = std::make_unique<vkb::Stats>(std::set<vkb::StatIndex>{vkb::StatIndex::frame_times, vkb::StatIndex::cpu_cycles});
	gui   = std::make_unique<vkb::Gui>(*this, platform.get_window().get_dpi_factor());

	// Adjust the maximum number of secondary command buffers
	// In this sample, only the recording of opaque meshes will be multi-threaded
	auto is_opaque = [](vkb::sg::SubMesh *sub_mesh) {
		return sub_mesh->get_material()->alpha_mode != vkb::sg::AlphaMode::Blend;
	};
	auto count_opaque_submeshes = [is_opaque](uint32_t accumulated, const vkb::sg::Mesh *mesh) -> uint32_t {
		return accumulated + vkb::to_u32(mesh->get_nodes().size() * std::count_if(mesh->get_submeshes().begin(), mesh->get_submeshes().end(), is_opaque));
	};
	const auto &   mesh_components   = scene->get_components<vkb::sg::Mesh>();
	const uint32_t opaque_mesh_count = std::accumulate(mesh_components.begin(), mesh_components.end(), 0, count_opaque_submeshes);

	max_secondary_command_buffer_count = std::min(opaque_mesh_count, max_secondary_command_buffer_count);

	// Show number of opaque meshes in debug window
	get_debug_info().insert<vkb::field::Static, uint32_t>("opaque_mesh_count", opaque_mesh_count);

	return true;
}

void CommandBufferUsage::prepare_render_context()
{
	max_thread_count = std::max(std::thread::hardware_concurrency(), MIN_THREAD_COUNT);
	get_render_context().prepare(max_thread_count);
}

void CommandBufferUsage::update(float delta_time)
{
	auto &subpass_state = static_cast<ForwardSubpassSecondary *>(render_pipeline->get_active_subpass().get())->get_state();

	// Process GUI input
	subpass_state.secondary_cmd_buf_count = vkb::to_u32(gui_secondary_cmd_buf_count);

	use_secondary_command_buffers = subpass_state.secondary_cmd_buf_count > 0;

	// If there are not enough command buffers to keep all threads busy, use fewer threads
	subpass_state.thread_count = std::min(subpass_state.secondary_cmd_buf_count, max_thread_count);

	subpass_state.command_buffer_reset_mode = static_cast<vkb::CommandBuffer::ResetMode>(gui_command_buffer_reset_mode);

	subpass_state.multi_threading = gui_multi_threading;

	update_scene(delta_time);

	update_stats(delta_time);

	update_gui(delta_time);

	auto &render_context = get_render_context();

	auto &primary_command_buffer = render_context.begin(subpass_state.command_buffer_reset_mode);

	primary_command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	draw(primary_command_buffer, render_context.get_active_frame().get_render_target());

	primary_command_buffer.end();

	render_context.submit(primary_command_buffer);
}

void CommandBufferUsage::draw_gui()
{
	const bool landscape = camera->get_aspect_ratio() > 1.0f;
	uint32_t   lines     = landscape ? 3 : 5;

	const auto &subpass = static_cast<ForwardSubpassSecondary *>(render_pipeline->get_active_subpass().get());

	gui->show_options_window(
	    /* body = */ [&]() {
		    // Secondary command buffer count
		    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.55f);
		    ImGui::SliderInt("", &gui_secondary_cmd_buf_count, 0, max_secondary_command_buffer_count, "Secondary CmdBuffs: %d");
		    ImGui::SameLine();
		    ImGui::Text("Draws/buf: %.1f", subpass->get_avg_draws_per_buffer());

		    // Multi-threading (no effect if 0 secondary command buffers)
		    ImGui::Checkbox("Multi-threading", &gui_multi_threading);
		    ImGui::SameLine();
		    ImGui::Text("(%d threads)", subpass->get_state().thread_count);

		    // Buffer management options
		    ImGui::RadioButton("Allocate and free", &gui_command_buffer_reset_mode, static_cast<int>(vkb::CommandBuffer::ResetMode::AlwaysAllocate));
		    if (landscape)
		    {
			    ImGui::SameLine();
		    }
		    ImGui::RadioButton("Reset buffer", &gui_command_buffer_reset_mode, static_cast<int>(vkb::CommandBuffer::ResetMode::ResetIndividually));
		    if (landscape)
		    {
			    ImGui::SameLine();
		    }
		    ImGui::RadioButton("Reset pool", &gui_command_buffer_reset_mode, static_cast<int>(vkb::CommandBuffer::ResetMode::ResetPool));
	    },
	    /* lines = */ lines);
}

void CommandBufferUsage::render(vkb::CommandBuffer &primary_command_buffer)
{
	if (render_pipeline)
	{
		if (use_secondary_command_buffers)
		{
			// The user will set the number of secondary command buffers used for opaque meshes
			// There will be additional buffers for transparent meshes and for the GUI
			render_pipeline->draw(primary_command_buffer, get_render_context().get_active_frame().get_render_target(), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		}
		else
		{
			render_pipeline->draw(primary_command_buffer, get_render_context().get_active_frame().get_render_target(), VK_SUBPASS_CONTENTS_INLINE);
		}
	}
}

void CommandBufferUsage::draw_renderpass(vkb::CommandBuffer &primary_command_buffer, vkb::RenderTarget &render_target)
{
	const auto &subpass = static_cast<ForwardSubpassSecondary *>(render_pipeline->get_active_subpass().get());
	auto &      extent  = render_target.get_extent();

	VkViewport viewport{};
	viewport.width    = static_cast<float>(extent.width);
	viewport.height   = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	primary_command_buffer.set_viewport(0, {viewport});
	subpass->set_viewport(viewport);

	VkRect2D scissor{};
	scissor.extent = extent;
	primary_command_buffer.set_scissor(0, {scissor});
	subpass->set_scissor(scissor);

	render(primary_command_buffer);

	// Draw gui
	if (gui)
	{
		if (use_secondary_command_buffers)
		{
			const auto &queue = device->get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

			auto &secondary_command_buffer = get_render_context().get_active_frame().request_command_buffer(queue, subpass->get_state().command_buffer_reset_mode, VK_COMMAND_BUFFER_LEVEL_SECONDARY);

			secondary_command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &primary_command_buffer);

			secondary_command_buffer.set_viewport(0, {viewport});

			secondary_command_buffer.set_scissor(0, {scissor});

			gui->draw(secondary_command_buffer);

			secondary_command_buffer.end();

			primary_command_buffer.execute_commands(secondary_command_buffer);
		}
		else
		{
			gui->draw(primary_command_buffer);
		}
	}

	primary_command_buffer.end_render_pass();
}

CommandBufferUsage::ForwardSubpassSecondary::ForwardSubpassSecondary(vkb::RenderContext &render_context,
                                                                     vkb::ShaderSource &&vertex_shader, vkb::ShaderSource &&fragment_shader, vkb::sg::Scene &scene_, vkb::sg::Camera &camera) :
    vkb::ForwardSubpass{render_context, std::move(vertex_shader), std::move(fragment_shader), scene_, camera}
{
}

void CommandBufferUsage::ForwardSubpassSecondary::record_draw(vkb::CommandBuffer &                                               command_buffer,
                                                              const std::vector<std::pair<vkb::sg::Node *, vkb::sg::SubMesh *>> &nodes,
                                                              uint32_t mesh_start, uint32_t mesh_end, size_t thread_index)
{
	command_buffer.set_color_blend_state(color_blend_state);

	command_buffer.set_depth_stencil_state(get_depth_stencil_state());

	command_buffer.bind_buffer(light_buffer.get_buffer(), light_buffer.get_offset(), light_buffer.get_size(), 0, 4, 0);

	for (uint32_t i = mesh_start; i < mesh_end; i++)
	{
		update_uniform(command_buffer, *nodes.at(i).first, thread_index);

		draw_submesh(command_buffer, *nodes.at(i).second);
	}
}

vkb::CommandBuffer *CommandBufferUsage::ForwardSubpassSecondary::record_draw_secondary(vkb::CommandBuffer &                                               primary_command_buffer,
                                                                                       const std::vector<std::pair<vkb::sg::Node *, vkb::sg::SubMesh *>> &nodes,
                                                                                       uint32_t mesh_start, uint32_t mesh_end, size_t thread_index)
{
	const auto &queue = render_context.get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

	auto &secondary_command_buffer = render_context.get_active_frame().request_command_buffer(queue, state.command_buffer_reset_mode, VK_COMMAND_BUFFER_LEVEL_SECONDARY, thread_index);

	secondary_command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &primary_command_buffer);

	secondary_command_buffer.set_viewport(0, {viewport});

	secondary_command_buffer.set_scissor(0, {scissor});

	record_draw(secondary_command_buffer, nodes, mesh_start, mesh_end, thread_index);

	secondary_command_buffer.end();

	return &secondary_command_buffer;
}
void CommandBufferUsage::ForwardSubpassSecondary::draw(vkb::CommandBuffer &primary_command_buffer)
{
	std::multimap<float, std::pair<vkb::sg::Node *, vkb::sg::SubMesh *>> opaque_nodes;

	std::multimap<float, std::pair<vkb::sg::Node *, vkb::sg::SubMesh *>> transparent_nodes;

	get_sorted_nodes(opaque_nodes, transparent_nodes);

	// Sort opaque objects in front-to-back order
	// Note: sorting objects does not help on PowerVR, so it can be avoided to save CPU cycles
	std::vector<std::pair<vkb::sg::Node *, vkb::sg::SubMesh *>> sorted_opaque_nodes;
	for (auto node_it = opaque_nodes.begin(); node_it != opaque_nodes.end(); node_it++)
	{
		sorted_opaque_nodes.push_back(node_it->second);
	}
	const auto opaque_submeshes = vkb::to_u32(sorted_opaque_nodes.size());

	// Sort transparent objects in back-to-front order
	std::vector<std::pair<vkb::sg::Node *, vkb::sg::SubMesh *>> sorted_transparent_nodes;
	for (auto node_it = transparent_nodes.rbegin(); node_it != transparent_nodes.rend(); node_it++)
	{
		sorted_transparent_nodes.push_back(node_it->second);
	}
	const auto transparent_submeshes = vkb::to_u32(sorted_transparent_nodes.size());

	light_buffer = allocate_lights<vkb::ForwardLights>(scene.get_components<vkb::sg::Light>(), MAX_FORWARD_LIGHT_COUNT);

	color_blend_attachment.blend_enable = VK_FALSE;
	color_blend_state.attachments.resize(get_output_attachments().size());
	color_blend_state.attachments[0] = color_blend_attachment;

	// Draw opaque objects. Depending on the subpass state, use one or multiple
	// command buffers, and one or multiple threads
	const bool                        use_secondary_command_buffers = state.secondary_cmd_buf_count > 0;
	std::vector<vkb::CommandBuffer *> secondary_command_buffers;
	avg_draws_per_buffer = (state.secondary_cmd_buf_count > 0) ? static_cast<float>(opaque_submeshes) / state.secondary_cmd_buf_count : 0;

	if (state.thread_count != thread_pool.size())
	{
		thread_pool.resize(state.thread_count);
	}

	if (use_secondary_command_buffers)
	{
		std::vector<std::future<vkb::CommandBuffer *>> secondary_cmd_buf_futures;

		// Save the number of draws left over, these will be distributed among the first buffers
		uint32_t draws_per_buffer = vkb::to_u32(std::floor(avg_draws_per_buffer));
		uint32_t remainder_draws  = opaque_submeshes % state.secondary_cmd_buf_count;
		uint32_t mesh_start       = 0;

		for (uint32_t cb_count = 0; cb_count < state.secondary_cmd_buf_count; cb_count++)
		{
			// Latter command buffers may contain fewer draws
			uint32_t mesh_end = std::min(opaque_submeshes, mesh_start + draws_per_buffer);
			if (remainder_draws > 0)
			{
				mesh_end++;
				remainder_draws--;
			}

			if (state.multi_threading)
			{
				auto fut = thread_pool.push(
				    [this, cb_count, &primary_command_buffer, &sorted_opaque_nodes, mesh_start, mesh_end](size_t thread_id) {
					    return record_draw_secondary(primary_command_buffer, sorted_opaque_nodes, mesh_start, mesh_end, thread_id);
				    });

				secondary_cmd_buf_futures.push_back(std::move(fut));
			}
			else
			{
				secondary_command_buffers.push_back(record_draw_secondary(primary_command_buffer, sorted_opaque_nodes, mesh_start, mesh_end));
			}

			mesh_start = mesh_end;
		}

		if (state.multi_threading)
		{
			for (auto &fut : secondary_cmd_buf_futures)
			{
				secondary_command_buffers.push_back(fut.get());
			}
		}
	}
	else
	{
		record_draw(primary_command_buffer, sorted_opaque_nodes, 0, opaque_submeshes);
	}

	// Enable alpha blending
	color_blend_attachment.blend_enable           = VK_TRUE;
	color_blend_attachment.src_color_blend_factor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dst_color_blend_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.src_alpha_blend_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

	color_blend_state.attachments[0] = color_blend_attachment;

	// Draw transparent objects
	if (transparent_submeshes > 0)
	{
		if (use_secondary_command_buffers)
		{
			secondary_command_buffers.push_back(record_draw_secondary(primary_command_buffer, sorted_transparent_nodes, 0, transparent_submeshes));
		}
		else
		{
			record_draw(primary_command_buffer, sorted_transparent_nodes, 0, transparent_submeshes);
		}
	}

	if (use_secondary_command_buffers)
	{
		primary_command_buffer.execute_commands(secondary_command_buffers);
	}
}

void CommandBufferUsage::ForwardSubpassSecondary::set_viewport(VkViewport &viewport)
{
	this->viewport = viewport;
}

void CommandBufferUsage::ForwardSubpassSecondary::set_scissor(VkRect2D &scissor)
{
	this->scissor = scissor;
}

float CommandBufferUsage::ForwardSubpassSecondary::get_avg_draws_per_buffer() const
{
	return avg_draws_per_buffer;
}

CommandBufferUsage::ForwardSubpassSecondaryState &CommandBufferUsage::ForwardSubpassSecondary::get_state()
{
	return state;
}

std::unique_ptr<vkb::VulkanSample> create_command_buffer_usage()
{
	return std::make_unique<CommandBufferUsage>();
}
