/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "core/device.h"
#include "rendering/pipeline_state.h"

namespace vkb
{
namespace
{
template <typename T>
inline void hash_param(size_t &seed, const T &value)
{
	hash_combine(seed, value);
}

template <>
inline void hash_param(size_t & /*seed*/, const VkPipelineCache & /*value*/)
{
}

template <>
inline void hash_param<std::vector<uint8_t>>(
    size_t &                    seed,
    const std::vector<uint8_t> &value)
{
	hash_combine(seed, std::string{value.begin(), value.end()});
}

template <>
inline void hash_param<std::vector<Attachment>>(
    size_t &                       seed,
    const std::vector<Attachment> &value)
{
	for (auto &attachment : value)
	{
		hash_combine(seed, attachment);
	}
}

template <>
inline void hash_param<std::vector<LoadStoreInfo>>(
    size_t &                          seed,
    const std::vector<LoadStoreInfo> &value)
{
	for (auto &load_store_info : value)
	{
		hash_combine(seed, load_store_info);
	}
}

template <>
inline void hash_param<std::vector<SubpassInfo>>(
    size_t &                        seed,
    const std::vector<SubpassInfo> &value)
{
	for (auto &subpass_info : value)
	{
		hash_combine(seed, subpass_info);
	}
}

template <>
inline void hash_param<std::vector<ShaderModule *>>(
    size_t &                           seed,
    const std::vector<ShaderModule *> &value)
{
	for (auto &shader_module : value)
	{
		hash_combine(seed, shader_module->get_id());
	}
}

template <>
inline void hash_param<std::vector<ShaderResource>>(
    size_t &                           seed,
    const std::vector<ShaderResource> &value)
{
	for (auto &resource : value)
	{
		hash_combine(seed, resource);
	}
}

template <>
inline void hash_param<std::map<uint32_t, std::map<uint32_t, VkDescriptorBufferInfo>>>(
    size_t &                                                              seed,
    const std::map<uint32_t, std::map<uint32_t, VkDescriptorBufferInfo>> &value)
{
	for (auto &binding_set : value)
	{
		hash_combine(seed, binding_set.first);

		for (auto &binding_element : binding_set.second)
		{
			hash_combine(seed, binding_element.first);
			hash_combine(seed, binding_element.second);
		}
	}
}

template <>
inline void hash_param<std::map<uint32_t, std::map<uint32_t, VkDescriptorImageInfo>>>(
    size_t &                                                             seed,
    const std::map<uint32_t, std::map<uint32_t, VkDescriptorImageInfo>> &value)
{
	for (auto &binding_set : value)
	{
		hash_combine(seed, binding_set.first);

		for (auto &binding_element : binding_set.second)
		{
			hash_combine(seed, binding_element.first);
			hash_combine(seed, binding_element.second);
		}
	}
}

template <typename T, typename... Args>
inline void hash_param(size_t &seed, const T &first_arg, const Args &... args)
{
	hash_param(seed, first_arg);

	hash_param(seed, args...);
}

template <class T, class... A>
struct RecordHelper
{
	size_t record(ResourceRecord & /*recorder*/, A &... /*args*/)
	{
		return 0;
	}

	void index(ResourceRecord & /*recorder*/, size_t /*index*/, T & /*resource*/)
	{
	}
};

template <class... A>
struct RecordHelper<ShaderModule, A...>
{
	size_t record(ResourceRecord &recorder, A &... args)
	{
		return recorder.register_shader_module(args...);
	}

	void index(ResourceRecord &recorder, size_t index, ShaderModule &shader_module)
	{
		recorder.set_shader_module(index, shader_module);
	}
};

template <class... A>
struct RecordHelper<PipelineLayout, A...>
{
	size_t record(ResourceRecord &recorder, A &... args)
	{
		return recorder.register_pipeline_layout(args...);
	}

	void index(ResourceRecord &recorder, size_t index, PipelineLayout &pipeline_layout)
	{
		recorder.set_pipeline_layout(index, pipeline_layout);
	}
};

template <class... A>
struct RecordHelper<RenderPass, A...>
{
	size_t record(ResourceRecord &recorder, A &... args)
	{
		return recorder.register_render_pass(args...);
	}

	void index(ResourceRecord &recorder, size_t index, RenderPass &render_pass)
	{
		recorder.set_render_pass(index, render_pass);
	}
};

template <class... A>
struct RecordHelper<GraphicsPipeline, A...>
{
	size_t record(ResourceRecord &recorder, A &... args)
	{
		return recorder.register_graphics_pipeline(args...);
	}

	void index(ResourceRecord &recorder, size_t index, GraphicsPipeline &graphics_pipeline)
	{
		recorder.set_graphics_pipeline(index, graphics_pipeline);
	}
};

template <class T, class... A>
T &request_resource(Device &device, ResourceRecord &recorder, std::unordered_map<std::size_t, T> &resources, A &... args)
{
	RecordHelper<T, A...> record_helper;

	std::size_t hash{0U};
	hash_param(hash, args...);

	auto res_it = resources.find(hash);

	if (res_it != resources.end())
	{
		return res_it->second;
	}

	// If we do not have it already, create and cache it
	const char *res_type = typeid(T).name();
	size_t      res_id   = resources.size();

	LOGI("Building #{} cache object ({})", res_id, res_type);

	size_t index = record_helper.record(recorder, args...);

	try
	{
		T resource(device, args...);

		auto res_ins_it = resources.emplace(hash, std::move(resource));

		if (!res_ins_it.second)
		{
			throw std::runtime_error{std::string{"Insertion error for #"} + std::to_string(res_id) + "cache object (" + res_type + ")"};
		}

		res_it = res_ins_it.first;

		record_helper.index(recorder, index, res_it->second);
	}
	catch (const std::exception &e)
	{
		LOGE("Creation error for #{} cache object ({})", res_id, res_type);
		throw e;
	}

	return res_it->second;
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
	return request_resource(device, recorder, shader_modules, stage, glsl_source, entry_point, shader_variant);
}

PipelineLayout &ResourceCache::request_pipeline_layout(const std::vector<ShaderModule *> &requested_shader_modules)
{
	return request_resource(device, recorder, pipeline_layouts, requested_shader_modules);
}

DescriptorSetLayout &ResourceCache::request_descriptor_set_layout(const std::vector<ShaderResource> &set_resources)
{
	return request_resource(device, recorder, descriptor_set_layouts, set_resources);
}

GraphicsPipeline &ResourceCache::request_graphics_pipeline(PipelineState &pipeline_state)
{
	return request_resource(device, recorder, graphics_pipelines, pipeline_cache, pipeline_state);
}

ComputePipeline &ResourceCache::request_compute_pipeline(PipelineState &pipeline_state)
{
	return request_resource(device, recorder, compute_pipelines, pipeline_cache, pipeline_state);
}

DescriptorSet &ResourceCache::request_descriptor_set(DescriptorSetLayout &descriptor_set_layout, const BindingMap<VkDescriptorBufferInfo> &buffer_infos, const BindingMap<VkDescriptorImageInfo> &image_infos)
{
	return request_resource(device, recorder, descriptor_sets, descriptor_set_layout, buffer_infos, image_infos);
}

RenderPass &ResourceCache::request_render_pass(const std::vector<Attachment> &attachments, const std::vector<LoadStoreInfo> &load_store_infos, const std::vector<SubpassInfo> &subpasses)
{
	return request_resource(device, recorder, render_passes, attachments, load_store_infos, subpasses);
}

Framebuffer &ResourceCache::request_framebuffer(const RenderTarget &render_target, const RenderPass &render_pass)
{
	return request_resource(device, recorder, framebuffers, render_target, render_pass);
}

void ResourceCache::clear_pipelines()
{
	graphics_pipelines.clear();

	compute_pipelines.clear();
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

		for (auto &kd_pair : descriptor_sets)
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

							VkDescriptorSetLayoutBinding binding_info;
							if (!descriptor_set.get_layout().get_layout_binding(binding, binding_info))
							{
								LOGE("Shader layout set does not use image binding at #{}", binding);
								continue;
							}

							write_descriptor_set.dstBinding      = binding;
							write_descriptor_set.descriptorType  = binding_info.descriptorType;
							write_descriptor_set.pImageInfo      = &image_info;
							write_descriptor_set.dstSet          = descriptor_set.get_handle();
							write_descriptor_set.dstArrayElement = array_element;
							write_descriptor_set.descriptorCount = 1;

							set_updates.push_back(write_descriptor_set);
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
		auto it             = descriptor_sets.find(match);
		auto descriptor_set = std::move(it->second);
		descriptor_sets.erase(match);

		// Generate new key
		size_t new_key = 0U;
		hash_param(new_key, descriptor_set.get_layout(), descriptor_set.get_buffer_infos(), descriptor_set.get_image_infos());

		// Add (key, resource) to the cache
		descriptor_sets.emplace(new_key, std::move(descriptor_set));
	}
}

void ResourceCache::clear_framebuffers()
{
	framebuffers.clear();
}

void ResourceCache::clear()
{
	shader_modules.clear();
	pipeline_layouts.clear();
	descriptor_sets.clear();
	descriptor_set_layouts.clear();
	render_passes.clear();
	clear_pipelines();
	clear_framebuffers();
}
}        // namespace vkb
