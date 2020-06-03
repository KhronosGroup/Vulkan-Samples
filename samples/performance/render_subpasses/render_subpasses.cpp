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

#include "render_subpasses.h"

#include "common/vk_common.h"
#include "platform/platform.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_context.h"
#include "rendering/render_pipeline.h"
#include "rendering/subpasses/geometry_subpass.h"
#include "rendering/subpasses/lighting_subpass.h"
#include "scene_graph/node.h"

RenderSubpasses::RenderSubpasses()
{
	auto &config = get_configuration();

	// Good settings
	config.insert<vkb::IntSetting>(0, configs[Config::RenderTechnique].value, 0);
	config.insert<vkb::IntSetting>(0, configs[Config::TransientAttachments].value, 0);
	config.insert<vkb::IntSetting>(0, configs[Config::GBufferSize].value, 0);

	// Use two render passes
	config.insert<vkb::IntSetting>(1, configs[Config::RenderTechnique].value, 1);
	config.insert<vkb::IntSetting>(1, configs[Config::TransientAttachments].value, 0);
	config.insert<vkb::IntSetting>(1, configs[Config::GBufferSize].value, 0);

	// Disable transient attachments
	config.insert<vkb::IntSetting>(2, configs[Config::RenderTechnique].value, 0);
	config.insert<vkb::IntSetting>(2, configs[Config::TransientAttachments].value, 1);
	config.insert<vkb::IntSetting>(2, configs[Config::GBufferSize].value, 0);

	// Increase G-buffer size
	config.insert<vkb::IntSetting>(3, configs[Config::RenderTechnique].value, 0);
	config.insert<vkb::IntSetting>(3, configs[Config::TransientAttachments].value, 0);
	config.insert<vkb::IntSetting>(3, configs[Config::GBufferSize].value, 1);
}

std::unique_ptr<vkb::RenderTarget> RenderSubpasses::create_render_target(vkb::core::Image &&swapchain_image)
{
	auto &device = swapchain_image.get_device();
	auto &extent = swapchain_image.get_extent();

	// G-Buffer should fit 128-bit budget for buffer color storage
	// in order to enable subpasses merging by the driver
	// Light (swapchain_image) RGBA8_UNORM   (32-bit)
	// Albedo                  RGBA8_UNORM   (32-bit)
	// Normal                  RGB10A2_UNORM (32-bit)

	vkb::core::Image depth_image{device,
	                             extent,
	                             vkb::get_suitable_depth_format(swapchain_image.get_device().get_gpu().get_handle()),
	                             VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | rt_usage_flags,
	                             VMA_MEMORY_USAGE_GPU_ONLY};

	vkb::core::Image albedo_image{device,
	                              extent,
	                              albedo_format,
	                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | rt_usage_flags,
	                              VMA_MEMORY_USAGE_GPU_ONLY};

	vkb::core::Image normal_image{device,
	                              extent,
	                              normal_format,
	                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | rt_usage_flags,
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

void RenderSubpasses::prepare_render_context()
{
	get_render_context().prepare(1, [this](vkb::core::Image &&swapchain_image) { return create_render_target(std::move(swapchain_image)); });
}

bool RenderSubpasses::prepare(vkb::Platform &platform)
{
	if (!VulkanSample::prepare(platform))
	{
		return false;
	}

	std::set<VkImageUsageFlagBits> usage = {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT};
	get_render_context().update_swapchain(usage);

	load_scene("scenes/sponza/Sponza01.gltf");

	scene->clear_components<vkb::sg::Light>();

	auto light_pos   = glm::vec3(0.0f, 128.0f, -225.0f);
	auto light_color = glm::vec3(1.0, 1.0, 1.0);

	// Magic numbers used to offset lights in the Sponza scene
	for (int i = -4; i < 4; ++i)
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

				vkb::add_point_light(*scene, pos, props);
			}
		}
	}

	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = dynamic_cast<vkb::sg::PerspectiveCamera *>(&camera_node.get_component<vkb::sg::Camera>());

	render_pipeline = create_one_renderpass_two_subpasses();

	geometry_render_pipeline = create_geometry_renderpass();
	lighting_render_pipeline = create_lighting_renderpass();

	// Enable stats
	stats->request_stats({vkb::StatIndex::gpu_fragment_jobs,
	                      vkb::StatIndex::gpu_tiles,
	                      vkb::StatIndex::gpu_ext_read_bytes,
	                      vkb::StatIndex::gpu_ext_write_bytes});

	// Enable gui
	gui = std::make_unique<vkb::Gui>(*this, platform.get_window(), stats.get());

	return true;
}

void RenderSubpasses::update(float delta_time)
{
	// Check whether the user changed the render technique
	if (configs[Config::RenderTechnique].value != last_render_technique)
	{
		LOGI("Changing render technique");
		last_render_technique = configs[Config::RenderTechnique].value;

		// Reset frames, their synchronization objects and their command buffers
		for (auto &frame : get_render_context().get_render_frames())
		{
			frame->reset();
		}
	}

	// Check whether the user switched the attachment or the G-buffer option
	if (configs[Config::TransientAttachments].value != last_transient_attachment ||
	    configs[Config::GBufferSize].value != last_g_buffer_size)
	{
		// If attachment option has changed
		if (configs[Config::TransientAttachments].value != last_transient_attachment)
		{
			rt_usage_flags = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

			// If attachment should be transient
			if (configs[Config::TransientAttachments].value == 0)
			{
				rt_usage_flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
			}
			else
			{
				LOGI("Creating non transient attachments");
			}
			last_transient_attachment = configs[Config::TransientAttachments].value;
		}

		// It G-buffer option has changed
		if (configs[Config::GBufferSize].value != last_g_buffer_size)
		{
			if (configs[Config::GBufferSize].value == 0)
			{
				// Use less bits
				albedo_format = VK_FORMAT_R8G8B8A8_UNORM;                  // 32-bit
				normal_format = VK_FORMAT_A2B10G10R10_UNORM_PACK32;        // 32-bit
			}
			else
			{
				// Use more bits
				albedo_format = VK_FORMAT_R16G16B16A16_SFLOAT;        // 64-bit
				normal_format = VK_FORMAT_R16G16B16A16_SFLOAT;        // 64-bit
			}

			last_g_buffer_size = configs[Config::GBufferSize].value;
		}

		// Reset frames, their synchronization objects and their command buffers
		for (auto &frame : get_render_context().get_render_frames())
		{
			frame->reset();
		}

		LOGI("Recreating render target");
		render_context->recreate();
	}

	VulkanSample::update(delta_time);
}

void RenderSubpasses::draw_gui()
{
	auto lines = configs.size();
	if (camera->get_aspect_ratio() < 1.0f)
	{
		// In portrait, show buttons below heading
		lines = lines * 2;
	}

	gui->show_options_window(
	    /* body = */ [this, lines]() {
		    // Create a line for every config
		    for (size_t i = 0; i < configs.size(); ++i)
		    {
			    // Avoid conflicts between buttons with identical labels
			    ImGui::PushID(vkb::to_u32(i));

			    auto &config = configs[i];

			    ImGui::Text("%s: ", config.description);

			    if (camera->get_aspect_ratio() > 1.0f)
			    {
				    // In landscape, show all options following the heading
				    ImGui::SameLine();
			    }

			    // Create a radio button for every option
			    for (size_t j = 0; j < config.options.size(); ++j)
			    {
				    ImGui::RadioButton(config.options[j], &config.value, vkb::to_u32(j));

				    // Keep it on the same line til the last one
				    if (j < config.options.size() - 1)
				    {
					    ImGui::SameLine();
				    }
			    }

			    ImGui::PopID();
		    }
	    },
	    /* lines = */ vkb::to_u32(lines));
}

std::unique_ptr<vkb::RenderPipeline> RenderSubpasses::create_one_renderpass_two_subpasses()
{
	// Geometry subpass
	auto geometry_vs   = vkb::ShaderSource{"deferred/geometry.vert"};
	auto geometry_fs   = vkb::ShaderSource{"deferred/geometry.frag"};
	auto scene_subpass = std::make_unique<vkb::GeometrySubpass>(get_render_context(), std::move(geometry_vs), std::move(geometry_fs), *scene, *camera);

	// Outputs are depth, albedo, and normal
	scene_subpass->set_output_attachments({1, 2, 3});

	// Lighting subpass
	auto lighting_vs      = vkb::ShaderSource{"deferred/lighting.vert"};
	auto lighting_fs      = vkb::ShaderSource{"deferred/lighting.frag"};
	auto lighting_subpass = std::make_unique<vkb::LightingSubpass>(get_render_context(), std::move(lighting_vs), std::move(lighting_fs), *camera, *scene);

	// Inputs are depth, albedo, and normal from the geometry subpass
	lighting_subpass->set_input_attachments({1, 2, 3});

	// Create subpasses pipeline
	std::vector<std::unique_ptr<vkb::Subpass>> subpasses{};
	subpasses.push_back(std::move(scene_subpass));
	subpasses.push_back(std::move(lighting_subpass));

	auto render_pipeline = std::make_unique<vkb::RenderPipeline>(std::move(subpasses));

	render_pipeline->set_load_store(vkb::gbuffer::get_clear_all_store_swapchain());

	render_pipeline->set_clear_value(vkb::gbuffer::get_clear_value());

	return render_pipeline;
}

std::unique_ptr<vkb::RenderPipeline> RenderSubpasses::create_geometry_renderpass()
{
	// Geometry subpass
	auto geometry_vs   = vkb::ShaderSource{"deferred/geometry.vert"};
	auto geometry_fs   = vkb::ShaderSource{"deferred/geometry.frag"};
	auto scene_subpass = std::make_unique<vkb::GeometrySubpass>(get_render_context(), std::move(geometry_vs), std::move(geometry_fs), *scene, *camera);

	// Outputs are depth, albedo, and normal
	scene_subpass->set_output_attachments({1, 2, 3});

	// Create geomtry pipeline
	std::vector<std::unique_ptr<vkb::Subpass>> scene_subpasses{};
	scene_subpasses.push_back(std::move(scene_subpass));

	auto geometry_render_pipeline = std::make_unique<vkb::RenderPipeline>(std::move(scene_subpasses));

	geometry_render_pipeline->set_load_store(vkb::gbuffer::get_clear_store_all());

	geometry_render_pipeline->set_clear_value(vkb::gbuffer::get_clear_value());

	return geometry_render_pipeline;
}

std::unique_ptr<vkb::RenderPipeline> RenderSubpasses::create_lighting_renderpass()
{
	// Lighting subpass
	auto lighting_vs      = vkb::ShaderSource{"deferred/lighting.vert"};
	auto lighting_fs      = vkb::ShaderSource{"deferred/lighting.frag"};
	auto lighting_subpass = std::make_unique<vkb::LightingSubpass>(get_render_context(), std::move(lighting_vs), std::move(lighting_fs), *camera, *scene);

	// Inputs are depth, albedo, and normal from the geometry subpass
	lighting_subpass->set_input_attachments({1, 2, 3});
	// Create lighting pipeline
	std::vector<std::unique_ptr<vkb::Subpass>> lighting_subpasses{};
	lighting_subpasses.push_back(std::move(lighting_subpass));

	auto lighting_render_pipeline = std::make_unique<vkb::RenderPipeline>(std::move(lighting_subpasses));

	lighting_render_pipeline->set_load_store(vkb::gbuffer::get_load_all_store_swapchain());

	lighting_render_pipeline->set_clear_value(vkb::gbuffer::get_clear_value());

	return lighting_render_pipeline;
}

void RenderSubpasses::draw_pipeline(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target, vkb::RenderPipeline &render_pipeline, vkb::Gui *gui)
{
	set_viewport_and_scissor(command_buffer, render_target.get_extent());

	render_pipeline.draw(command_buffer, render_target);

	if (gui)
	{
		gui->draw(command_buffer);
	}

	command_buffer.end_render_pass();
}

void RenderSubpasses::draw_render_subpasses(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target)
{
	draw_pipeline(command_buffer, render_target, *render_pipeline, gui.get());
}

void RenderSubpasses::draw_renderpasses(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target)
{
	// First render pass (no gui)
	draw_pipeline(command_buffer, render_target, *geometry_render_pipeline);

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

	// Second render pass
	draw_pipeline(command_buffer, render_target, *lighting_render_pipeline, gui.get());
}

void RenderSubpasses::draw_renderpass(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target)
{
	if (configs[Config::RenderTechnique].value == 0)
	{
		// Efficient way
		draw_render_subpasses(command_buffer, render_target);
	}
	else
	{
		// Inefficient way
		draw_renderpasses(command_buffer, render_target);
	}
}

std::unique_ptr<vkb::VulkanSample> create_render_subpasses()
{
	return std::make_unique<RenderSubpasses>();
}
