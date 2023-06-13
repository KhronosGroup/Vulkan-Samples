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

#include "swapchain_images.h"

#include "core/device.h"
#include "core/pipeline_layout.h"
#include "core/shader_module.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"

#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/pbr_material.h"
#include "stats/stats.h"

SwapchainImages::SwapchainImages()
{
	auto &config = get_configuration();

	config.insert<vkb::IntSetting>(0, swapchain_image_count, 3);
	config.insert<vkb::IntSetting>(1, swapchain_image_count, 2);
}

bool SwapchainImages::prepare(const vkb::ApplicationOptions &options)
{
	if (!VulkanSample::prepare(options))
	{
		return false;
	}

	load_scene("scenes/sponza/Sponza01.gltf");

	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = &camera_node.get_component<vkb::sg::Camera>();

	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_subpass = std::make_unique<vkb::ForwardSubpass>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, *camera);

	auto render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_subpass));

	set_render_pipeline(std::move(render_pipeline));

	stats->request_stats({vkb::StatIndex::frame_times});
	gui = std::make_unique<vkb::Gui>(*this, *window, stats.get());

	return true;
}

void SwapchainImages::update(float delta_time)
{
	// Process GUI input
	if (swapchain_image_count != last_swapchain_image_count)
	{
		get_device().wait_idle();

		// Create a new swapchain with a new swapchain image count
		render_context->update_swapchain(swapchain_image_count);

		last_swapchain_image_count = swapchain_image_count;
	}

	VulkanSample::update(delta_time);
}

void SwapchainImages::draw_gui()
{
	gui->show_options_window(
	    /* body = */ [this]() {
		    ImGui::RadioButton("Double buffering", &swapchain_image_count, 2);
		    ImGui::SameLine();
		    ImGui::RadioButton("Triple buffering", &swapchain_image_count, 3);
		    ImGui::SameLine();
	    },
	    /* lines = */ 1);
}

std::unique_ptr<vkb::VulkanSample> create_swapchain_images()
{
	return std::make_unique<SwapchainImages>();
}
