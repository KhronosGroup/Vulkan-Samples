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

#include "pipeline_cache.h"

#include <imgui_internal.h>

#include "common/logging.h"
#include "core/device.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/node.h"
#include "stats.h"

PipelineCache::PipelineCache()
{
	auto &config = get_configuration();

	config.insert<vkb::BoolSetting>(0, enable_pipeline_cache, true);
	config.insert<vkb::BoolSetting>(1, enable_pipeline_cache, false);
}

PipelineCache::~PipelineCache()
{
	if (pipeline_cache != VK_NULL_HANDLE)
	{
		/* Get size of pipeline cache */
		size_t size{};
		VK_CHECK(vkGetPipelineCacheData(device->get_handle(), pipeline_cache, &size, nullptr));

		/* Get data of pipeline cache */
		std::vector<uint8_t> data(size);
		VK_CHECK(vkGetPipelineCacheData(device->get_handle(), pipeline_cache, &size, data.data()));

		/* Write pipeline cache data to a file in binary format */
		vkb::fs::write_temp(data, "pipeline_cache.data");

		/* Destroy Vulkan pipeline cache */
		vkDestroyPipelineCache(device->get_handle(), pipeline_cache, nullptr);
	}

	vkb::fs::write_temp(device->get_resource_cache().serialize(), "cache.data");
}

bool PipelineCache::prepare(vkb::Platform &platform)
{
	if (!VulkanSample::prepare(platform))
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
	VkPipelineCacheCreateInfo create_info{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
	create_info.initialDataSize = pipeline_data.size();
	create_info.pInitialData    = pipeline_data.data();

	/* Create Vulkan pipeline cache */
	VK_CHECK(vkCreatePipelineCache(device->get_handle(), &create_info, nullptr, &pipeline_cache));

	vkb::ResourceCache &resource_cache = device->get_resource_cache();

	// Use pipeline cache to store pipelines
	resource_cache.set_pipeline_cache(pipeline_cache);

	std::vector<uint8_t> data_cache;

	try
	{
		data_cache = vkb::fs::read_temp("cache.data");
	}
	catch (std::runtime_error &ex)
	{
		LOGW("No data cache found. {}", ex.what());
	}

	// Build all pipelines from a previous run
	resource_cache.warmup(data_cache);

	stats = std::make_unique<vkb::Stats>(std::set<vkb::StatIndex>{vkb::StatIndex::frame_times});

	float dpi_factor = platform.get_window().get_dpi_factor();

	button_size.x = button_size.x * dpi_factor;
	button_size.y = button_size.y * dpi_factor;

	gui = std::make_unique<vkb::Gui>(*this, platform.get_window());

	load_scene("scenes/sponza/Sponza01.gltf");

	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = &camera_node.get_component<vkb::sg::Camera>();

	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_subpass = std::make_unique<vkb::ForwardSubpass>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, *camera);

	auto render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_subpass));

	set_render_pipeline(std::move(render_pipeline));

	return true;
}

void PipelineCache::draw_gui()
{
	gui->show_options_window(
	    /* body = */ [this]() {
		    if (ImGui::Checkbox("Pipeline cache", &enable_pipeline_cache))
		    {
			    vkb::ResourceCache &resource_cache = device->get_resource_cache();

			    if (enable_pipeline_cache)
			    {
				    // Use pipeline cache to store pipelines
				    resource_cache.set_pipeline_cache(pipeline_cache);
			    }
			    else
			    {
				    // Don't use a pipeline cache
				    resource_cache.set_pipeline_cache(VK_NULL_HANDLE);
			    }
		    }

		    ImGui::SameLine();

		    if (ImGui::Button("Destroy Pipelines", button_size))
		    {
			    device->wait_idle();
			    device->get_resource_cache().clear_pipelines();
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

void PipelineCache::update(float delta_time)
{
	if (record_frame_time_next_frame)
	{
		rebuild_pipelines_frame_time_ms = delta_time * 1000.0f;
		record_frame_time_next_frame    = false;
	}

	VulkanSample::update(delta_time);
}

std::unique_ptr<vkb::VulkanSample> create_pipeline_cache()
{
	return std::make_unique<PipelineCache>();
}
