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

#include "descriptor_set_layout.h"

#include "descriptor_pool.h"
#include "descriptor_set.h"
#include "device.h"
#include "shader_module.h"

namespace vkb
{
namespace
{
inline VkDescriptorType find_descriptor_type(ShaderResourceType resource_type, bool dynamic)
{
	switch (resource_type)
	{
		case ShaderResourceType::InputAttachment:
			return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			break;
		case ShaderResourceType::Image:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			break;
		case ShaderResourceType::ImageSampler:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			break;
		case ShaderResourceType::ImageStorage:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			break;
		case ShaderResourceType::Sampler:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
			break;
		case ShaderResourceType::BufferUniform:
			if (dynamic)
			{
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			}
			else
			{
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			}
			break;
		case ShaderResourceType::BufferStorage:
			if (dynamic)
			{
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			}
			else
			{
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			}
			break;
		default:
			throw std::runtime_error("No conversion possible for the shader resource type.");
			break;
	}
}
}        // namespace

DescriptorSetLayout::DescriptorSetLayout(Device &device, const std::vector<ShaderResource> &set_resources) :
    device{device}
{
	for (auto &resource : set_resources)
	{
		// Skip shader resources whitout a binding point
		if (resource.type == ShaderResourceType::Input ||
		    resource.type == ShaderResourceType::Output ||
		    resource.type == ShaderResourceType::PushConstant ||
		    resource.type == ShaderResourceType::SpecializationConstant)
		{
			continue;
		}

		// Convert from ShaderResourceType to VkDescriptorType.
		auto descriptor_type = find_descriptor_type(resource.type, resource.dynamic);

		// Convert ShaderResource to VkDescriptorSetLayoutBinding
		VkDescriptorSetLayoutBinding layout_binding{};

		layout_binding.binding         = resource.binding;
		layout_binding.descriptorCount = resource.array_size;
		layout_binding.descriptorType  = descriptor_type;
		layout_binding.stageFlags      = static_cast<VkShaderStageFlags>(resource.stages);

		bindings.push_back(layout_binding);

		// Store mapping between binding and the binding point
		bindings_lookup.emplace(resource.binding, layout_binding);

		resources_lookup.emplace(resource.name, resource.binding);
	}

	VkDescriptorSetLayoutCreateInfo create_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};

	create_info.bindingCount = to_u32(bindings.size());
	create_info.pBindings    = bindings.data();

	// Create the Vulkan descriptor set layout handle
	VkResult result = vkCreateDescriptorSetLayout(device.get_handle(), &create_info, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create DescriptorSetLayout"};
	}

	descriptor_pool = std::make_unique<DescriptorPool>(device, *this);
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout &&other) :
    device{other.device},
    descriptor_pool{std::move(other.descriptor_pool)},
    handle{other.handle},
    bindings{std::move(other.bindings)},
    bindings_lookup{std::move(other.bindings_lookup)},
    resources_lookup{std::move(other.resources_lookup)}
{
	other.handle = VK_NULL_HANDLE;

	descriptor_pool->set_descriptor_set_layout(*this);
}

DescriptorSetLayout::~DescriptorSetLayout()
{
	// Destroy descriptor set layout
	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(device.get_handle(), handle, nullptr);
	}
}

VkDescriptorSetLayout DescriptorSetLayout::get_handle() const
{
	return handle;
}

DescriptorPool &DescriptorSetLayout::get_descriptor_pool()
{
	assert(handle != VK_NULL_HANDLE && "DescriptorSetLayout is not valid anymore");
	return *descriptor_pool;
}

const std::vector<VkDescriptorSetLayoutBinding> &DescriptorSetLayout::get_bindings() const
{
	return bindings;
}

bool DescriptorSetLayout::get_layout_binding(uint32_t binding_index, VkDescriptorSetLayoutBinding &binding) const
{
	auto it = bindings_lookup.find(binding_index);

	if (it == bindings_lookup.end())
	{
		return false;
	}

	binding = it->second;

	return true;
}

bool DescriptorSetLayout::has_layout_binding(const std::string &name, VkDescriptorSetLayoutBinding &binding) const
{
	auto it = resources_lookup.find(name);

	if (it == resources_lookup.end())
	{
		return false;
	}

	return get_layout_binding(it->second, binding);
}
}        // namespace vkb
