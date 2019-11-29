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

#pragma once

#include <unordered_map>
#include <vector>

#include "common/helpers.h"
#include "core/descriptor_pool.h"
#include "core/descriptor_set.h"
#include "core/descriptor_set_layout.h"
#include "core/framebuffer.h"
#include "core/pipeline.h"
#include "resource_record.h"
#include "resource_replay.h"

namespace vkb
{
class Device;

namespace core
{
class ImageView;
}

/**
 * @brief Struct to hold the internal state of the Resource Cache
 *
 */
struct ResourceCacheState
{
	std::unordered_map<std::size_t, ShaderModule> shader_modules;

	std::unordered_map<std::size_t, PipelineLayout> pipeline_layouts;

	std::unordered_map<std::size_t, DescriptorSetLayout> descriptor_set_layouts;

	std::unordered_map<std::size_t, DescriptorPool> descriptor_pools;

	std::unordered_map<std::size_t, RenderPass> render_passes;

	std::unordered_map<std::size_t, GraphicsPipeline> graphics_pipelines;

	std::unordered_map<std::size_t, ComputePipeline> compute_pipelines;

	std::unordered_map<std::size_t, DescriptorSet> descriptor_sets;

	std::unordered_map<std::size_t, Framebuffer> framebuffers;
};

/**
 * @brief Cache all sorts of Vulkan objects specific to a Vulkan device.
 * Supports serialization and deserialization of cached resources.
 * There is only one cache for all these objects, with several unordered_map of hash indices
 * and objects. For every object requested, there is a templated version on request_resource.
 * Some objects may need building if they are not found in the cache.
 *
 * The resource cache is also linked with ResourceRecord and ResourceReplay. Replay can warm-up
 * the cache on app startup by creating all necessary objects.
 * The cache holds pointers to objects and has a mapping from such pointers to hashes.
 * It can only be destroyed in bulk, single elements cannot be removed.
 */
class ResourceCache
{
  public:
	ResourceCache(Device &device);

	ResourceCache(const ResourceCache &) = delete;

	ResourceCache(ResourceCache &&) = delete;

	ResourceCache &operator=(const ResourceCache &) = delete;

	ResourceCache &operator=(ResourceCache &&) = delete;

	void warmup(const std::vector<uint8_t> &data);

	std::vector<uint8_t> serialize();

	void set_pipeline_cache(VkPipelineCache pipeline_cache);

	ShaderModule &request_shader_module(VkShaderStageFlagBits stage, const ShaderSource &glsl_source, const ShaderVariant &shader_variant = {});

	PipelineLayout &request_pipeline_layout(const std::vector<ShaderModule *> &shader_modules);

	DescriptorSetLayout &request_descriptor_set_layout(const std::vector<ShaderResource> &set_resources);

	GraphicsPipeline &request_graphics_pipeline(PipelineState &pipeline_state);

	ComputePipeline &request_compute_pipeline(PipelineState &pipeline_state);

	DescriptorSet &request_descriptor_set(DescriptorSetLayout &                     descriptor_set_layout,
	                                      const BindingMap<VkDescriptorBufferInfo> &buffer_infos,
	                                      const BindingMap<VkDescriptorImageInfo> & image_infos);

	RenderPass &request_render_pass(const std::vector<Attachment> &   attachments,
	                                const std::vector<LoadStoreInfo> &load_store_infos,
	                                const std::vector<SubpassInfo> &  subpasses);

	Framebuffer &request_framebuffer(const RenderTarget &render_target,
	                                 const RenderPass &  render_pass);

	void clear_pipelines();

	/// @brief Update those descriptor sets referring to old views
	/// @param old_views Old image views referred by descriptor sets
	/// @param new_views New image views to be referred
	void update_descriptor_sets(const std::vector<core::ImageView> &old_views, const std::vector<core::ImageView> &new_views);

	void clear_framebuffers();

	void clear();

	const ResourceCacheState &get_internal_state() const;

  private:
	Device &device;

	ResourceRecord recorder;

	ResourceReplay replayer;

	VkPipelineCache pipeline_cache{VK_NULL_HANDLE};

	ResourceCacheState state;

	std::mutex descriptor_set_mutex;

	std::mutex pipeline_layout_mutex;

	std::mutex shader_module_mutex;

	std::mutex descriptor_set_layout_mutex;

	std::mutex graphics_pipeline_mutex;

	std::mutex render_pass_mutex;

	std::mutex compute_pipeline_mutex;

	std::mutex framebuffer_mutex;
};
}        // namespace vkb
