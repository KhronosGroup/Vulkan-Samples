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

#include "pipeline_barriers.h"

#include "core/device.h"
#include "core/pipeline_layout.h"
#include "core/shader_module.h"
#include "filesystem/legacy.h"
#include "gltf_loader.h"
#include "gui.h"

#include "rendering/subpasses/forward_subpass.h"
#include "rendering/subpasses/lighting_subpass.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/perspective_camera.h"
#include "stats/stats.h"

PipelineBarriers::PipelineBarriers()
{
	auto &config = get_configuration();

	config.insert<vkb::IntSetting>(0, reinterpret_cast<int &>(dependency_type), DependencyType::BOTTOM_TO_TOP);
	config.insert<vkb::IntSetting>(1, reinterpret_cast<int &>(dependency_type), DependencyType::FRAG_TO_VERT);
	config.insert<vkb::IntSetting>(2, reinterpret_cast<int &>(dependency_type), DependencyType::FRAG_TO_FRAG);

#if defined(PLATFORM__MACOS) && TARGET_OS_IOS && TARGET_OS_SIMULATOR
	// On iOS Simulator use layer setting to disable MoltenVK Metal argument buffers
	add_instance_extension(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME, /*optional*/ true);

	VkLayerSettingEXT layerSetting;
	layerSetting.pLayerName   = "MoltenVK";
	layerSetting.pSettingName = "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS";
	layerSetting.type         = VK_LAYER_SETTING_TYPE_INT32_EXT;
	layerSetting.valueCount   = 1;

	// Make this static so layer setting reference remains valid after leaving constructor scope
	static const int32_t useMetalArgumentBuffers = 0;
	layerSetting.pValues                         = &useMetalArgumentBuffers;

	add_layer_setting(layerSetting);

	// On iOS Simulator also set environment variable as fallback in case layer settings not available at runtime with older SDKs
	// Will not work in batch mode, but is the best we can do short of using the deprecated MoltenVK private config API
	setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "0", 1);
#endif
}

bool PipelineBarriers::prepare(const vkb::ApplicationOptions &options)
{
	if (!VulkanSample::prepare(options))
	{
		return false;
	}

	load_scene("scenes/sponza/Sponza01.gltf");

	get_scene().clear_components<vkb::sg::Light>();

	auto light_pos   = glm::vec3(0.0f, 128.0f, -225.0f);
	auto light_color = glm::vec3(1.0, 1.0, 1.0);

	// Magic numbers used to offset lights in the Sponza scene
	for (int i = -2; i < 2; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			glm::vec3 pos = light_pos;
			pos.x += i * 400;
			pos.z += j * (225 + 140);
			pos.y = 8;

			for (int k = 0; k < 3; ++k)
			{
				pos.y = pos.y + (k * 100);

				light_color.x = static_cast<float>(rand()) / (RAND_MAX);
				light_color.y = static_cast<float>(rand()) / (RAND_MAX);
				light_color.z = static_cast<float>(rand()) / (RAND_MAX);

				vkb::sg::LightProperties props;
				props.color     = light_color;
				props.intensity = 0.2f;

				vkb::add_point_light(get_scene(), pos, props);
			}
		}
	}

	auto &camera_node = vkb::add_free_camera(get_scene(), "main_camera", get_render_context().get_surface_extent());
	camera            = &camera_node.get_component<vkb::sg::Camera>();

	auto geometry_vs = vkb::ShaderSource{"deferred/geometry.vert"};
	auto geometry_fs = vkb::ShaderSource{"deferred/geometry.frag"};

	auto gbuffer_pass = std::make_unique<vkb::GeometrySubpass>(get_render_context(), std::move(geometry_vs), std::move(geometry_fs), get_scene(), *camera);
	gbuffer_pass->set_output_attachments({1, 2, 3});
	gbuffer_pipeline.add_subpass(std::move(gbuffer_pass));
	gbuffer_pipeline.set_load_store(vkb::gbuffer::get_clear_store_all());

	auto lighting_vs = vkb::ShaderSource{"deferred/lighting.vert"};
	auto lighting_fs = vkb::ShaderSource{"deferred/lighting.frag"};

	auto lighting_subpass = std::make_unique<vkb::LightingSubpass>(get_render_context(), std::move(lighting_vs), std::move(lighting_fs), *camera, get_scene());
	lighting_subpass->set_input_attachments({1, 2, 3});
	lighting_pipeline.add_subpass(std::move(lighting_subpass));
	lighting_pipeline.set_load_store(vkb::gbuffer::get_load_all_store_swapchain());

	get_stats().request_stats({vkb::StatIndex::frame_times,
	                           vkb::StatIndex::gpu_vertex_cycles,
	                           vkb::StatIndex::gpu_fragment_cycles},
	                          vkb::CounterSamplingConfig{vkb::CounterSamplingMode::Continuous});

	create_gui(*window, &get_stats());

	return true;
}

void PipelineBarriers::prepare_render_context()
{
	get_render_context().prepare(1, [this](vkb::core::Image &&swapchain_image) { return create_render_target(std::move(swapchain_image)); });
}

std::unique_ptr<vkb::RenderTarget> PipelineBarriers::create_render_target(vkb::core::Image &&swapchain_image)
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

void PipelineBarriers::draw(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target)
{
	// POI
	//
	// Pipeline stages and access masks for all barriers are picked based on the sample's setting.
	//
	// The first set of barriers transitions images for the first render pass. Color images only need to be ready
	// at COLOR_ATTACHMENT_OUTPUT time (while the depth image needs EARLY_FRAGMENT_TESTS | LATE_FRAGMENT_TESTS).
	// More conservative barriers are shown, waiting for acquisition at either VERTEX_SHADER or even TOP_OF_PIPE.
	//

	auto &views = render_target.get_views();
	assert(1 < views.size());

	{
		// Image 0 is the swapchain
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		memory_barrier.src_access_mask = 0;

		switch (dependency_type)
		{
			case DependencyType::BOTTOM_TO_TOP:
				memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				memory_barrier.dst_access_mask = 0;
				break;
			case DependencyType::FRAG_TO_VERT:
				memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
				memory_barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
				break;
			case DependencyType::FRAG_TO_FRAG:
			default:
				memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				memory_barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
		}

		command_buffer.image_memory_barrier(views[0], memory_barrier);

		// Skip 1 as it is handled later as a depth-stencil attachment
		for (size_t i = 2; i < views.size(); ++i)
		{
			memory_barrier.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
			command_buffer.image_memory_barrier(views[i], memory_barrier);
		}
	}

	{
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		memory_barrier.src_access_mask = 0;

		switch (dependency_type)
		{
			case DependencyType::BOTTOM_TO_TOP:
				memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				memory_barrier.dst_access_mask = 0;
				break;
			case DependencyType::FRAG_TO_VERT:
				memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
				memory_barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
				break;
			case DependencyType::FRAG_TO_FRAG:
			default:
				memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				memory_barrier.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
		}

		command_buffer.image_memory_barrier(views[1], memory_barrier);
	}

	set_viewport_and_scissor(command_buffer, render_target.get_extent());

	gbuffer_pipeline.draw(command_buffer, get_render_context().get_active_frame().get_render_target());

	command_buffer.end_render_pass();

	// POI
	//
	// The second set of barriers transitions the G-buffer images to SHADER_READ_ONLY_OPTIMAL for the second render pass.
	// It also ensures proper synchronization between render passes. The most optimal set of barriers is from COLOR_ATTACHMENT_OUTPUT
	// to FRAGMENT_SHADER, as the images only need to be ready at fragment shading time for the second render pass.
	//
	// With an optimal set of barriers, tiled GPUs would be able to run vertex shading for the second render pass in parallel with
	// fragment shading for the first render pass. Again, more conservative barriers are shown, waiting for VERTEX_SHADER or even TOP_OF_PIPE.
	// Those barriers will flush the GPU's pipeline, causing serialization between vertex and fragment work, potentially affecting performance.
	//
	for (size_t i = 1; i < render_target.get_views().size(); ++i)
	{
		auto &view = render_target.get_views()[i];

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

		switch (dependency_type)
		{
			case DependencyType::BOTTOM_TO_TOP:
				barrier.src_stage_mask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				barrier.src_access_mask = 0;
				barrier.dst_stage_mask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				barrier.dst_access_mask = 0;
				break;
			case DependencyType::FRAG_TO_VERT:
				barrier.dst_stage_mask  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
				barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
				break;
			case DependencyType::FRAG_TO_FRAG:
			default:
				barrier.dst_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				barrier.dst_access_mask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
				break;
		}

		command_buffer.image_memory_barrier(view, barrier);
	}

	lighting_pipeline.draw(command_buffer, get_render_context().get_active_frame().get_render_target());

	if (has_gui())
	{
		get_gui().draw(command_buffer);
	}

	command_buffer.end_render_pass();

	{
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		memory_barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		command_buffer.image_memory_barrier(views[0], memory_barrier);
	}
}

void PipelineBarriers::draw_gui()
{
	int  lines         = 2;
	bool portrait_mode = (reinterpret_cast<vkb::sg::PerspectiveCamera *>(camera)->get_aspect_ratio() < 1.0f);

	if (portrait_mode)
	{
		// In portrait, break the radio buttons into two separate lines
		lines++;
	}

	get_gui().show_options_window(
	    /* body = */ [this, portrait_mode]() {
		    ImGui::Text("Pipeline barrier stages:");
		    ImGui::RadioButton("Bottom to top", reinterpret_cast<int *>(&dependency_type), DependencyType::BOTTOM_TO_TOP);
		    ImGui::SameLine();
		    ImGui::RadioButton("Frag to vert", reinterpret_cast<int *>(&dependency_type), DependencyType::FRAG_TO_VERT);

		    if (!portrait_mode)
		    {
			    ImGui::SameLine();
		    }

		    ImGui::RadioButton("Frag to frag", reinterpret_cast<int *>(&dependency_type), DependencyType::FRAG_TO_FRAG);
	    },
	    /* lines = */ lines);
}

std::unique_ptr<vkb::VulkanSampleC> create_pipeline_barriers()
{
	return std::make_unique<PipelineBarriers>();
}
