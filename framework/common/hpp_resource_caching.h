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

#include "resource_caching.h"
#include <core/hpp_device.h>
#include <vulkan/vulkan_hash.hpp>

namespace std
{
template <typename Key, typename Value>
struct hash<std::map<Key, Value>>
{
	size_t operator()(std::map<Key, Value> const &bindings) const
	{
		size_t result = 0;
		vkb::hash_combine(result, bindings.size());
		for (auto const &binding : bindings)
		{
			vkb::hash_combine(result, binding.first);
			vkb::hash_combine(result, binding.second);
		}
		return result;
	}
};

template <typename T>
struct hash<std::vector<T>>
{
	size_t operator()(std::vector<T> const &values) const
	{
		size_t result = 0;
		vkb::hash_combine(result, values.size());
		for (auto const &value : values)
		{
			vkb::hash_combine(result, value);
		}
		return result;
	}
};

template <>
struct hash<vkb::common::HPPLoadStoreInfo>
{
	size_t operator()(vkb::common::HPPLoadStoreInfo const &lsi) const
	{
		size_t result = 0;
		vkb::hash_combine(result, lsi.load_op);
		vkb::hash_combine(result, lsi.store_op);
		return result;
	}
};

template <typename T>
struct hash<vkb::core::HPPVulkanResource<T>>
{
	size_t operator()(const vkb::core::HPPVulkanResource<T> &vulkan_resource) const
	{
		return std::hash<T>()(vulkan_resource.get_handle());
	}
};

template <>
struct hash<vkb::core::HPPDescriptorPool>
{
	size_t operator()(const vkb::core::HPPDescriptorPool &descriptor_pool) const
	{
		return std::hash<vkb::DescriptorPool>()(reinterpret_cast<vkb::DescriptorPool const &>(descriptor_pool));
	}
};

template <>
struct hash<vkb::core::HPPDescriptorSet>
{
	size_t operator()(vkb::core::HPPDescriptorSet &descriptor_set) const
	{
		size_t result = 0;
		vkb::hash_combine(result, descriptor_set.get_layout());
		// descriptor_pool ?
		vkb::hash_combine(result, descriptor_set.get_buffer_infos());
		vkb::hash_combine(result, descriptor_set.get_image_infos());
		vkb::hash_combine(result, descriptor_set.get_handle());
		// write_descriptor_sets ?

		return result;
	}
};

template <>
struct hash<vkb::core::HPPDescriptorSetLayout>
{
	size_t operator()(const vkb::core::HPPDescriptorSetLayout &descriptor_set_layout) const
	{
		return std::hash<vkb::DescriptorSetLayout>()(reinterpret_cast<vkb::DescriptorSetLayout const &>(descriptor_set_layout));
	}
};

template <>
struct hash<vkb::core::HPPImage>
{
	size_t operator()(const vkb::core::HPPImage &image) const
	{
		size_t result = 0;
		vkb::hash_combine(result, image.get_memory());
		vkb::hash_combine(result, image.get_type());
		vkb::hash_combine(result, image.get_extent());
		vkb::hash_combine(result, image.get_format());
		vkb::hash_combine(result, image.get_usage());
		vkb::hash_combine(result, image.get_sample_count());
		vkb::hash_combine(result, image.get_tiling());
		vkb::hash_combine(result, image.get_subresource());
		vkb::hash_combine(result, image.get_array_layer_count());
		return result;
	}
};

template <>
struct hash<vkb::core::HPPImageView>
{
	size_t operator()(const vkb::core::HPPImageView &image_view) const
	{
		size_t result = std::hash<vkb::core::HPPVulkanResource<vk::ImageView>>()(image_view);
		vkb::hash_combine(result, image_view.get_image());
		vkb::hash_combine(result, image_view.get_format());
		vkb::hash_combine(result, image_view.get_subresource_range());
		return result;
	}
};

template <>
struct hash<vkb::core::HPPRenderPass>
{
	size_t operator()(const vkb::core::HPPRenderPass &render_pass) const
	{
		return std::hash<vkb::RenderPass>()(reinterpret_cast<vkb::RenderPass const &>(render_pass));
	}
};

template <>
struct hash<vkb::core::HPPShaderModule>
{
	size_t operator()(const vkb::core::HPPShaderModule &shader_module) const
	{
		return std::hash<vkb::ShaderModule>()(reinterpret_cast<vkb::ShaderModule const &>(shader_module));
	}
};

template <>
struct hash<vkb::core::HPPShaderResource>
{
	size_t operator()(vkb::core::HPPShaderResource const &shader_resource) const
	{
		size_t result = 0;
		vkb::hash_combine(result, shader_resource.stages);
		vkb::hash_combine(result, shader_resource.type);
		vkb::hash_combine(result, shader_resource.mode);
		vkb::hash_combine(result, shader_resource.set);
		vkb::hash_combine(result, shader_resource.binding);
		vkb::hash_combine(result, shader_resource.location);
		vkb::hash_combine(result, shader_resource.input_attachment_index);
		vkb::hash_combine(result, shader_resource.vec_size);
		vkb::hash_combine(result, shader_resource.columns);
		vkb::hash_combine(result, shader_resource.array_size);
		vkb::hash_combine(result, shader_resource.offset);
		vkb::hash_combine(result, shader_resource.size);
		vkb::hash_combine(result, shader_resource.constant_id);
		vkb::hash_combine(result, shader_resource.qualifiers);
		vkb::hash_combine(result, shader_resource.name);
		return result;
	}
};

template <>
struct hash<vkb::core::HPPShaderSource>
{
	size_t operator()(const vkb::core::HPPShaderSource &shader_source) const
	{
		return std::hash<vkb::ShaderSource>()(reinterpret_cast<vkb::ShaderSource const &>(shader_source));
	}
};

template <>
struct hash<vkb::core::HPPShaderVariant>
{
	size_t operator()(const vkb::core::HPPShaderVariant &shader_variant) const
	{
		return std::hash<vkb::ShaderVariant>()(reinterpret_cast<vkb::ShaderVariant const &>(shader_variant));
	}
};

template <>
struct hash<vkb::core::HPPSubpassInfo>
{
	size_t operator()(vkb::core::HPPSubpassInfo const &subpass_info) const
	{
		size_t result = 0;
		vkb::hash_combine(result, subpass_info.input_attachments);
		vkb::hash_combine(result, subpass_info.output_attachments);
		vkb::hash_combine(result, subpass_info.color_resolve_attachments);
		vkb::hash_combine(result, subpass_info.disable_depth_stencil_attachment);
		vkb::hash_combine(result, subpass_info.depth_stencil_resolve_attachment);
		vkb::hash_combine(result, subpass_info.depth_stencil_resolve_mode);
		vkb::hash_combine(result, subpass_info.debug_name);
		return result;
	}
};

template <>
struct hash<vkb::rendering::HPPAttachment>
{
	size_t operator()(const vkb::rendering::HPPAttachment &attachment) const
	{
		size_t result = 0;
		vkb::hash_combine(result, attachment.format);
		vkb::hash_combine(result, attachment.samples);
		vkb::hash_combine(result, attachment.usage);
		vkb::hash_combine(result, attachment.initial_layout);
		return result;
	}
};

template <>
struct hash<vkb::rendering::HPPPipelineState>
{
	size_t operator()(const vkb::rendering::HPPPipelineState &pipeline_state) const
	{
		return std::hash<vkb::PipelineState>()(reinterpret_cast<vkb::PipelineState const &>(pipeline_state));
	}
};

template <>
struct hash<vkb::rendering::HPPRenderTarget>
{
	size_t operator()(const vkb::rendering::HPPRenderTarget &render_target) const
	{
		size_t result = 0;
		vkb::hash_combine(result, render_target.get_extent());
		for (auto const &view : render_target.get_views())
		{
			vkb::hash_combine(result, view);
		}
		for (auto const &attachment : render_target.get_attachments())
		{
			vkb::hash_combine(result, attachment);
		}
		for (auto const &input : render_target.get_input_attachments())
		{
			vkb::hash_combine(result, input);
		}
		for (auto const &output : render_target.get_output_attachments())
		{
			vkb::hash_combine(result, output);
		}
		return result;
	}
};

}        // namespace std

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
