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

#include "layout_transitions.h"

#include "core/device.h"
#include "core/pipeline_layout.h"
#include "core/shader_module.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/platform.h"
#include "rendering/subpasses/forward_subpass.h"
#include "rendering/subpasses/lighting_subpass.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/pbr_material.h"
#include "stats/stats.h"

LayoutTransitions::LayoutTransitions()
{
	auto &config = get_configuration();

	config.insert<vkb::IntSetting>(0, reinterpret_cast<int &>(layout_transition_type), LayoutTransitionType::UNDEFINED);
	config.insert<vkb::IntSetting>(1, reinterpret_cast<int &>(layout_transition_type), LayoutTransitionType::LAST_LAYOUT);
}

bool LayoutTransitions::prepare(vkb::Platform &platform)
{
	if (!VulkanSample::prepare(platform))
	{
		return false;
	}

	load_scene("/scenes/sponza/Sponza01.gltf");

	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = &camera_node.get_component<vkb::sg::Camera>();

	auto geometry_vs = vkb::ShaderSource{"deferred/geometry.vert"};
	auto geometry_fs = vkb::ShaderSource{"deferred/geometry.frag"};

	std::unique_ptr<vkb::Subpass> gbuffer_pass = std::make_unique<vkb::GeometrySubpass>(get_render_context(), std::move(geometry_vs), std::move(geometry_fs), *scene, *camera);
	gbuffer_pass->set_output_attachments({1, 2, 3});
	gbuffer_pipeline.add_subpass(std::move(gbuffer_pass));
	gbuffer_pipeline.set_load_store(vkb::gbuffer::get_clear_store_all());

	auto lighting_vs = vkb::ShaderSource{"deferred/lighting.vert"};
	auto lighting_fs = vkb::ShaderSource{"deferred/lighting.frag"};

	std::unique_ptr<vkb::Subpass> lighting_subpass = std::make_unique<vkb::LightingSubpass>(get_render_context(), std::move(lighting_vs), std::move(lighting_fs), *camera, *scene);
	lighting_subpass->set_input_attachments({1, 2, 3});
	lighting_pipeline.add_subpass(std::move(lighting_subpass));
	lighting_pipeline.set_load_store(vkb::gbuffer::get_load_all_store_swapchain());

	stats->request_stats({vkb::StatIndex::gpu_killed_tiles,
	                      vkb::StatIndex::gpu_ext_write_bytes});

	gui = std::make_unique<vkb::Gui>(*this, platform.get_window(), stats.get());

	return true;
}

void LayoutTransitions::prepare_render_context()
{
	get_render_context().prepare(1, [this](vkb::core::Image &&swapchain_image) { return create_render_target(std::move(swapchain_image)); });
}

std::unique_ptr<vkb::RenderTarget> LayoutTransitions::create_render_target(vkb::core::Image &&swapchain_image)
{
	auto &device = swapchain_image.get_device();
	auto &extent = swapchain_image.get_extent();

	vkb::core::Image depth_image{device,
	                             extent,
	                             vkb::get_suitable_depth_format(swapchain_image.get_device().get_gpu().get_handle()),
	                             VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
	                             VMA_MEMORY_USAGE_GPU_ONLY};

	vkb::core::Image albedo_image{device,
	                              extent,
	                              VK_FORMAT_R8G8B8A8_UNORM,
	                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
	                              VMA_MEMORY_USAGE_GPU_ONLY};

	vkb::core::Image normal_image{device,
	                              extent,
	                              VK_FORMAT_A2B10G10R10_UNORM_PACK32,
	                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
	                              VMA_MEMORY_USAGE_GPU_ONLY};

	std::vector<vkb::core::Image> images;

	// Attachment 0
	images.push_back(std::move(swapchain_image));

	// Attachment 1
	images.push_back(std::move(depth_image));

	// Attachment 2
	images.push_back(std::move(albedo_image));

	// Attachment 3
	images.push_back(std::move(normal_image));

	return std::make_unique<vkb::RenderTarget>(std::move(images));
}

VkImageLayout LayoutTransitions::pick_old_layout(VkImageLayout last_layout)
{
	return (layout_transition_type == LayoutTransitionType::UNDEFINED) ?
	           VK_IMAGE_LAYOUT_UNDEFINED :
	           last_layout;
}

void LayoutTransitions::draw(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target)
{
	// POI
	//
	// The old_layout for each memory barrier is picked based on the sample's setting.
	// We either use the last valid layout for the image or UNDEFINED.
	//
	// Both approaches are functionally correct, as we are clearing the images anyway,
	// but using the last valid layout can give the driver more optimization opportunities.
	//

	auto &views = render_target.get_views();

	{
		// Image 0 is the swapchain
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = pick_old_layout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		memory_barrier.src_access_mask = 0;
		memory_barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		command_buffer.image_memory_barrier(views.at(0), memory_barrier);

		// Skip 1 as it is handled later as a depth-stencil attachment
		for (size_t i = 2; i < views.size(); ++i)
		{
			memory_barrier.old_layout = pick_old_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			command_buffer.image_memory_barrier(views.at(i), memory_barrier);
		}
	}

	{
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = pick_old_layout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		memory_barrier.src_access_mask = 0;
		memory_barrier.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		command_buffer.image_memory_barrier(views.at(1), memory_barrier);
	}

	auto &extent = render_target.get_extent();

	VkViewport viewport{};
	viewport.width    = static_cast<float>(extent.width);
	viewport.height   = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	command_buffer.set_viewport(0, {viewport});

	VkRect2D scissor{};
	scissor.extent = extent;
	command_buffer.set_scissor(0, {scissor});

	gbuffer_pipeline.draw(command_buffer, get_render_context().get_active_frame().get_render_target());

	command_buffer.end_render_pass();

	// Memory barriers needed
	for (size_t i = 1; i < render_target.get_views().size(); ++i)
	{
		auto &view = render_target.get_views().at(i);

		vkb::ImageMemoryBarrier barrier;

		if (i == 1)
		{
			barrier.old_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			barrier.new_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

			barrier.src_stage_mask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			barrier.src_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}
		else
		{
			barrier.old_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			barrier.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}

		barrier.dst_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		barrier.dst_access_mask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

		command_buffer.image_memory_barrier(view, barrier);
	}

	lighting_pipeline.draw(command_buffer, get_render_context().get_active_frame().get_render_target());

	if (gui)
	{
		gui->draw(command_buffer);
	}

	command_buffer.end_render_pass();

	{
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		memory_barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		command_buffer.image_memory_barrier(views.at(0), memory_barrier);
	}
}

void LayoutTransitions::draw_gui()
{
	gui->show_options_window(
	    /* body = */ [this]() {
		    ImGui::Text("Transition images from:");
		    ImGui::RadioButton("Undefined layout", reinterpret_cast<int *>(&layout_transition_type), LayoutTransitionType::UNDEFINED);
		    ImGui::SameLine();
		    ImGui::RadioButton("Current layout", reinterpret_cast<int *>(&layout_transition_type), LayoutTransitionType::LAST_LAYOUT);
		    ImGui::SameLine();
	    },
	    /* lines = */ 2);
}

std::unique_ptr<vkb::VulkanSample> create_layout_transitions()
{
	return std::make_unique<LayoutTransitions>();
}
