/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_pipeline_cache.h"

#include "common/hpp_utils.h"
#include "hpp_gui.h"
#include "rendering/subpasses/hpp_forward_subpass.h"

HPPPipelineCache::HPPPipelineCache()
{
	auto &config = get_configuration();

	config.insert<vkb::BoolSetting>(0, enable_pipeline_cache, true);
	config.insert<vkb::BoolSetting>(1, enable_pipeline_cache, false);
}

HPPPipelineCache::~HPPPipelineCache()
{
	if (pipeline_cache)
	{
		/* Get data of pipeline cache */
		std::vector<uint8_t> data = get_device().get_handle().getPipelineCacheData(pipeline_cache);

		/* Write pipeline cache data to a file in binary format */
		vkb::fs::write_temp(data, "pipeline_cache.data");

		/* Destroy Vulkan pipeline cache */
		get_device().get_handle().destroyPipelineCache(pipeline_cache);
	}

	vkb::fs::write_temp(get_device().get_resource_cache().serialize(), "hpp_cache.data");
}

bool HPPPipelineCache::prepare(const vkb::ApplicationOptions &options)
{
	if (!vkb::VulkanSampleCpp::prepare(options))
	{
		return false;
	}

	/* Try to read pipeline cache file if exists */
	std::vector<uint8_t> pipeline_data;
	try
	{
		pipeline_data = vkb::fs::read_temp("pipeline_cache.data");
	}
	catch (std::runtime_error &ex)
	{
		LOGW("No pipeline cache found. {}", ex.what());
	}

	/* Add initial pipeline cache data from the cached file */
	/* Note: unfortunately, no compiler can detect the correct data type from the "natural" constructor here:                   */
	/*	vk::PipelineCacheCreateInfo pipeline_cache_create_info({}, pipeline_data);                                              */
	/* This is due to the templatized second argument. You could help the compiler by a cast                                    */
	/*	vk::PipelineCacheCreateInfo pipeline_cache_create_info({}, vk::ArrayProxyNoTemporaries<const uint8_t>(pipeline_data));  */
	/* But, obviously, this looks a bit awkward. Instead, you can use the simple two-argument approach with size and pointer:   */
	vk::PipelineCacheCreateInfo pipeline_cache_create_info({}, pipeline_data.size(), pipeline_data.data());

	/* Create Vulkan pipeline cache */
	pipeline_cache = get_device().get_handle().createPipelineCache(pipeline_cache_create_info);

	vkb::HPPResourceCache &resource_cache = get_device().get_resource_cache();

	/* Use pipeline cache to store pipelines */
	resource_cache.set_pipeline_cache(pipeline_cache);

	std::vector<uint8_t> data_cache;
	try
	{
		data_cache = vkb::fs::read_temp("hpp_cache.data");
	}
	catch (std::runtime_error &ex)
	{
		LOGW("No data cache found. {}", ex.what());
	}

	/* Build all pipelines from a previous run */
	resource_cache.warmup(data_cache);

	get_stats().request_stats({vkb::StatIndex::frame_times});

	float dpi_factor = window->get_dpi_factor();

	button_size.x = button_size.x * dpi_factor;
	button_size.y = button_size.y * dpi_factor;

	create_gui(*window, &get_stats());

	load_scene("scenes/sponza/Sponza01.gltf");

	auto &camera_node = vkb::common::add_free_camera(get_scene(), "main_camera", get_render_context().get_surface_extent());
	camera            = &camera_node.get_component<vkb::sg::Camera>();

	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_subpass = std::make_unique<vkb::rendering::subpasses::HPPForwardSubpass>(
        get_render_context(), std::move(vert_shader), std::move(frag_shader), get_scene(), *camera);

	auto render_pipeline = std::make_unique<vkb::rendering::HPPRenderPipeline>();
	render_pipeline->add_subpass(std::move(scene_subpass));

	set_render_pipeline(std::move(render_pipeline));

	return true;
}

void HPPPipelineCache::draw_gui()
{
	get_gui().show_options_window(
	    /* body = */ [this]() {
		    if (ImGui::Checkbox("Pipeline cache", &enable_pipeline_cache))
		    {
			    get_device().get_resource_cache().set_pipeline_cache(enable_pipeline_cache ? pipeline_cache : nullptr);
		    }

		    ImGui::SameLine();

		    if (ImGui::Button("Destroy Pipelines", button_size))
		    {
			    get_device().get_handle().waitIdle();
			    get_device().get_resource_cache().clear_pipelines();
			    record_frame_time_next_frame = true;
		    }

		    if (rebuild_pipelines_frame_time_ms > 0.0f)
		    {
			    ImGui::Text("Pipeline rebuild frame time: %.1f ms", rebuild_pipelines_frame_time_ms);
		    }
		    else
		    {
			    ImGui::Text("Pipeline rebuild frame time: N/A");
		    }
	    },
	    /* lines = */ 2);
}

void HPPPipelineCache::update(float delta_time)
{
	if (record_frame_time_next_frame)
	{
		rebuild_pipelines_frame_time_ms = delta_time * 1000.0f;
		record_frame_time_next_frame    = false;
	}

	vkb::VulkanSampleCpp::update(delta_time);
}

std::unique_ptr<vkb::VulkanSampleCpp> create_hpp_pipeline_cache()
{
	return std::make_unique<HPPPipelineCache>();
}
