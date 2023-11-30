/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_resource_record.h"

namespace vkb
{
class HPPResourceCache;

namespace core
{
class HPPShaderModule;
class HPPPipelineLayout;
class HPPRenderPass;
class HPPGraphicsPipeline;
}        // namespace core

enum class HPPResourceType
{
	ShaderModule,
	PipelineLayout,
	RenderPass,
	GraphicsPipeline
};

class HPPResourceReplay
{
  public:
	HPPResourceReplay();
	void play(vkb::HPPResourceCache &resource_cache, vkb::HPPResourceRecord &recorder);

  protected:
	void create_shader_module(HPPResourceCache &resource_cache, std::istringstream &stream);
	void create_pipeline_layout(HPPResourceCache &resource_cache, std::istringstream &stream);
	void create_render_pass(HPPResourceCache &resource_cache, std::istringstream &stream);
	void create_graphics_pipeline(HPPResourceCache &resource_cache, std::istringstream &stream);

  private:
	using ResourceFunc = std::function<void(HPPResourceCache &, std::istringstream &)>;
	std::unordered_map<HPPResourceType, ResourceFunc> stream_resources;

	std::vector<core::HPPShaderModule *>           shader_modules;
	std::vector<core::HPPPipelineLayout *>         pipeline_layouts;
	std::vector<const core::HPPRenderPass *>       render_passes;
	std::vector<const core::HPPGraphicsPipeline *> graphics_pipelines;
};
}        // namespace vkb