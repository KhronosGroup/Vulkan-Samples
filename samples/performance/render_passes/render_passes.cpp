/* Copyright (c) 2018-2020, Arm Limited and Contributors
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

#include "render_passes.h"

#include "common/vk_common.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "rendering/subpasses/forward_subpass.h"
#include "stats/stats.h"

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#	include "platform/android/android_platform.h"
#endif

RenderPassesSample::RenderPassesSample()
{
	auto &config = get_configuration();

	config.insert<vkb::BoolSetting>(0, cmd_clear, false);
	config.insert<vkb::IntSetting>(0, load.value, 0);
	config.insert<vkb::IntSetting>(0, store.value, 0);

	config.insert<vkb::BoolSetting>(1, cmd_clear, true);
	config.insert<vkb::IntSetting>(1, load.value, 1);
	config.insert<vkb::IntSetting>(1, store.value, 1);
}

void RenderPassesSample::reset_stats_view()
{
	if (load.value == VK_ATTACHMENT_LOAD_OP_LOAD)
	{
		gui->get_stats_view().reset_max_value(vkb::StatIndex::gpu_ext_read_bytes);
	}

	if (store.value == VK_ATTACHMENT_STORE_OP_STORE)
	{
		gui->get_stats_view().reset_max_value(vkb::StatIndex::gpu_ext_write_bytes);
	}
}

void RenderPassesSample::draw_gui()
{
	auto lines = radio_buttons.size() + 1 /* checkbox */;
	if (camera->get_aspect_ratio() < 1.0f)
	{
		// In portrait, show buttons below heading
		lines = lines * 2;
	}

	gui->show_options_window(
	    /* body = */ [this, lines]() {
		    // Checkbox vkCmdClear
		    ImGui::Checkbox("Use vkCmdClearAttachments (color)", &cmd_clear);

		    // For every option set
		    for (size_t i = 0; i < radio_buttons.size(); ++i)
		    {
			    // Avoid conflicts between buttons with identical labels
			    ImGui::PushID(vkb::to_u32(i));

			    auto &radio_button = radio_buttons[i];

			    ImGui::Text("%s: ", radio_button->description);

			    if (camera->get_aspect_ratio() > 1.0f)
			    {
				    // In landscape, show all options following the heading
				    ImGui::SameLine();
			    }

			    // For every option
			    for (size_t j = 0; j < radio_button->options.size(); ++j)
			    {
				    ImGui::RadioButton(radio_button->options[j], &radio_button->value, vkb::to_u32(j));

				    if (j < radio_button->options.size() - 1)
				    {
					    ImGui::SameLine();
				    }
			    }

			    ImGui::PopID();
		    }
	    },
	    /* lines = */ vkb::to_u32(lines));
}

bool RenderPassesSample::prepare(vkb::Platform &platform)
{
	if (!VulkanSample::prepare(platform))
	{
		return false;
	}

	stats->request_stats({vkb::StatIndex::gpu_fragment_cycles,
	                      vkb::StatIndex::gpu_ext_read_bytes,
	                      vkb::StatIndex::gpu_ext_write_bytes});

	load_scene("scenes/sponza/Sponza01.gltf");

	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = dynamic_cast<vkb::sg::PerspectiveCamera *>(&camera_node.get_component<vkb::sg::Camera>());

	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_subpass = std::make_unique<vkb::ForwardSubpass>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, *camera);

	auto render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_subpass));

	set_render_pipeline(std::move(render_pipeline));

	gui = std::make_unique<vkb::Gui>(*this, platform.get_window(), stats.get());

	return true;
}

void RenderPassesSample::draw_renderpass(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target)
{
	std::vector<vkb::LoadStoreInfo> load_store{2};

	// The load operation for the color attachment is selected by the user at run-time
	auto loadop            = static_cast<VkAttachmentLoadOp>(load.value);
	load_store[0].load_op  = loadop;
	load_store[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	load_store[1].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// Store operation for depth attachment is selected by the user at run-time
	load_store[1].store_op = static_cast<VkAttachmentStoreOp>(store.value);

	get_render_pipeline().set_load_store(load_store);

	auto &extent = render_target.get_extent();

	set_viewport_and_scissor(command_buffer, extent);

	auto &subpasses = render_pipeline->get_subpasses();
	command_buffer.begin_render_pass(render_target, load_store, render_pipeline->get_clear_value(), subpasses);

	if (cmd_clear)
	{
		VkClearAttachment attachment = {};
		// Clear color only
		attachment.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
		attachment.clearValue      = {0, 0, 0};
		attachment.colorAttachment = 0;

		VkClearRect rect = {};
		rect.layerCount  = 1;
		rect.rect.extent = extent;

		command_buffer.clear(attachment, rect);
	}

	subpasses.at(0)->draw(command_buffer);

	gui->draw(command_buffer);

	command_buffer.end_render_pass();
}

std::unique_ptr<vkb::VulkanSample> create_render_passes()
{
	return std::make_unique<RenderPassesSample>();
}
