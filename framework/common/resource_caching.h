/* Copyright (c) 2018-2020, Arm Limited and Contributors
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

#include "core/descriptor_pool.h"
#include "core/descriptor_set.h"
#include "core/descriptor_set_layout.h"
#include "core/framebuffer.h"
#include "core/pipeline.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_target.h"
#include "resource_record.h"

#include "common/helpers.h"

namespace std
{
template <>
struct hash<vkb::ShaderSource>
{
	std::size_t operator()(const vkb::ShaderSource &shader_source) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, shader_source.get_id());

		return result;
	}
};

template <>
struct hash<vkb::ShaderVariant>
{
	std::size_t operator()(const vkb::ShaderVariant &shader_variant) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, shader_variant.get_id());

		return result;
	}
};

template <>
struct hash<vkb::ShaderModule>
{
	std::size_t operator()(const vkb::ShaderModule &shader_module) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, shader_module.get_id());

		return result;
	}
};

template <>
struct hash<vkb::DescriptorSetLayout>
{
	std::size_t operator()(const vkb::DescriptorSetLayout &descriptor_set_layout) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, descriptor_set_layout.get_handle());

		return result;
	}
};

template <>
struct hash<vkb::DescriptorPool>
{
	std::size_t operator()(const vkb::DescriptorPool &descriptor_pool) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, descriptor_pool.get_descriptor_set_layout());

		return result;
	}
};

template <>
struct hash<vkb::PipelineLayout>
{
	std::size_t operator()(const vkb::PipelineLayout &pipeline_layout) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, pipeline_layout.get_handle());

		return result;
	}
};

template <>
struct hash<vkb::RenderPass>
{
	std::size_t operator()(const vkb::RenderPass &render_pass) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, render_pass.get_handle());

		return result;
	}
};

template <>
struct hash<vkb::Attachment>
{
	std::size_t operator()(const vkb::Attachment &attachment) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, static_cast<std::underlying_type<VkFormat>::type>(attachment.format));
		vkb::hash_combine(result, static_cast<std::underlying_type<VkSampleCountFlagBits>::type>(attachment.samples));

		return result;
	}
};

template <>
struct hash<vkb::LoadStoreInfo>
{
	std::size_t operator()(const vkb::LoadStoreInfo &load_store_info) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, static_cast<std::underlying_type<VkAttachmentLoadOp>::type>(load_store_info.load_op));
		vkb::hash_combine(result, static_cast<std::underlying_type<VkAttachmentStoreOp>::type>(load_store_info.store_op));

		return result;
	}
};

template <>
struct hash<vkb::SubpassInfo>
{
	std::size_t operator()(const vkb::SubpassInfo &subpass_info) const
	{
		std::size_t result = 0;

		for (uint32_t output_attachment : subpass_info.output_attachments)
		{
			vkb::hash_combine(result, output_attachment);
		}

		for (uint32_t input_attachment : subpass_info.input_attachments)
		{
			vkb::hash_combine(result, input_attachment);
		}

		return result;
	}
};

template <>
struct hash<vkb::SpecializationConstantState>
{
	std::size_t operator()(const vkb::SpecializationConstantState &specialization_constant_state) const
	{
		std::size_t result = 0;

		for (auto constants : specialization_constant_state.get_specialization_constant_state())
		{
			vkb::hash_combine(result, constants.first);
			for (const auto data : constants.second)
			{
				vkb::hash_combine(result, data);
			}
		}

		return result;
	}
};

template <>
struct hash<vkb::ShaderResource>
{
	std::size_t operator()(const vkb::ShaderResource &shader_resource) const
	{
		std::size_t result = 0;

		if (shader_resource.type == vkb::ShaderResourceType::Input ||
		    shader_resource.type == vkb::ShaderResourceType::Output ||
		    shader_resource.type == vkb::ShaderResourceType::PushConstant ||
		    shader_resource.type == vkb::ShaderResourceType::SpecializationConstant)
		{
			return result;
		}

		vkb::hash_combine(result, shader_resource.set);
		vkb::hash_combine(result, shader_resource.binding);
		vkb::hash_combine(result, static_cast<std::underlying_type<vkb::ShaderResourceType>::type>(shader_resource.type));

		return result;
	}
};

template <>
struct hash<VkDescriptorBufferInfo>
{
	std::size_t operator()(const VkDescriptorBufferInfo &descriptor_buffer_info) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, descriptor_buffer_info.buffer);
		vkb::hash_combine(result, descriptor_buffer_info.range);
		vkb::hash_combine(result, descriptor_buffer_info.offset);

		return result;
	}
};

template <>
struct hash<VkDescriptorImageInfo>
{
	std::size_t operator()(const VkDescriptorImageInfo &descriptor_image_info) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, descriptor_image_info.imageView);
		vkb::hash_combine(result, static_cast<std::underlying_type<VkImageLayout>::type>(descriptor_image_info.imageLayout));
		vkb::hash_combine(result, descriptor_image_info.sampler);

		return result;
	}
};

template <>
struct hash<VkVertexInputAttributeDescription>
{
	std::size_t operator()(const VkVertexInputAttributeDescription &vertex_attrib) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, vertex_attrib.binding);
		vkb::hash_combine(result, static_cast<std::underlying_type<VkFormat>::type>(vertex_attrib.format));
		vkb::hash_combine(result, vertex_attrib.location);
		vkb::hash_combine(result, vertex_attrib.offset);

		return result;
	}
};

template <>
struct hash<VkVertexInputBindingDescription>
{
	std::size_t operator()(const VkVertexInputBindingDescription &vertex_binding) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, vertex_binding.binding);
		vkb::hash_combine(result, static_cast<std::underlying_type<VkVertexInputRate>::type>(vertex_binding.inputRate));
		vkb::hash_combine(result, vertex_binding.stride);

		return result;
	}
};

template <>
struct hash<vkb::StencilOpState>
{
	std::size_t operator()(const vkb::StencilOpState &stencil) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, static_cast<std::underlying_type<VkCompareOp>::type>(stencil.compare_op));
		vkb::hash_combine(result, static_cast<std::underlying_type<VkStencilOp>::type>(stencil.depth_fail_op));
		vkb::hash_combine(result, static_cast<std::underlying_type<VkStencilOp>::type>(stencil.fail_op));
		vkb::hash_combine(result, static_cast<std::underlying_type<VkStencilOp>::type>(stencil.pass_op));

		return result;
	}
};

template <>
struct hash<VkExtent2D>
{
	size_t operator()(const VkExtent2D &extent) const
	{
		size_t result = 0;

		vkb::hash_combine(result, extent.width);
		vkb::hash_combine(result, extent.height);

		return result;
	}
};

template <>
struct hash<VkOffset2D>
{
	size_t operator()(const VkOffset2D &offset) const
	{
		size_t result = 0;

		vkb::hash_combine(result, offset.x);
		vkb::hash_combine(result, offset.y);

		return result;
	}
};

template <>
struct hash<VkRect2D>
{
	size_t operator()(const VkRect2D &rect) const
	{
		size_t result = 0;

		vkb::hash_combine(result, rect.extent);
		vkb::hash_combine(result, rect.offset);

		return result;
	}
};

template <>
struct hash<VkViewport>
{
	size_t operator()(const VkViewport &viewport) const
	{
		size_t result = 0;

		vkb::hash_combine(result, viewport.width);
		vkb::hash_combine(result, viewport.height);
		vkb::hash_combine(result, viewport.maxDepth);
		vkb::hash_combine(result, viewport.minDepth);
		vkb::hash_combine(result, viewport.x);
		vkb::hash_combine(result, viewport.y);

		return result;
	}
};

template <>
struct hash<vkb::ColorBlendAttachmentState>
{
	std::size_t operator()(const vkb::ColorBlendAttachmentState &color_blend_attachment) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, static_cast<std::underlying_type<VkBlendOp>::type>(color_blend_attachment.alpha_blend_op));
		vkb::hash_combine(result, color_blend_attachment.blend_enable);
		vkb::hash_combine(result, static_cast<std::underlying_type<VkBlendOp>::type>(color_blend_attachment.color_blend_op));
		vkb::hash_combine(result, color_blend_attachment.color_write_mask);
		vkb::hash_combine(result, static_cast<std::underlying_type<VkBlendFactor>::type>(color_blend_attachment.dst_alpha_blend_factor));
		vkb::hash_combine(result, static_cast<std::underlying_type<VkBlendFactor>::type>(color_blend_attachment.dst_color_blend_factor));
		vkb::hash_combine(result, static_cast<std::underlying_type<VkBlendFactor>::type>(color_blend_attachment.src_alpha_blend_factor));
		vkb::hash_combine(result, static_cast<std::underlying_type<VkBlendFactor>::type>(color_blend_attachment.src_color_blend_factor));

		return result;
	}
};

template <>
struct hash<vkb::RenderTarget>
{
	std::size_t operator()(const vkb::RenderTarget &render_target) const
	{
		std::size_t result = 0;

		for (auto &view : render_target.get_views())
		{
			vkb::hash_combine(result, view.get_handle());
		}

		return result;
	}
};

template <>
struct hash<vkb::PipelineState>
{
	std::size_t operator()(const vkb::PipelineState &pipeline_state) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, pipeline_state.get_pipeline_layout().get_handle());

		// For graphics only
		if (auto render_pass = pipeline_state.get_render_pass())
		{
			vkb::hash_combine(result, render_pass->get_handle());
		}

		vkb::hash_combine(result, pipeline_state.get_specialization_constant_state());

		vkb::hash_combine(result, pipeline_state.get_subpass_index());

		for (auto stage : pipeline_state.get_pipeline_layout().get_shader_modules())
		{
			vkb::hash_combine(result, stage->get_id());
		}

		// VkPipelineVertexInputStateCreateInfo
		for (auto &attribute : pipeline_state.get_vertex_input_state().attributes)
		{
			vkb::hash_combine(result, attribute);
		}

		for (auto &binding : pipeline_state.get_vertex_input_state().bindings)
		{
			vkb::hash_combine(result, binding);
		}

		// VkPipelineInputAssemblyStateCreateInfo
		vkb::hash_combine(result, pipeline_state.get_input_assembly_state().primitive_restart_enable);
		vkb::hash_combine(result, static_cast<std::underlying_type<VkPrimitiveTopology>::type>(pipeline_state.get_input_assembly_state().topology));

		//VkPipelineViewportStateCreateInfo
		vkb::hash_combine(result, pipeline_state.get_viewport_state().viewport_count);
		vkb::hash_combine(result, pipeline_state.get_viewport_state().scissor_count);

		// VkPipelineRasterizationStateCreateInfo
		vkb::hash_combine(result, pipeline_state.get_rasterization_state().cull_mode);
		vkb::hash_combine(result, pipeline_state.get_rasterization_state().depth_bias_enable);
		vkb::hash_combine(result, pipeline_state.get_rasterization_state().depth_clamp_enable);
		vkb::hash_combine(result, static_cast<std::underlying_type<VkFrontFace>::type>(pipeline_state.get_rasterization_state().front_face));
		vkb::hash_combine(result, static_cast<std::underlying_type<VkPolygonMode>::type>(pipeline_state.get_rasterization_state().polygon_mode));
		vkb::hash_combine(result, pipeline_state.get_rasterization_state().rasterizer_discard_enable);

		// VkPipelineMultisampleStateCreateInfo
		vkb::hash_combine(result, pipeline_state.get_multisample_state().alpha_to_coverage_enable);
		vkb::hash_combine(result, pipeline_state.get_multisample_state().alpha_to_one_enable);
		vkb::hash_combine(result, pipeline_state.get_multisample_state().min_sample_shading);
		vkb::hash_combine(result, static_cast<std::underlying_type<VkSampleCountFlagBits>::type>(pipeline_state.get_multisample_state().rasterization_samples));
		vkb::hash_combine(result, pipeline_state.get_multisample_state().sample_shading_enable);
		vkb::hash_combine(result, pipeline_state.get_multisample_state().sample_mask);

		// VkPipelineDepthStencilStateCreateInfo
		vkb::hash_combine(result, pipeline_state.get_depth_stencil_state().back);
		vkb::hash_combine(result, pipeline_state.get_depth_stencil_state().depth_bounds_test_enable);
		vkb::hash_combine(result, static_cast<std::underlying_type<VkCompareOp>::type>(pipeline_state.get_depth_stencil_state().depth_compare_op));
		vkb::hash_combine(result, pipeline_state.get_depth_stencil_state().depth_test_enable);
		vkb::hash_combine(result, pipeline_state.get_depth_stencil_state().depth_write_enable);
		vkb::hash_combine(result, pipeline_state.get_depth_stencil_state().front);
		vkb::hash_combine(result, pipeline_state.get_depth_stencil_state().stencil_test_enable);

		// VkPipelineColorBlendStateCreateInfo
		vkb::hash_combine(result, static_cast<std::underlying_type<VkLogicOp>::type>(pipeline_state.get_color_blend_state().logic_op));
		vkb::hash_combine(result, pipeline_state.get_color_blend_state().logic_op_enable);

		for (auto &attachment : pipeline_state.get_color_blend_state().attachments)
		{
			vkb::hash_combine(result, attachment);
		}

		return result;
	}
};
}        // namespace std

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
}        // namespace

template <class T, class... A>
T &request_resource(Device &device, ResourceRecord *recorder, std::unordered_map<std::size_t, T> &resources, A &... args)
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

	LOGD("Building #{} cache object ({})", res_id, res_type);

// Only error handle in release
#ifndef DEBUG
	try
	{
#endif
		T resource(device, args...);

		auto res_ins_it = resources.emplace(hash, std::move(resource));

		if (!res_ins_it.second)
		{
			throw std::runtime_error{std::string{"Insertion error for #"} + std::to_string(res_id) + "cache object (" + res_type + ")"};
		}

		res_it = res_ins_it.first;

		if (recorder)
		{
			size_t index = record_helper.record(*recorder, args...);
			record_helper.index(*recorder, index, res_it->second);
		}
#ifndef DEBUG
	}
	catch (const std::exception &e)
	{
		LOGE("Creation error for #{} cache object ({})", res_id, res_type);
		throw e;
	}
#endif

	return res_it->second;
}
}        // namespace vkb
