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
class HPPDevice;
class HPPPipelineLayout;
class HPPShaderModule;
}        // namespace core

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

	vkb::core::HPPPipelineLayout &request_pipeline_layout(const std::vector<vkb::core::HPPShaderModule *> &shader_modules)
	{
		return reinterpret_cast<vkb::core::HPPPipelineLayout &>(
		    vkb::ResourceCache::request_pipeline_layout(reinterpret_cast<std::vector<vkb::ShaderModule *> const &>(shader_modules)));
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
