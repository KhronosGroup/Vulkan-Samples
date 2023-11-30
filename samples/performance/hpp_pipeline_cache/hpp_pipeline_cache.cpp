/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "common/hpp_resource_caching.h"
#include "common/hpp_utils.h"
#include "hpp_gui.h"
#include "rendering/subpasses/hpp_forward_subpass.h"

#include "hpp_resource_cache.h"

#include "hpp_resource_record.h"
#include "hpp_resource_replay.h"

namespace
{
template <class T, class... A>
struct HPPRecordHelper
{
	size_t record(vkb::HPPResourceRecord & /*recorder*/, A &.../*args*/)
	{
		return 0;
	}

	void index(vkb::HPPResourceRecord & /*recorder*/, size_t /*index*/, T & /*resource*/)
	{}
};

template <class... A>
struct HPPRecordHelper<vkb::core::HPPShaderModule, A...>
{
	size_t record(vkb::HPPResourceRecord &recorder, A &...args)
	{
		return recorder.register_shader_module(args...);
	}

	void index(vkb::HPPResourceRecord &recorder, size_t index, vkb::core::HPPShaderModule &shader_module)
	{
		recorder.set_shader_module(index, shader_module);
	}
};

template <class... A>
struct HPPRecordHelper<vkb::core::HPPPipelineLayout, A...>
{
	size_t record(vkb::HPPResourceRecord &recorder, A &...args)
	{
		return recorder.register_pipeline_layout(args...);
	}

	void index(vkb::HPPResourceRecord &recorder, size_t index, vkb::core::HPPPipelineLayout &pipeline_layout)
	{
		recorder.set_pipeline_layout(index, pipeline_layout);
	}
};

template <class... A>
struct HPPRecordHelper<vkb::core::HPPRenderPass, A...>
{
	size_t record(vkb::HPPResourceRecord &recorder, A &...args)
	{
		return recorder.register_render_pass(args...);
	}

	void index(vkb::HPPResourceRecord &recorder, size_t index, vkb::core::HPPRenderPass &render_pass)
	{
		recorder.set_render_pass(index, render_pass);
	}
};

template <class... A>
struct HPPRecordHelper<vkb::core::HPPGraphicsPipeline, A...>
{
	size_t record(vkb::HPPResourceRecord &recorder, A &...args)
	{
		return recorder.register_graphics_pipeline(args...);
	}

	void index(vkb::HPPResourceRecord &recorder, size_t index, vkb::core::HPPGraphicsPipeline &graphics_pipeline)
	{
		recorder.set_graphics_pipeline(index, graphics_pipeline);
	}
};

template <class T, class... A>
T &request_resource(vkb::core::HPPDevice &device, vkb::HPPResourceRecord &recorder, vkb::CacheMap<std::size_t, T> &resources, A &...args)
{
	auto it = resources.find_or_insert(vkb::inline_hash_param(args...), [&device, &recorder, &resources, &args...]() {
		T resource{device, args...};

		HPPRecordHelper<T, A...> helper;
		helper.index(recorder, resources.size(), resource);

		return resource;
	});
	return it->second;
}
}        // namespace

// A cache which hydrates its resources from an external source
class HPPPipelineCacheResourceCache : public vkb::HPPResourceCache
{
  public:
	explicit HPPPipelineCacheResourceCache(vkb::core::HPPDevice &device) :
	    vkb::HPPResourceCache(device)
	{}

	vkb::core::HPPComputePipeline &request_compute_pipeline(vkb::rendering::HPPPipelineState &pipeline_state) override
	{
		return request_resource(device, recorder, state.compute_pipelines, pipeline_cache, pipeline_state);
	}
	vkb::core::HPPDescriptorSet &request_descriptor_set(vkb::core::HPPDescriptorSetLayout &descriptor_set_layout, const BindingMap<vk::DescriptorBufferInfo> &buffer_infos, const BindingMap<vk::DescriptorImageInfo> &image_infos) override
	{
		auto &descriptor_pool = request_resource(device, recorder, state.descriptor_pools, descriptor_set_layout);
		return request_resource(device, recorder, state.descriptor_sets, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos);
	}
	vkb::core::HPPDescriptorSetLayout &request_descriptor_set_layout(const uint32_t set_index, const std::vector<vkb::core::HPPShaderModule *> &shader_modules, const std::vector<vkb::core::HPPShaderResource> &set_resources) override
	{
		return request_resource(device, recorder, state.descriptor_set_layouts, set_index, shader_modules, set_resources);
	}
	vkb::core::HPPFramebuffer &request_framebuffer(const vkb::rendering::HPPRenderTarget &render_target, const vkb::core::HPPRenderPass &render_pass) override
	{
		return request_resource(device, recorder, state.framebuffers, render_target, render_pass);
	}
	vkb::core::HPPGraphicsPipeline &request_graphics_pipeline(vkb::rendering::HPPPipelineState &pipeline_state) override
	{
		return request_resource(device, recorder, state.graphics_pipelines, pipeline_cache, pipeline_state);
	}
	vkb::core::HPPPipelineLayout &request_pipeline_layout(const std::vector<vkb::core::HPPShaderModule *> &shader_modules) override
	{
		return request_resource(device, recorder, state.pipeline_layouts, shader_modules);
	}
	vkb::core::HPPRenderPass &request_render_pass(const std::vector<vkb::rendering::HPPAttachment> &attachments, const std::vector<vkb::common::HPPLoadStoreInfo> &load_store_infos, const std::vector<vkb::core::HPPSubpassInfo> &subpasses) override
	{
		return request_resource(device, recorder, state.render_passes, attachments, load_store_infos, subpasses);
	}
	vkb::core::HPPShaderModule &request_shader_module(vk::ShaderStageFlagBits stage, const vkb::ShaderSource &glsl_source, const vkb::ShaderVariant &shader_variant) override
	{
		std::string entry_point{"main"};
		return request_resource(device, recorder, state.shader_modules, stage, glsl_source, entry_point, shader_variant);
	}
	void clear_pipelines()
	{
		state.compute_pipelines.clear();
		state.graphics_pipelines.clear();
	}
	void set_pipeline_cache(vk::PipelineCache pipeline_cache)
	{
		this->pipeline_cache = pipeline_cache;
	}
	void warmup(std::vector<uint8_t> &&data)
	{
		recorder.set_data(std::move(data));
		replayer.play(*this, recorder);
	}
	std::vector<uint8_t> serialize() const
	{
		return recorder.get_data();
	}

  private:
	vk::PipelineCache pipeline_cache{VK_NULL_HANDLE};

	vkb::HPPResourceRecord recorder;

	vkb::HPPResourceReplay replayer;
};

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
		std::vector<uint8_t> data = device->get_handle().getPipelineCacheData(pipeline_cache);

		/* Write pipeline cache data to a file in binary format */
		vkb::fs::write_temp(data, "pipeline_cache.data");

		/* Destroy Vulkan pipeline cache */
		device->get_handle().destroyPipelineCache(pipeline_cache);
	}

	vkb::fs::write_temp(device->get_resource_cache<HPPPipelineCacheResourceCache>().serialize(), "cache.data");
}

bool HPPPipelineCache::prepare(const vkb::ApplicationOptions &options)
{
	if (!HPPVulkanSample::prepare(options))
	{
		return false;
	}

	device->override_resource_cache(std::make_unique<HPPPipelineCacheResourceCache>(*device));

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
	pipeline_cache = device->get_handle().createPipelineCache(pipeline_cache_create_info);

	HPPPipelineCacheResourceCache &resource_cache = device->get_resource_cache<HPPPipelineCacheResourceCache>();

	/* Use pipeline cache to store pipelines */
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

	/* Build all pipelines from a previous run */
	resource_cache.warmup(std::move(data_cache));

	stats->request_stats({vkb::StatIndex::frame_times});

	float dpi_factor = window->get_dpi_factor();

	button_size.x = button_size.x * dpi_factor;
	button_size.y = button_size.y * dpi_factor;

	gui = std::make_unique<vkb::HPPGui>(*this, *window, stats.get());

	load_scene("scenes/sponza/Sponza01.gltf");

	auto &camera_node = vkb::common::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = &camera_node.get_component<vkb::sg::Camera>();

	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_subpass =
	    std::make_unique<vkb::rendering::subpasses::HPPForwardSubpass>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, *camera);

	auto render_pipeline = vkb::rendering::HPPRenderPipeline();
	render_pipeline.add_subpass(std::move(scene_subpass));

	set_render_pipeline(std::move(render_pipeline));

	return true;
}

void HPPPipelineCache::draw_gui()
{
	gui->show_options_window(
	    /* body = */ [this]() {
		    if (ImGui::Checkbox("Pipeline cache", &enable_pipeline_cache))
		    {
			    device->get_resource_cache<HPPPipelineCacheResourceCache>().set_pipeline_cache(enable_pipeline_cache ? pipeline_cache : nullptr);
		    }

		    ImGui::SameLine();

		    if (ImGui::Button("Destroy Pipelines", button_size))
		    {
			    device->get_handle().waitIdle();
			    device->get_resource_cache<HPPPipelineCacheResourceCache>().clear_pipelines();
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

	HPPVulkanSample::update(delta_time);
}

std::unique_ptr<vkb::HPPVulkanSample> create_hpp_pipeline_cache()
{
	return std::make_unique<HPPPipelineCache>();
}
