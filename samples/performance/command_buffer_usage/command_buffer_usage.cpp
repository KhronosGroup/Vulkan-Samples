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

#include "core/device.h"
#include "core/pipeline_layout.h"
#include "core/shader_module.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "stats.h"

#include "rendering/pipeline_state.h"

CommandBufferUsage::CommandBufferUsage()
{
	auto &config = get_configuration();

	config.insert<vkb::BoolSetting>(0, use_secondary_command_buffers, false);
	config.insert<vkb::IntSetting>(0, reuse_selection, 0);

	config.insert<vkb::BoolSetting>(1, use_secondary_command_buffers, true);
	config.insert<vkb::IntSetting>(1, reuse_selection, 0);

	config.insert<vkb::BoolSetting>(2, use_secondary_command_buffers, true);
	config.insert<vkb::IntSetting>(2, reuse_selection, 1);

	config.insert<vkb::BoolSetting>(3, use_secondary_command_buffers, true);
	config.insert<vkb::IntSetting>(3, reuse_selection, 2);
}

bool CommandBufferUsage::prepare(vkb::Platform &platform)
{
	if (!VulkanSample::prepare(platform))
	{
		return false;
	}

	auto enabled_stats = {vkb::StatIndex::frame_times, vkb::StatIndex::cpu_cycles};

	stats = std::make_unique<vkb::Stats>(enabled_stats);

	load_scene("scenes/bonza/Bonza.gltf");
	auto &camera_node = add_free_camera("main_camera");
	camera            = &camera_node.get_component<vkb::sg::Camera>();

	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_subpass = std::make_unique<SceneSubpassSecondary>(*render_context, std::move(vert_shader), std::move(frag_shader), *scene, *camera);
	scene_subpass_ptr               = scene_subpass.get();

	auto render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_subpass));

	set_render_pipeline(std::move(render_pipeline));

	gui = std::make_unique<vkb::Gui>(*render_context, platform.get_dpi_factor());

	return true;
}

void CommandBufferUsage::update(float delta_time)
{
	update_scene(delta_time);

	update_stats(delta_time);

	update_gui(delta_time);

	auto acquired_semaphore = render_context->begin_frame();

	if (acquired_semaphore == VK_NULL_HANDLE)
	{
		return;
	}

	const auto &queue = device->get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

	auto &render_target = render_context->get_active_frame().get_render_target();

	scene_subpass_ptr->set_command_buffer_reset_mode(static_cast<vkb::CommandBuffer::ResetMode>(reuse_selection));

	auto &primary_command_buffer = render_context->request_frame_command_buffer(queue, static_cast<vkb::CommandBuffer::ResetMode>(reuse_selection));

	primary_command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	record_scene_rendering_commands(primary_command_buffer, render_target);

	primary_command_buffer.end();

	auto render_semaphore = render_context->submit(queue, primary_command_buffer, acquired_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	render_context->end_frame(render_semaphore);
}

void CommandBufferUsage::draw_gui()
{
	auto     extent    = render_context->get_swapchain().get_extent();
	bool     landscape = (extent.width / extent.height) > 1.0f;
	uint32_t lines     = landscape ? 2 : 4;

	gui->show_options_window(
	    /* body = */ [&]() {
		    ImGui::Checkbox("Secondary command buffers", &use_secondary_command_buffers);
		    ImGui::RadioButton("Allocate and free", &reuse_selection, static_cast<int>(vkb::CommandBuffer::ResetMode::AlwaysAllocate));
		    if (landscape)
			    ImGui::SameLine();
		    ImGui::RadioButton("Reset buffer", &reuse_selection, static_cast<int>(vkb::CommandBuffer::ResetMode::ResetIndividually));
		    if (landscape)
			    ImGui::SameLine();
		    ImGui::RadioButton("Reset pool", &reuse_selection, static_cast<int>(vkb::CommandBuffer::ResetMode::ResetPool));
	    },
	    /* lines = */ lines);
}

void CommandBufferUsage::render(vkb::CommandBuffer &primary_command_buffer)
{
	if (render_pipeline)
	{
		scene_subpass_ptr->set_use_secondary_command_buffers(use_secondary_command_buffers);

		if (use_secondary_command_buffers)
		{
			// Record a secondary command buffer for each object in the scene, and the GUI
			// This is definitely not recommended. This sample offers this option to make
			// the difference between the command buffer reset modes more pronounced
			render_pipeline->draw(primary_command_buffer, render_context->get_active_frame().get_render_target(), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		}
		else
		{
			render_pipeline->draw(primary_command_buffer, render_context->get_active_frame().get_render_target(), VK_SUBPASS_CONTENTS_INLINE);
		}
	}
}

void CommandBufferUsage::draw_swapchain_renderpass(vkb::CommandBuffer &primary_command_buffer, vkb::RenderTarget &render_target)
{
	auto &extent = render_target.get_extent();

	VkViewport viewport{};
	viewport.width    = static_cast<float>(extent.width);
	viewport.height   = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	primary_command_buffer.set_viewport(0, {viewport});
	scene_subpass_ptr->set_viewport(viewport);

	VkRect2D scissor{};
	scissor.extent = extent;
	primary_command_buffer.set_scissor(0, {scissor});
	scene_subpass_ptr->set_scissor(scissor);

	const auto &queue = device->get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

	std::vector<vkb::CommandBuffer *> secondary_command_buffers;

	vkb::CommandBuffer *command_buffer = &primary_command_buffer;

	render(primary_command_buffer);

	// Draw gui
	if (gui)
	{
		if (use_secondary_command_buffers)
		{
			vkb::CommandBuffer &secondary_command_buffer = render_context->request_frame_command_buffer(queue, static_cast<vkb::CommandBuffer::ResetMode>(reuse_selection), VK_COMMAND_BUFFER_LEVEL_SECONDARY);

			secondary_command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &primary_command_buffer);

			command_buffer = &secondary_command_buffer;
		}

		command_buffer->set_viewport(0, {viewport});

		command_buffer->set_scissor(0, {scissor});

		gui->draw(*command_buffer);

		if (use_secondary_command_buffers)
		{
			command_buffer->end();

			secondary_command_buffers.push_back(command_buffer);
		}
	}

	if (use_secondary_command_buffers)
	{
		primary_command_buffer.execute_commands(secondary_command_buffers);
	}

	primary_command_buffer.end_render_pass();
}

CommandBufferUsage::SceneSubpassSecondary::SceneSubpassSecondary(vkb::RenderContext &render_context,
                                                                 vkb::ShaderSource &&vertex_shader, vkb::ShaderSource &&fragment_shader, vkb::sg::Scene &scene, vkb::sg::Camera &camera) :
    vkb::SceneSubpass{render_context, std::move(vertex_shader), std::move(fragment_shader), scene, camera}
{
}

void CommandBufferUsage::SceneSubpassSecondary::draw(vkb::CommandBuffer &primary_command_buffer)
{
	std::multimap<float, std::pair<vkb::sg::Node *, vkb::sg::SubMesh *>> opaque_nodes;
	std::multimap<float, std::pair<vkb::sg::Node *, vkb::sg::SubMesh *>> transparent_nodes;

	get_sorted_nodes(opaque_nodes, transparent_nodes);

	const auto &queue = render_context.get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

	VkCommandBufferUsageFlags secondary_usage_flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

	std::vector<vkb::CommandBuffer *> secondary_command_buffers;

	vkb::CommandBuffer *command_buffer = &primary_command_buffer;

	vkb::ColorBlendAttachmentState color_blend_attachment{};
	vkb::ColorBlendState           color_blend_state{};
	color_blend_state.attachments.resize(get_output_attachments().size());
	color_blend_state.attachments[0] = color_blend_attachment;

	// Draw opaque objects in front-to-back order
	// Note: sorting objects does not help on PowerVR, so it can be avoided to save CPU cycles
	for (auto node_it = opaque_nodes.begin(); node_it != opaque_nodes.end(); node_it++)
	{
		if (use_secondary_command_buffers)
		{
			vkb::CommandBuffer &secondary_command_buffer = render_context.request_frame_command_buffer(queue, command_buffer_reset_mode, VK_COMMAND_BUFFER_LEVEL_SECONDARY);

			secondary_command_buffer.begin(secondary_usage_flags, &primary_command_buffer);

			command_buffer = &secondary_command_buffer;
		}

		command_buffer->set_color_blend_state(color_blend_state);

		command_buffer->set_viewport(0, {viewport});

		command_buffer->set_scissor(0, {scissor});

		command_buffer->set_color_blend_state(color_blend_state);

		update_uniform(*command_buffer, *node_it->second.first);

		draw_submesh(*command_buffer, *node_it->second.second);

		if (use_secondary_command_buffers)
		{
			command_buffer->end();

			secondary_command_buffers.push_back(command_buffer);
		}
	}

	// Enable alpha blending
	color_blend_attachment.blend_enable           = VK_TRUE;
	color_blend_attachment.src_color_blend_factor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dst_color_blend_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.src_alpha_blend_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

	color_blend_state.attachments[0] = color_blend_attachment;

	// Draw transparent objects in back-to-front order
	for (auto node_it = transparent_nodes.rbegin(); node_it != transparent_nodes.rend(); node_it++)
	{
		if (use_secondary_command_buffers)
		{
			vkb::CommandBuffer &secondary_command_buffer = render_context.request_frame_command_buffer(queue, command_buffer_reset_mode, VK_COMMAND_BUFFER_LEVEL_SECONDARY);

			secondary_command_buffer.begin(secondary_usage_flags, &primary_command_buffer);

			command_buffer = &secondary_command_buffer;
		}

		command_buffer->set_viewport(0, {viewport});

		command_buffer->set_scissor(0, {scissor});

		command_buffer->set_color_blend_state(color_blend_state);

		command_buffer->set_depth_stencil_state(get_depth_stencil_state());

		update_uniform(*command_buffer, *node_it->second.first);

		draw_submesh(*command_buffer, *node_it->second.second);

		if (use_secondary_command_buffers)
		{
			command_buffer->end();

			secondary_command_buffers.push_back(command_buffer);
		}
	}

	if (use_secondary_command_buffers)
	{
		primary_command_buffer.execute_commands(secondary_command_buffers);
	}
}

void CommandBufferUsage::SceneSubpassSecondary::set_use_secondary_command_buffers(bool use_secondary)
{
	use_secondary_command_buffers = use_secondary;
}

void CommandBufferUsage::SceneSubpassSecondary::set_command_buffer_reset_mode(vkb::CommandBuffer::ResetMode reset_mode)
{
	command_buffer_reset_mode = reset_mode;
}

void CommandBufferUsage::SceneSubpassSecondary::set_viewport(VkViewport &viewport)
{
	this->viewport = viewport;
}

void CommandBufferUsage::SceneSubpassSecondary::set_scissor(VkRect2D &scissor)
{
	this->scissor = scissor;
}

std::unique_ptr<vkb::VulkanSample> create_command_buffer_usage()
{
	return std::make_unique<CommandBufferUsage>();
}
