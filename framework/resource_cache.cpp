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

#include "resource_cache.h"

#include "common/resource_caching.h"
#include "core/device.h"

namespace vkb
{
namespace
{
template <class T, class... A>
T &request_resource(Device &device, ResourceRecord &recorder, std::mutex &resource_mutex, std::unordered_map<std::size_t, T> &resources, A &... args)
{
	std::lock_guard<std::mutex> guard(resource_mutex);

	auto &res = request_resource(device, &recorder, resources, args...);

	return res;
}
}        // namespace

ResourceCache::ResourceCache(Device &device) :
    device{device}
{
}

void ResourceCache::warmup(const std::vector<uint8_t> &data)
{
	recorder.set_data(data);

	replayer.play(*this, recorder);
}

std::vector<uint8_t> ResourceCache::serialize()
{
	return recorder.get_data();
}

void ResourceCache::set_pipeline_cache(VkPipelineCache new_pipeline_cache)
{
	pipeline_cache = new_pipeline_cache;
}

ShaderModule &ResourceCache::request_shader_module(VkShaderStageFlagBits stage, const ShaderSource &glsl_source, const ShaderVariant &shader_variant)
{
	std::string entry_point{"main"};
	return request_resource(device, recorder, shader_module_mutex, state.shader_modules, stage, glsl_source, entry_point, shader_variant);
}

PipelineLayout &ResourceCache::request_pipeline_layout(const std::vector<ShaderModule *> &shader_modules)
{
	return request_resource(device, recorder, pipeline_layout_mutex, state.pipeline_layouts, shader_modules);
}

DescriptorSetLayout &ResourceCache::request_descriptor_set_layout(const uint32_t                     set_index,
                                                                  const std::vector<ShaderModule *> &shader_modules,
                                                                  const std::vector<ShaderResource> &set_resources)
{
	return request_resource(device, recorder, descriptor_set_layout_mutex, state.descriptor_set_layouts, set_index, shader_modules, set_resources);
}

GraphicsPipeline &ResourceCache::request_graphics_pipeline(PipelineState &pipeline_state)
{
	return request_resource(device, recorder, graphics_pipeline_mutex, state.graphics_pipelines, pipeline_cache, pipeline_state);
}

ComputePipeline &ResourceCache::request_compute_pipeline(PipelineState &pipeline_state)
{
	return request_resource(device, recorder, compute_pipeline_mutex, state.compute_pipelines, pipeline_cache, pipeline_state);
}

DescriptorSet &ResourceCache::request_descriptor_set(DescriptorSetLayout &descriptor_set_layout, const BindingMap<VkDescriptorBufferInfo> &buffer_infos, const BindingMap<VkDescriptorImageInfo> &image_infos)
{
	auto &descriptor_pool = request_resource(device, recorder, descriptor_set_mutex, state.descriptor_pools, descriptor_set_layout);
	return request_resource(device, recorder, descriptor_set_mutex, state.descriptor_sets, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos);
}

RenderPass &ResourceCache::request_render_pass(const std::vector<Attachment> &attachments, const std::vector<LoadStoreInfo> &load_store_infos, const std::vector<SubpassInfo> &subpasses)
{
	return request_resource(device, recorder, render_pass_mutex, state.render_passes, attachments, load_store_infos, subpasses);
}

Framebuffer &ResourceCache::request_framebuffer(const RenderTarget &render_target, const RenderPass &render_pass)
{
	return request_resource(device, recorder, framebuffer_mutex, state.framebuffers, render_target, render_pass);
}

void ResourceCache::clear_pipelines()
{
	state.graphics_pipelines.clear();
	state.compute_pipelines.clear();
}

void ResourceCache::update_descriptor_sets(const std::vector<core::ImageView> &old_views, const std::vector<core::ImageView> &new_views)
{
	// Find descriptor sets referring to the old image view
	std::vector<VkWriteDescriptorSet> set_updates;
	std::set<size_t>                  matches;

	for (size_t i = 0; i < old_views.size(); ++i)
	{
		auto &old_view = old_views[i];
		auto &new_view = new_views[i];

		for (auto &kd_pair : state.descriptor_sets)
		{
			auto &key            = kd_pair.first;
			auto &descriptor_set = kd_pair.second;

			auto &image_infos = descriptor_set.get_image_infos();

			for (auto &ba_pair : image_infos)
			{
				auto &binding = ba_pair.first;
				auto &array   = ba_pair.second;

				for (auto &ai_pair : array)
				{
					auto &array_element = ai_pair.first;
					auto &image_info    = ai_pair.second;

					if (image_info.imageView == old_view.get_handle())
					{
						// Save key to remove old descriptor set
						matches.insert(key);

						// Update image info with new view
						image_info.imageView = new_view.get_handle();

						// Save struct for writing the update later
						{
							VkWriteDescriptorSet write_descriptor_set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

							if (auto binding_info = descriptor_set.get_layout().get_layout_binding(binding))
							{
								write_descriptor_set.dstBinding      = binding;
								write_descriptor_set.descriptorType  = binding_info->descriptorType;
								write_descriptor_set.pImageInfo      = &image_info;
								write_descriptor_set.dstSet          = descriptor_set.get_handle();
								write_descriptor_set.dstArrayElement = array_element;
								write_descriptor_set.descriptorCount = 1;

								set_updates.push_back(write_descriptor_set);
							}
							else
							{
								LOGE("Shader layout set does not use image binding at #{}", binding)
							}
						}
					}
				}
			}
		}
	}

	if (!set_updates.empty())
	{
		vkUpdateDescriptorSets(device.get_handle(), to_u32(set_updates.size()), set_updates.data(),
		                       0, nullptr);
	}

	// Delete old entries (moved out descriptor sets)
	for (auto &match : matches)
	{
		// Move out of the map
		auto it             = state.descriptor_sets.find(match);
		auto descriptor_set = std::move(it->second);
		state.descriptor_sets.erase(match);

		// Generate new key
		size_t new_key = 0U;
		hash_param(new_key, descriptor_set.get_layout(), descriptor_set.get_buffer_infos(), descriptor_set.get_image_infos());

		// Add (key, resource) to the cache
		state.descriptor_sets.emplace(new_key, std::move(descriptor_set));
	}
}

void ResourceCache::clear_framebuffers()
{
	state.framebuffers.clear();
}

void ResourceCache::clear()
{
	state.shader_modules.clear();
	state.pipeline_layouts.clear();
	state.descriptor_sets.clear();
	state.descriptor_set_layouts.clear();
	state.render_passes.clear();
	clear_pipelines();
	clear_framebuffers();
}

const ResourceCacheState &ResourceCache::get_internal_state() const
{
	return state;
}
}        // namespace vkb
