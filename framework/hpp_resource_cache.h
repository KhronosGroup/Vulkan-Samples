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

#include <resource_cache.h>

namespace vkb
{
namespace core
{
class HPPComputePipeline;
class HPPDevice;
class HPPGraphicsPipeline;
class HPPPipelineLayout;
class HPPShaderModule;
}        // namespace core

namespace rendering
{
struct HPPAttachment;
}

/**
 * @brief facade class around vkb::ResourceCache, providing a vulkan.hpp-based interface
 *
 * See vkb::ResourceCache for documentation
 */
class HPPResourceCache : private vkb::ResourceCache
{
  public:
	using vkb::ResourceCache::clear;
	using vkb::ResourceCache::clear_framebuffers;
	using vkb::ResourceCache::clear_pipelines;
	using vkb::ResourceCache::serialize;
	using vkb::ResourceCache::warmup;

	HPPResourceCache(vkb::core::HPPDevice &device) :
	    vkb::ResourceCache(reinterpret_cast<vkb::Device &>(device))
	{}

	vkb::core::HPPComputePipeline &request_compute_pipeline(vkb::rendering::HPPPipelineState &pipeline_state)
	{
		return reinterpret_cast<vkb::core::HPPComputePipeline &>(
		    vkb::ResourceCache::request_compute_pipeline(reinterpret_cast<vkb::PipelineState &>(pipeline_state)));
	}

	vkb::core::HPPFramebuffer &request_framebuffer(const vkb::rendering::HPPRenderTarget &render_target, const vkb::core::HPPRenderPass &render_pass)
	{
		return reinterpret_cast<vkb::core::HPPFramebuffer &>(vkb::ResourceCache::request_framebuffer(reinterpret_cast<vkb::RenderTarget const &>(render_target),
		                                                                                             reinterpret_cast<vkb::RenderPass const &>(render_pass)));
	}

	vkb::core::HPPGraphicsPipeline &request_graphics_pipeline(vkb::rendering::HPPPipelineState &pipeline_state)
	{
		return reinterpret_cast<vkb::core::HPPGraphicsPipeline &>(
		    vkb::ResourceCache::request_graphics_pipeline(reinterpret_cast<vkb::PipelineState &>(pipeline_state)));
	}

	vkb::core::HPPPipelineLayout &request_pipeline_layout(const std::vector<vkb::core::HPPShaderModule *> &shader_modules)
	{
		return reinterpret_cast<vkb::core::HPPPipelineLayout &>(
		    vkb::ResourceCache::request_pipeline_layout(reinterpret_cast<std::vector<vkb::ShaderModule *> const &>(shader_modules)));
	}

	vkb::core::HPPRenderPass &request_render_pass(const std::vector<vkb::rendering::HPPAttachment> &attachments,
	                                              const std::vector<vkb::common::HPPLoadStoreInfo> &load_store_infos,
	                                              const std::vector<vkb::core::HPPSubpassInfo>     &subpasses)
	{
		return reinterpret_cast<vkb::core::HPPRenderPass &>(
		    vkb::ResourceCache::request_render_pass(reinterpret_cast<std::vector<vkb::Attachment> const &>(attachments),
		                                            reinterpret_cast<std::vector<vkb::LoadStoreInfo> const &>(load_store_infos),
		                                            reinterpret_cast<std::vector<vkb::SubpassInfo> const &>(subpasses)));
	}

	vkb::core::HPPShaderModule &request_shader_module(vk::ShaderStageFlagBits stage, const ShaderSource &glsl_source, const ShaderVariant &shader_variant = {})
	{
		return reinterpret_cast<vkb::core::HPPShaderModule &>(
		    vkb::ResourceCache::request_shader_module(static_cast<VkShaderStageFlagBits>(stage), glsl_source, shader_variant));
	}

	void set_pipeline_cache(vk::PipelineCache pipeline_cache)
	{
		vkb::ResourceCache::set_pipeline_cache(static_cast<VkPipelineCache>(pipeline_cache));
	}
};

}        // namespace vkb
