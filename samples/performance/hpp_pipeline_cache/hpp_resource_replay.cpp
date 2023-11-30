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

#include "hpp_resource_replay.h"
#include "hpp_resource_record.h"

#include "common/hpp_vk_common.h"
#include "common/logging.h"
#include "hpp_resource_cache.h"
#include "rendering/hpp_pipeline_state.h"
#include <core/hpp_pipeline.h>

namespace vkb
{
namespace
{
inline void read_subpass_info(std::istringstream &is, std::vector<core::HPPSubpassInfo> &value)
{
	std::size_t size;
	read(is, size);
	value.resize(size);
	for (core::HPPSubpassInfo &subpass : value)
	{
		read(is, subpass.input_attachments);
		read(is, subpass.output_attachments);
	}
}

inline void read_processes(std::istringstream &is, std::vector<std::string> &value)
{
	std::size_t size;
	read(is, size);
	value.resize(size);
	for (std::string &item : value)
	{
		read(is, item);
	}
}
}        // namespace

HPPResourceReplay::HPPResourceReplay()
{
	stream_resources[HPPResourceType::ShaderModule]     = std::bind(&HPPResourceReplay::create_shader_module, this, std::placeholders::_1, std::placeholders::_2);
	stream_resources[HPPResourceType::PipelineLayout]   = std::bind(&HPPResourceReplay::create_pipeline_layout, this, std::placeholders::_1, std::placeholders::_2);
	stream_resources[HPPResourceType::RenderPass]       = std::bind(&HPPResourceReplay::create_render_pass, this, std::placeholders::_1, std::placeholders::_2);
	stream_resources[HPPResourceType::GraphicsPipeline] = std::bind(&HPPResourceReplay::create_graphics_pipeline, this, std::placeholders::_1, std::placeholders::_2);
}

void HPPResourceReplay::play(HPPResourceCache &resource_cache, HPPResourceRecord &recorder)
{
	std::istringstream stream{recorder.get_stream().str()};

	while (true)
	{
		// Read command id
		HPPResourceType resource_type;
		read(stream, resource_type);

		if (stream.eof())
		{
			break;
		}

		// Find command function for the given command id
		auto cmd_it = stream_resources.find(resource_type);

		// Check if command replayer supports the given command
		if (cmd_it != stream_resources.end())
		{
			// Run command function
			cmd_it->second(resource_cache, stream);
		}
		else
		{
			LOGE("Replay command not supported.");
		}
	}
}

void HPPResourceReplay::create_shader_module(HPPResourceCache &resource_cache, std::istringstream &stream)
{
	vk::ShaderStageFlagBits  stage{};
	std::string              glsl_source;
	std::string              entry_point;
	std::string              preamble;
	std::vector<std::string> processes;

	read(stream,
	     stage,
	     glsl_source,
	     entry_point,
	     preamble);

	read_processes(stream, processes);

	ShaderSource shader_source{};
	shader_source.set_source(std::move(glsl_source));
	ShaderVariant shader_variant(std::move(preamble), std::move(processes));

	auto &shader_module = resource_cache.request_shader_module(stage, shader_source, shader_variant);

	shader_modules.push_back(&shader_module);
}

void HPPResourceReplay::create_pipeline_layout(HPPResourceCache &resource_cache, std::istringstream &stream)
{
	std::vector<size_t> shader_indices;

	read(stream,
	     shader_indices);

	std::vector<core::HPPShaderModule *> shader_stages(shader_indices.size());
	std::transform(shader_indices.begin(),
	               shader_indices.end(),
	               shader_stages.begin(),
	               [&](size_t shader_index) {
		               assert(shader_index < shader_modules.size());
		               return shader_modules[shader_index];
	               });

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_stages);

	pipeline_layouts.push_back(&pipeline_layout);
}

void HPPResourceReplay::create_render_pass(HPPResourceCache &resource_cache, std::istringstream &stream)
{
	std::vector<rendering::HPPAttachment> attachments;
	std::vector<common::HPPLoadStoreInfo> load_store_infos;
	std::vector<core::HPPSubpassInfo>     subpasses;

	read(stream,
	     attachments,
	     load_store_infos);

	read_subpass_info(stream, subpasses);

	auto &render_pass = resource_cache.request_render_pass(attachments, load_store_infos, subpasses);

	render_passes.push_back(&render_pass);
}

void HPPResourceReplay::create_graphics_pipeline(HPPResourceCache &resource_cache, std::istringstream &stream)
{
	size_t   pipeline_layout_index{};
	size_t   render_pass_index{};
	uint32_t subpass_index{};

	read(stream,
	     pipeline_layout_index,
	     render_pass_index,
	     subpass_index);

	std::map<uint32_t, std::vector<uint8_t>> specialization_constant_state{};
	read(stream,
	     specialization_constant_state);

	rendering::HPPVertexInputState vertex_input_state{};

	read(stream,
	     vertex_input_state.attributes,
	     vertex_input_state.bindings);

	rendering::HPPInputAssemblyState input_assembly_state{};
	rendering::HPPRasterizationState rasterization_state{};
	rendering::HPPViewportState      viewport_state{};
	rendering::HPPMultisampleState   multisample_state{};
	rendering::HPPDepthStencilState  depth_stencil_state{};

	read(stream,
	     input_assembly_state,
	     rasterization_state,
	     viewport_state,
	     multisample_state,
	     depth_stencil_state);

	rendering::HPPColorBlendState color_blend_state{};

	read(stream,
	     color_blend_state.logic_op,
	     color_blend_state.logic_op_enable,
	     color_blend_state.attachments);

	rendering::HPPPipelineState pipeline_state{};
	assert(pipeline_layout_index < pipeline_layouts.size());
	pipeline_state.set_pipeline_layout(*pipeline_layouts[pipeline_layout_index]);
	assert(render_pass_index < render_passes.size());
	pipeline_state.set_render_pass(*render_passes[render_pass_index]);

	for (auto &item : specialization_constant_state)
	{
		pipeline_state.set_specialization_constant(item.first, item.second);
	}

	pipeline_state.set_subpass_index(subpass_index);
	pipeline_state.set_vertex_input_state(vertex_input_state);
	pipeline_state.set_input_assembly_state(input_assembly_state);
	pipeline_state.set_rasterization_state(rasterization_state);
	pipeline_state.set_viewport_state(viewport_state);
	pipeline_state.set_multisample_state(multisample_state);
	pipeline_state.set_depth_stencil_state(depth_stencil_state);
	pipeline_state.set_color_blend_state(color_blend_state);

	auto &graphics_pipeline = resource_cache.request_graphics_pipeline(pipeline_state);

	graphics_pipelines.push_back(&graphics_pipeline);
}
}        // namespace vkb
