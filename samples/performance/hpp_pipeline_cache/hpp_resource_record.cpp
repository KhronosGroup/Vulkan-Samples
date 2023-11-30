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

#include "hpp_resource_record.h"
#include "hpp_resource_replay.h"

#include "core/hpp_pipeline.h"
#include "core/hpp_pipeline_layout.h"
#include "core/hpp_render_pass.h"
#include "core/hpp_shader_module.h"
#include "hpp_resource_cache.h"
#include "rendering/hpp_pipeline_state.h"
#include "rendering/pipeline_state.h"

namespace vkb
{
namespace common
{
/**
 * @brief facade helper functions and structs around the functions and structs in common/resource_caching, providing a vulkan.hpp-based interface
 */

namespace
{
template <class T, class... A>
struct HPPRecordHelper
{
	size_t record(HPPResourceRecord & /*recorder*/, A &.../*args*/)
	{
		return 0;
	}

	void index(HPPResourceRecord & /*recorder*/, size_t /*index*/, T & /*resource*/)
	{}
};

template <class... A>
struct HPPRecordHelper<vkb::core::HPPShaderModule, A...>
{
	size_t record(HPPResourceRecord &recorder, A &...args)
	{
		return recorder.register_shader_module(args...);
	}

	void index(HPPResourceRecord &recorder, size_t index, vkb::core::HPPShaderModule &shader_module)
	{
		recorder.set_shader_module(index, shader_module);
	}
};

template <class... A>
struct HPPRecordHelper<vkb::core::HPPPipelineLayout, A...>
{
	size_t record(HPPResourceRecord &recorder, A &...args)
	{
		return recorder.register_pipeline_layout(args...);
	}

	void index(HPPResourceRecord &recorder, size_t index, vkb::core::HPPPipelineLayout &pipeline_layout)
	{
		recorder.set_pipeline_layout(index, pipeline_layout);
	}
};

template <class... A>
struct HPPRecordHelper<vkb::core::HPPRenderPass, A...>
{
	size_t record(HPPResourceRecord &recorder, A &...args)
	{
		return recorder.register_render_pass(args...);
	}

	void index(HPPResourceRecord &recorder, size_t index, vkb::core::HPPRenderPass &render_pass)
	{
		recorder.set_render_pass(index, render_pass);
	}
};

template <class... A>
struct HPPRecordHelper<vkb::core::HPPGraphicsPipeline, A...>
{
	size_t record(HPPResourceRecord &recorder, A &...args)
	{
		return recorder.register_graphics_pipeline(args...);
	}

	void index(HPPResourceRecord &recorder, size_t index, vkb::core::HPPGraphicsPipeline &graphics_pipeline)
	{
		recorder.set_graphics_pipeline(index, graphics_pipeline);
	}
};
}        // namespace
}        // namespace common
}        // namespace vkb

namespace vkb
{
namespace
{
inline void write_subpass_info(std::ostringstream &os, const std::vector<core::HPPSubpassInfo> &value)
{
	write(os, value.size());
	for (const core::HPPSubpassInfo &item : value)
	{
		write(os, item.input_attachments);
		write(os, item.output_attachments);
	}
}

inline void write_processes(std::ostringstream &os, const std::vector<std::string> &value)
{
	write(os, value.size());
	for (const std::string &item : value)
	{
		write(os, item);
	}
}
}        // namespace

void HPPResourceRecord::set_data(const std::vector<uint8_t> &data)
{
	stream.str(std::string{data.begin(), data.end()});
}

std::vector<uint8_t> HPPResourceRecord::get_data() const
{
	std::string str = stream.str();

	return std::vector<uint8_t>{str.begin(), str.end()};
}

const std::ostringstream &HPPResourceRecord::get_stream()
{
	return stream;
}

size_t HPPResourceRecord::register_shader_module(vk::ShaderStageFlagBits stage, const ShaderSource &glsl_source, const std::string &entry_point, const ShaderVariant &shader_variant)
{
	shader_module_indices.push_back(shader_module_indices.size());

	write(stream, ResourceType::ShaderModule, stage, glsl_source.get_source(), entry_point, shader_variant.get_preamble());

	write_processes(stream, shader_variant.get_processes());

	return shader_module_indices.back();
}

size_t HPPResourceRecord::register_pipeline_layout(const std::vector<core::HPPShaderModule *> &shader_modules)
{
	pipeline_layout_indices.push_back(pipeline_layout_indices.size());

	std::vector<size_t> shader_indices(shader_modules.size());
	std::transform(shader_modules.begin(), shader_modules.end(), shader_indices.begin(),
	               [this](core::HPPShaderModule *shader_module) { return shader_module_to_index.at(shader_module); });

	write(stream,
	      ResourceType::PipelineLayout,
	      shader_indices);

	return pipeline_layout_indices.back();
}

size_t HPPResourceRecord::register_render_pass(const std::vector<rendering::HPPAttachment> &attachments, const std::vector<common::HPPLoadStoreInfo> &load_store_infos, const std::vector<core::HPPSubpassInfo> &subpasses)
{
	render_pass_indices.push_back(render_pass_indices.size());

	write(stream,
	      ResourceType::RenderPass,
	      attachments,
	      load_store_infos);

	write_subpass_info(stream, subpasses);

	return render_pass_indices.back();
}

size_t HPPResourceRecord::register_graphics_pipeline(vk::PipelineCache /*pipeline_cache*/, rendering::HPPPipelineState &pipeline_state)
{
	graphics_pipeline_indices.push_back(graphics_pipeline_indices.size());

	auto &pipeline_layout = pipeline_state.get_pipeline_layout();
	auto  render_pass     = pipeline_state.get_render_pass();

	write(stream,
	      ResourceType::GraphicsPipeline,
	      pipeline_layout_to_index.at(&pipeline_layout),
	      render_pass_to_index.at(render_pass),
	      pipeline_state.get_subpass_index());

	auto &specialization_constant_state = pipeline_state.get_specialization_constant_state().get_specialization_constant_state();

	write(stream,
	      specialization_constant_state);

	auto &vertex_input_state = pipeline_state.get_vertex_input_state();

	write(stream,
	      vertex_input_state.attributes,
	      vertex_input_state.bindings);

	write(stream,
	      pipeline_state.get_input_assembly_state(),
	      pipeline_state.get_rasterization_state(),
	      pipeline_state.get_viewport_state(),
	      pipeline_state.get_multisample_state(),
	      pipeline_state.get_depth_stencil_state());

	auto &color_blend_state = pipeline_state.get_color_blend_state();

	write(stream,
	      color_blend_state.logic_op,
	      color_blend_state.logic_op_enable,
	      color_blend_state.attachments);

	return graphics_pipeline_indices.back();
}

void HPPResourceRecord::set_shader_module(size_t index, const core::HPPShaderModule &shader_module)
{
	shader_module_to_index[&shader_module] = index;
}

void HPPResourceRecord::set_pipeline_layout(size_t index, const core::HPPPipelineLayout &pipeline_layout)
{
	pipeline_layout_to_index[&pipeline_layout] = index;
}

void HPPResourceRecord::set_render_pass(size_t index, const core::HPPRenderPass &render_pass)
{
	render_pass_to_index[&render_pass] = index;
}

void HPPResourceRecord::set_graphics_pipeline(size_t index, const core::HPPGraphicsPipeline &graphics_pipeline)
{
	graphics_pipeline_to_index[&graphics_pipeline] = index;
}

}        // namespace vkb
