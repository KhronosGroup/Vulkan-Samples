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

#include "pipeline_layout.h"

#include "descriptor_set_layout.h"
#include "device.h"
#include "pipeline.h"
#include "shader_module.h"

namespace vkb
{
PipelineLayout::PipelineLayout(Device &device, const std::vector<ShaderModule *> &shader_modules) :
    device{device},
    shader_modules{shader_modules}
{
	// Merge shader stages resources
	for (ShaderModule *stage : shader_modules)
	{
		// Iterate over all of the shader resources
		for (const ShaderResource &resource : stage->get_resources())
		{
			std::string key = resource.name;

			// Update name as input and output resources can have the same name
			if (resource.type == ShaderResourceType::Output || resource.type == ShaderResourceType::Input)
			{
				key = std::to_string(resource.stages) + "_" + key;
			}

			// Find resource by name in the map
			auto it = resources.find(key);

			if (it != resources.end())
			{
				// Append stage flags if resource already exists
				it->second.stages |= resource.stages;
			}
			else
			{
				// Create a new entry in the map
				resources.emplace(key, resource);
			}
		}
	}

	// Separate all resources by set index
	for (auto &it : resources)
	{
		ShaderResource &resource = it.second;

		// Find binding by set index in the map.
		auto it2 = set_bindings.find(resource.set);

		if (it2 != set_bindings.end())
		{
			// Add resource to the found set index
			it2->second.push_back(resource);
		}
		else
		{
			// Create a new set index and with the first resource
			set_bindings.emplace(resource.set, std::vector<ShaderResource>{resource});
		}
	}

	// Create a descriptor set layout for each set index
	for (auto it : set_bindings)
	{
		set_layouts.emplace(it.first, &device.get_resource_cache().request_descriptor_set_layout(it.second));
	}

	std::vector<VkDescriptorSetLayout> descriptor_set_layouts;

	for (auto &it : set_layouts)
	{
		descriptor_set_layouts.push_back(it.second->get_handle());
	}

	std::vector<VkPushConstantRange> push_constant_ranges;

	for (auto it : resources)
	{
		ShaderResource &resource = it.second;

		if (resource.type != ShaderResourceType::PushConstant)
		{
			continue;
		}

		VkPushConstantRange range{};

		range.stageFlags = resource.stages;
		range.offset     = resource.offset;
		range.size       = resource.size;

		push_constant_ranges.push_back(range);
	}

	VkPipelineLayoutCreateInfo create_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

	create_info.setLayoutCount         = to_u32(descriptor_set_layouts.size());
	create_info.pSetLayouts            = descriptor_set_layouts.data();
	create_info.pushConstantRangeCount = to_u32(push_constant_ranges.size());
	create_info.pPushConstantRanges    = push_constant_ranges.data();

	// Create the Vulkan pipeline layout handle
	auto result = vkCreatePipelineLayout(device.get_handle(), &create_info, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create PipelineLayout"};
	}
}

PipelineLayout::PipelineLayout(PipelineLayout &&other) :
    device{other.device},
    handle{other.handle},
    shader_modules{std::move(other.shader_modules)},
    resources{std::move(other.resources)},
    set_bindings{std::move(other.set_bindings)},
    set_layouts{std::move(other.set_layouts)}
{
	other.handle = VK_NULL_HANDLE;
}

PipelineLayout::~PipelineLayout()
{
	// Destroy pipeline layout
	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(device.get_handle(), handle, nullptr);
	}
}

VkPipelineLayout PipelineLayout::get_handle() const
{
	return handle;
}

const std::vector<ShaderModule *> &PipelineLayout::get_stages() const
{
	return shader_modules;
}

const std::unordered_map<uint32_t, std::vector<ShaderResource>> &PipelineLayout::get_bindings() const
{
	return set_bindings;
}

const std::vector<ShaderResource> &PipelineLayout::get_set_bindings(uint32_t set_index) const
{
	return set_bindings.at(set_index);
}

bool PipelineLayout::has_set_layout(uint32_t set_index) const
{
	return set_index < set_layouts.size();
}

DescriptorSetLayout &PipelineLayout::get_set_layout(uint32_t set_index)
{
	return *set_layouts.at(set_index);
}

std::vector<ShaderResource> PipelineLayout::get_vertex_input_attributes() const
{
	std::vector<ShaderResource> vertex_input_attributes;

	// Iterate over all resources
	for (auto it : resources)
	{
		if (it.second.stages == VK_SHADER_STAGE_VERTEX_BIT &&
		    it.second.type == ShaderResourceType::Input)
		{
			vertex_input_attributes.push_back(it.second);
		}
	}

	return vertex_input_attributes;
}

std::vector<ShaderResource> PipelineLayout::get_fragment_output_attachments() const
{
	std::vector<ShaderResource> fragment_output_attachments;

	// Iterate over all resources
	for (auto it : resources)
	{
		if (it.second.stages == VK_SHADER_STAGE_FRAGMENT_BIT &&
		    it.second.type == ShaderResourceType::Output)
		{
			fragment_output_attachments.push_back(it.second);
		}
	}

	return fragment_output_attachments;
}

std::vector<ShaderResource> PipelineLayout::get_fragment_input_attachments() const
{
	std::vector<ShaderResource> fragment_input_attachments;

	// Iterate over all resources
	for (auto it : resources)
	{
		if (it.second.stages == VK_SHADER_STAGE_FRAGMENT_BIT &&
		    it.second.type == ShaderResourceType::InputAttachment)
		{
			fragment_input_attachments.push_back(it.second);
		}
	}

	return fragment_input_attachments;
}

VkShaderStageFlags PipelineLayout::get_push_constant_range_stage(uint32_t offset, uint32_t size) const
{
	VkShaderStageFlags stages = 0;

	// Iterate over all resources
	for (auto it : resources)
	{
		if (it.second.type == ShaderResourceType::PushConstant)
		{
			if (offset >= it.second.offset && offset + size <= it.second.offset + it.second.size)
			{
				stages |= it.second.stages;
			}
		}
	}
	return stages;
}
}        // namespace vkb
