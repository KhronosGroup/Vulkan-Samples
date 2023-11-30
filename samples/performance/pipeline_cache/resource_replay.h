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

#pragma once

#include "resource_record.h"

namespace vkb
{
class ResourceCache;

/**
 * @brief Reads Vulkan objects from a memory stream and creates them in the resource cache.
 */
class ResourceReplay
{
  public:
	ResourceReplay();

	void play(ResourceCache &resource_cache, ResourceRecord &recorder);

  protected:
	void create_shader_module(ResourceCache &resource_cache, std::istringstream &stream);

	void create_pipeline_layout(ResourceCache &resource_cache, std::istringstream &stream);

	void create_render_pass(ResourceCache &resource_cache, std::istringstream &stream);

	void create_graphics_pipeline(ResourceCache &resource_cache, std::istringstream &stream);

  private:
	using ResourceFunc = std::function<void(ResourceCache &, std::istringstream &)>;

	std::unordered_map<ResourceType, ResourceFunc> stream_resources;

	std::vector<ShaderModule *> shader_modules;

	std::vector<PipelineLayout *> pipeline_layouts;

	std::vector<const RenderPass *> render_passes;

	std::vector<const GraphicsPipeline *> graphics_pipelines;
};
}        // namespace vkb
