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

#pragma once

#include <core/hpp_descriptor_set.h>
#include <core/hpp_framebuffer.h>
#include <core/hpp_pipeline_layout.h>
#include <core/hpp_render_pass.h>
#include <hpp_resource_record.h>
#include <hpp_resource_replay.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
class HPPDescriptorPool;
class HPPDescriptorSetLayout;
class HPPImageView;
}        // namespace core

namespace rendering
{
class HPPRenderTarget;
}

/**
 * @brief Struct to hold the internal state of the Resource Cache
 *
 */
struct HPPResourceCacheState
{
	std::unordered_map<std::size_t, vkb::core::HPPShaderModule>        shader_modules;
	std::unordered_map<std::size_t, vkb::core::HPPPipelineLayout>      pipeline_layouts;
	std::unordered_map<std::size_t, vkb::core::HPPDescriptorSetLayout> descriptor_set_layouts;
	std::unordered_map<std::size_t, vkb::core::HPPDescriptorPool>      descriptor_pools;
	std::unordered_map<std::size_t, vkb::core::HPPRenderPass>          render_passes;
	std::unordered_map<std::size_t, vkb::core::HPPGraphicsPipeline>    graphics_pipelines;
	std::unordered_map<std::size_t, vkb::core::HPPComputePipeline>     compute_pipelines;
	std::unordered_map<std::size_t, vkb::core::HPPDescriptorSet>       descriptor_sets;
	std::unordered_map<std::size_t, vkb::core::HPPFramebuffer>         framebuffers;
};

/**
 * @brief vulkan.hpp version of the vkb::ResourceCache class
 *
 * See vkb::ResourceCache for documentation
 */
class HPPResourceCache
{
  public:
	HPPResourceCache(vkb::core::HPPDevice &device);

	HPPResourceCache(const HPPResourceCache &)            = delete;
	HPPResourceCache(HPPResourceCache &&)                 = delete;
	HPPResourceCache &operator=(const HPPResourceCache &) = delete;
	HPPResourceCache &operator=(HPPResourceCache &&)      = delete;

	void                               clear();
	void                               clear_framebuffers();
	void                               clear_pipelines();
	const HPPResourceCacheState       &get_internal_state() const;
	vkb::core::HPPComputePipeline     &request_compute_pipeline(vkb::rendering::HPPPipelineState &pipeline_state);
	vkb::core::HPPDescriptorSet       &request_descriptor_set(vkb::core::HPPDescriptorSetLayout          &descriptor_set_layout,
	                                                          const BindingMap<vk::DescriptorBufferInfo> &buffer_infos,
	                                                          const BindingMap<vk::DescriptorImageInfo>  &image_infos);
	vkb::core::HPPDescriptorSetLayout &request_descriptor_set_layout(const uint32_t                                   set_index,
	                                                                 const std::vector<vkb::core::HPPShaderModule *> &shader_modules,
	                                                                 const std::vector<vkb::core::HPPShaderResource> &set_resources);
	vkb::core::HPPFramebuffer         &request_framebuffer(const vkb::rendering::HPPRenderTarget &render_target, const vkb::core::HPPRenderPass &render_pass);
	vkb::core::HPPGraphicsPipeline    &request_graphics_pipeline(vkb::rendering::HPPPipelineState &pipeline_state);
	vkb::core::HPPPipelineLayout      &request_pipeline_layout(const std::vector<vkb::core::HPPShaderModule *> &shader_modules);
	vkb::core::HPPRenderPass          &request_render_pass(const std::vector<vkb::rendering::HPPAttachment> &attachments,
	                                                       const std::vector<vkb::common::HPPLoadStoreInfo> &load_store_infos,
	                                                       const std::vector<vkb::core::HPPSubpassInfo>     &subpasses);
	vkb::core::HPPShaderModule        &request_shader_module(
	           vk::ShaderStageFlagBits stage, const vkb::core::HPPShaderSource &glsl_source, const vkb::core::HPPShaderVariant &shader_variant = {});
	std::vector<uint8_t> serialize();
	void                 set_pipeline_cache(vk::PipelineCache pipeline_cache);

	/// @brief Update those descriptor sets referring to old views
	/// @param old_views Old image views referred by descriptor sets
	/// @param new_views New image views to be referred
	void update_descriptor_sets(const std::vector<vkb::core::HPPImageView> &old_views, const std::vector<vkb::core::HPPImageView> &new_views);

	void warmup(const std::vector<uint8_t> &data);

  private:
	vkb::core::HPPDevice  &device;
	vkb::HPPResourceRecord recorder                    = {};
	vkb::HPPResourceReplay replayer                    = {};
	vk::PipelineCache      pipeline_cache              = nullptr;
	HPPResourceCacheState  state                       = {};
	std::mutex             descriptor_set_mutex        = {};
	std::mutex             pipeline_layout_mutex       = {};
	std::mutex             shader_module_mutex         = {};
	std::mutex             descriptor_set_layout_mutex = {};
	std::mutex             graphics_pipeline_mutex     = {};
	std::mutex             render_pass_mutex           = {};
	std::mutex             compute_pipeline_mutex      = {};
	std::mutex             framebuffer_mutex           = {};
};
}        // namespace vkb
