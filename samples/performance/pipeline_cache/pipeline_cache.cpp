/* Copyright (c) 2019-2023, Arm Limited and Contributors
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
#include "common/resource_caching.h"
#include "core/device.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/window.h"

#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/node.h"
#include "stats/stats.h"

#include "resource_cache.h"
#include "resource_record.h"
#include "resource_replay.h"
namespace
{
template <class T, class... A>
struct RecordHelper
{
	size_t record(vkb::ResourceRecord & /*recorder*/, A &.../*args*/)
	{
		return 0;
	}

	void index(vkb::ResourceRecord & /*recorder*/, size_t /*index*/, T & /*resource*/)
	{
	}
};

template <class... A>
struct RecordHelper<vkb::ShaderModule, A...>
{
	size_t record(vkb::ResourceRecord &recorder, A &...args)
	{
		return recorder.register_shader_module(args...);
	}

	void index(vkb::ResourceRecord &recorder, size_t index, vkb::ShaderModule &shader_module)
	{
		recorder.set_shader_module(index, shader_module);
	}
};

template <class... A>
struct RecordHelper<vkb::PipelineLayout, A...>
{
	size_t record(vkb::ResourceRecord &recorder, A &...args)
	{
		return recorder.register_pipeline_layout(args...);
	}

	void index(vkb::ResourceRecord &recorder, size_t index, vkb::PipelineLayout &pipeline_layout)
	{
		recorder.set_pipeline_layout(index, pipeline_layout);
	}
};

template <class... A>
struct RecordHelper<vkb::RenderPass, A...>
{
	size_t record(vkb::ResourceRecord &recorder, A &...args)
	{
		return recorder.register_render_pass(args...);
	}

	void index(vkb::ResourceRecord &recorder, size_t index, vkb::RenderPass &render_pass)
	{
		recorder.set_render_pass(index, render_pass);
	}
};

template <class... A>
struct RecordHelper<vkb::GraphicsPipeline, A...>
{
	size_t record(vkb::ResourceRecord &recorder, A &...args)
	{
		return recorder.register_graphics_pipeline(args...);
	}

	void index(vkb::ResourceRecord &recorder, size_t index, vkb::GraphicsPipeline &graphics_pipeline)
	{
		recorder.set_graphics_pipeline(index, graphics_pipeline);
	}
};

template <class T, class... A>
T &request_resource(vkb::Device &device, vkb::ResourceRecord &recorder, vkb::CacheMap<std::size_t, T> &resources, A &...args)
{
	auto it = resources.find_or_insert(vkb::inline_hash_param(args...), [&device, &recorder, &resources, &args...]() {
		T resource{device, args...};

		// If we do not have it already, create and cache it
		const char *res_type = typeid(T).name();
		size_t      res_id   = resources.size();

		LOGD("Building #{} cache object ({})", res_id, res_type);

		return resource;
	});

	RecordHelper<T, A...> helper;
	helper.index(recorder, helper.record(recorder, args...), it->second);

	return it->second;
}
}        // namespace

class PipelineCacheResourceCache : public vkb::ResourceCache
{
  public:
	explicit PipelineCacheResourceCache(vkb::Device &device) :
	    vkb::ResourceCache{device} {};

	vkb::ShaderModule &request_shader_module(VkShaderStageFlagBits stage, const vkb::ShaderSource &glsl_source, const vkb::ShaderVariant &shader_variant) override
	{
		std::string entry_point{"main"};
		return request_resource<vkb::ShaderModule>(device, recorder, state.shader_modules, stage, glsl_source, entry_point, shader_variant);
	}
	vkb::PipelineLayout &request_pipeline_layout(const std::vector<vkb::ShaderModule *> &shader_modules) override
	{
		return request_resource<vkb::PipelineLayout>(device, recorder, state.pipeline_layouts, shader_modules);
	}
	vkb::DescriptorSetLayout &request_descriptor_set_layout(const uint32_t set_index, const std::vector<vkb::ShaderModule *> &shader_modules, const std::vector<vkb::ShaderResource> &set_resources) override
	{
		return request_resource<vkb::DescriptorSetLayout>(device, recorder, state.descriptor_set_layouts, set_index, shader_modules, set_resources);
	}
	vkb::GraphicsPipeline &request_graphics_pipeline(vkb::PipelineState &pipeline_state) override
	{
		return request_resource<vkb::GraphicsPipeline>(device, recorder, state.graphics_pipelines, pipeline_cache, pipeline_state);
	}
	vkb::ComputePipeline &request_compute_pipeline(vkb::PipelineState &pipeline_state) override
	{
		return request_resource<vkb::ComputePipeline>(device, recorder, state.compute_pipelines, pipeline_cache, pipeline_state);
	}
	vkb::DescriptorSet &request_descriptor_set(vkb::DescriptorSetLayout &descriptor_set_layout, const BindingMap<VkDescriptorBufferInfo> &buffer_infos, const BindingMap<VkDescriptorImageInfo> &image_infos) override
	{
		auto &descriptor_pool = request_resource<vkb::DescriptorPool>(device, recorder, state.descriptor_pools, descriptor_set_layout);
		return request_resource<vkb::DescriptorSet>(device, recorder, state.descriptor_sets, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos);
	}
	vkb::RenderPass &request_render_pass(const std::vector<vkb::Attachment> &attachments, const std::vector<vkb::LoadStoreInfo> &load_store_infos, const std::vector<vkb::SubpassInfo> &subpasses) override
	{
		return request_resource<vkb::RenderPass>(device, recorder, state.render_passes, attachments, load_store_infos, subpasses);
	}
	vkb::Framebuffer &request_framebuffer(const vkb::RenderTarget &render_target, const vkb::RenderPass &render_pass) override
	{
		return request_resource<vkb::Framebuffer>(device, recorder, state.framebuffers, render_target, render_pass);
	}
	void clear_pipelines()
	{
		state.graphics_pipelines.clear();
		state.compute_pipelines.clear();
	}
	void set_pipeline_cache(VkPipelineCache in_pipeline_cache)
	{
		pipeline_cache = in_pipeline_cache;
	}
	void warmup(std::vector<uint8_t> &&data)
	{
		recorder.set_data(data);
		replayer.play(*this, recorder);
	}
	std::vector<uint8_t> serialize() const
	{
		return recorder.get_data();
	}

  private:
	VkPipelineCache pipeline_cache{VK_NULL_HANDLE};

	vkb::ResourceRecord recorder;

	vkb::ResourceReplay replayer;
};

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

	vkb::fs::write_temp(device->get_resource_cache<PipelineCacheResourceCache>().serialize(), "cache.data");
}

bool PipelineCache::prepare(const vkb::ApplicationOptions &options)
{
	if (!VulkanSample::prepare(options))
	{
		return false;
	}

	device->override_resource_cache(std::make_unique<PipelineCacheResourceCache>(*device));

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

	PipelineCacheResourceCache &resource_cache = device->get_resource_cache<PipelineCacheResourceCache>();

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
	auto start_time = std::chrono::high_resolution_clock::now();
	resource_cache.warmup(std::move(data_cache));
	auto end_time = std::chrono::high_resolution_clock::now();
	LOGI("Pipeline cache warmup took {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count());

	stats->request_stats({vkb::StatIndex::frame_times});

	float dpi_factor = window->get_dpi_factor();

	button_size.x = button_size.x * dpi_factor;
	button_size.y = button_size.y * dpi_factor;

	gui = std::make_unique<vkb::Gui>(*this, *window, stats.get());

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
			    PipelineCacheResourceCache &resource_cache = device->get_resource_cache<PipelineCacheResourceCache>();

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
			    device->get_resource_cache<PipelineCacheResourceCache>().clear_pipelines();
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
