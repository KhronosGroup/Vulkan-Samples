/* Copyright (c) 2024-2025, Arm Limited and Contributors
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

#include "tensor_and_data_graph_common.h"

#include <list>
#include <rendering/render_context.h>
#include <spirv_glsl.hpp>

void write_descriptor_set(VkDevice                                                 device,
                          VkDescriptorSet                                          set,
                          const std::map<uint32_t, VkDescriptorImageInfo>         &image_bindings,
                          const std::map<uint32_t, VkWriteDescriptorSetTensorARM> &tensor_bindings)
{
	std::vector<VkWriteDescriptorSet> writes;

	for (const auto &image_binding : image_bindings)
	{
		VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		write.dstSet          = set;
		write.dstBinding      = image_binding.first;
		write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		write.pImageInfo      = &image_binding.second;
		write.descriptorCount = 1;
		writes.push_back(write);
	}
	for (const auto &tensor_binding : tensor_bindings)
	{
		VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		write.dstSet          = set;
		write.dstBinding      = tensor_binding.first;
		write.descriptorType  = VK_DESCRIPTOR_TYPE_TENSOR_ARM;
		write.pNext           = &tensor_binding.second;        // Tensor info is provided via pNext, rather than a pTensorInfo like for images/buffers
		write.descriptorCount = 1;
		writes.push_back(write);
	}

	vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
}

VkResult vmaCreateTensor(VkDevice                       device,
                         VmaAllocator                   allocator,
                         const VkTensorCreateInfoARM   *pTensorCreateInfo,
                         const VmaAllocationCreateInfo *pAllocationCreateInfo,
                         VkTensorARM                   *pTensor,
                         VmaAllocation                 *pAllocation,
                         VmaAllocationInfo             *pAllocationInfo)
{
	// Note that this implementation has some slight differences to the equivalent vmaCreateImage/Buffer functions because we are outside
	// the VMA implementation so can't use any of its internal functions and have to use the public APIs instead.

	if (pTensorCreateInfo == nullptr ||
	    pTensorCreateInfo->pDescription == nullptr ||
	    pTensor == nullptr ||
	    pAllocation == nullptr)
	{
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	*pTensor     = VK_NULL_HANDLE;
	*pAllocation = VK_NULL_HANDLE;

	// 1. Create VkTensor.
	VkResult res = vkCreateTensorARM(
	    device,
	    pTensorCreateInfo,
	    nullptr,
	    pTensor);
	if (res >= 0)
	{
		// 2. vkGetTensorMemoryRequirements.
		VkMemoryRequirements2 vkMemReq                             = {};
		vkMemReq.sType                                             = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		VkTensorMemoryRequirementsInfoARM memory_requirements_info = {VK_STRUCTURE_TYPE_TENSOR_MEMORY_REQUIREMENTS_INFO_ARM};
		memory_requirements_info.tensor                            = *pTensor;
		vkGetTensorMemoryRequirementsARM(device, &memory_requirements_info, &vkMemReq);

		// 3. Allocate memory using allocator.
		VmaAllocationInfo allocation_info;
		res = vmaAllocateMemory(allocator,
		                        &vkMemReq.memoryRequirements,
		                        pAllocationCreateInfo,
		                        pAllocation,
		                        &allocation_info);

		if (res >= 0)
		{
			if (pAllocationInfo != nullptr)
			{
				// Return allocation info to caller, if requested
				*pAllocationInfo = allocation_info;
			}

			// 4. Bind tensor with memory.
			if ((pAllocationCreateInfo->flags & VMA_ALLOCATION_CREATE_DONT_BIND_BIT) == 0)
			{
				VkBindTensorMemoryInfoARM bind_tensor_memory_info = {VK_STRUCTURE_TYPE_BIND_TENSOR_MEMORY_INFO_ARM};
				bind_tensor_memory_info.tensor                    = *pTensor;
				bind_tensor_memory_info.memory                    = allocation_info.deviceMemory;
				bind_tensor_memory_info.memoryOffset              = allocation_info.offset;
				res                                               = vkBindTensorMemoryARM(device, 1, &bind_tensor_memory_info);
			}
			if (res >= 0)
			{
				// All steps succeeded.
				return VK_SUCCESS;
			}
			vmaFreeMemory(allocator, *pAllocation);
			*pAllocation = VK_NULL_HANDLE;
		}
		vkDestroyTensorARM(device, *pTensor, nullptr);
		*pTensor = VK_NULL_HANDLE;
		return res;
	}
	return res;
}

void vmaDestroyTensor(VkDevice      device,
                      VmaAllocator  allocator,
                      VkTensorARM   tensor,
                      VmaAllocation allocation)
{
	if (tensor != VK_NULL_HANDLE)
	{
		vkDestroyTensorARM(device, tensor, nullptr);
	}

	if (allocation != VK_NULL_HANDLE)
	{
		vmaFreeMemory(allocator, allocation);
	}
}

VkResult vmaCreateDataGraphPipelineSession(VkDevice                                       device,
                                           VmaAllocator                                   allocator,
                                           const VkDataGraphPipelineSessionCreateInfoARM *pDataGraphPipelineSessionCreateInfo,
                                           const VmaAllocationCreateInfo                 *pAllocationCreateInfo,
                                           VkDataGraphPipelineSessionARM                 *pDataGraphPipelineSession,
                                           VmaAllocation                                 *pAllocation,
                                           VmaAllocationInfo                             *pAllocationInfo)
{
	// Note that this implementation has some slight differences to the equivalent vmaCreateImage/Buffer functions because we are outside
	// the VMA implementation so can't use any of its internal functions and have to use the public APIs instead.

	if (pDataGraphPipelineSessionCreateInfo == nullptr ||
	    pDataGraphPipelineSession == nullptr ||
	    pAllocation == nullptr)
	{
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	*pDataGraphPipelineSession = VK_NULL_HANDLE;
	*pAllocation               = VK_NULL_HANDLE;

	// 1. Create DataGraphPipelineSession.
	VkResult res = vkCreateDataGraphPipelineSessionARM(
	    device,
	    pDataGraphPipelineSessionCreateInfo,
	    nullptr,
	    pDataGraphPipelineSession);
	if (res >= 0)
	{
		// 2. Query valid bind points for the session
		VkDataGraphPipelineSessionBindPointRequirementsInfoARM bind_point_req_info = {VK_STRUCTURE_TYPE_DATA_GRAPH_PIPELINE_SESSION_BIND_POINT_REQUIREMENTS_INFO_ARM};
		bind_point_req_info.session                                                = *pDataGraphPipelineSession;
		uint32_t requirement_count                                                 = 0;

		res = vkGetDataGraphPipelineSessionBindPointRequirementsARM(device, &bind_point_req_info, &requirement_count, nullptr);
		if (res != VK_SUCCESS)
		{
			return res;
		}

		if (requirement_count > 1)
		{
			// A session could require more than one bind point, but for simplicity we only support one bind point type in this function.
			return VK_ERROR_INITIALIZATION_FAILED;
		}

		VkDataGraphPipelineSessionBindPointRequirementARM bind_point_requirement{VK_STRUCTURE_TYPE_DATA_GRAPH_PIPELINE_SESSION_BIND_POINT_REQUIREMENT_ARM};
		res = vkGetDataGraphPipelineSessionBindPointRequirementsARM(device, &bind_point_req_info, &requirement_count, &bind_point_requirement);
		if (res != VK_SUCCESS)
		{
			return res;
		}

		if (bind_point_requirement.numObjects > 1)
		{
			// A single bind point requirement could require more than one object, but for simplicity we only support one object type in this function.
			return VK_ERROR_INITIALIZATION_FAILED;
		}
		if (bind_point_requirement.bindPointType != VK_DATA_GRAPH_PIPELINE_SESSION_BIND_POINT_TYPE_MEMORY_ARM)
		{
			// Currently we only support the memory bind point type
			return VK_ERROR_INITIALIZATION_FAILED;
		}

		// 3. vkGetDataGraphPipelineSessionMemoryRequirements.
		VkDataGraphPipelineSessionBindPointARM              memory_bind_point        = VK_DATA_GRAPH_PIPELINE_SESSION_BIND_POINT_TRANSIENT_ARM;
		VkMemoryRequirements2                               vkMemReq                 = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
		VkDataGraphPipelineSessionMemoryRequirementsInfoARM memory_requirements_info = {VK_STRUCTURE_TYPE_DATA_GRAPH_PIPELINE_SESSION_MEMORY_REQUIREMENTS_INFO_ARM};
		memory_requirements_info.session                                             = *pDataGraphPipelineSession;
		memory_requirements_info.bindPoint                                           = memory_bind_point;
		memory_requirements_info.objectIndex                                         = 0;
		vkGetDataGraphPipelineSessionMemoryRequirementsARM(device, &memory_requirements_info, &vkMemReq);

		if (vkMemReq.memoryRequirements.size > 0)
		{
			// 4. Allocate memory using allocator.
			VmaAllocationInfo allocation_info;
			res = vmaAllocateMemory(allocator,
			                        &vkMemReq.memoryRequirements,
			                        pAllocationCreateInfo,
			                        pAllocation,
			                        &allocation_info);

			if (res >= 0)
			{
				if (pAllocationInfo != nullptr)
				{
					// Return allocation info to caller, if requested
					*pAllocationInfo = allocation_info;
				}

				// 5. Bind session with memory.
				if ((pAllocationCreateInfo->flags & VMA_ALLOCATION_CREATE_DONT_BIND_BIT) == 0)
				{
					VkBindDataGraphPipelineSessionMemoryInfoARM bind_info = {VK_STRUCTURE_TYPE_BIND_DATA_GRAPH_PIPELINE_SESSION_MEMORY_INFO_ARM};
					bind_info.session                                     = *pDataGraphPipelineSession;
					bind_info.memory                                      = allocation_info.deviceMemory;
					bind_info.memoryOffset                                = allocation_info.offset;
					bind_info.bindPoint                                   = memory_bind_point;
					bind_info.objectIndex                                 = 0;
					res                                                   = vkBindDataGraphPipelineSessionMemoryARM(device, 1, &bind_info);
				}

				if (res >= 0)
				{
					// All steps succeeded.
					return VK_SUCCESS;
				}

				// Cleanup in case of errors
				vmaFreeMemory(allocator, *pAllocation);
				*pAllocation = VK_NULL_HANDLE;
			}

			// Cleanup in case of errors
			vkDestroyDataGraphPipelineSessionARM(device, *pDataGraphPipelineSession, nullptr);
			*pDataGraphPipelineSession = VK_NULL_HANDLE;
		}
	}

	return res;
}

void vmaDestroyDataGraphPipelineSession(VkDevice                      device,
                                        VmaAllocator                  allocator,
                                        VkDataGraphPipelineSessionARM tensor,
                                        VmaAllocation                 allocation)
{
	if (tensor != VK_NULL_HANDLE)
	{
		vkDestroyDataGraphPipelineSessionARM(device, tensor, nullptr);
	}

	if (allocation != VK_NULL_HANDLE)
	{
		vmaFreeMemory(allocator, allocation);
	}
}

TensorBuilder::TensorBuilder(std::vector<int64_t> in_dimensions) :
    vkb::allocated::BuilderBaseC<TensorBuilder, VkTensorCreateInfoARM>(VkTensorCreateInfoARM{VK_STRUCTURE_TYPE_TENSOR_CREATE_INFO_ARM}),
    dimensions(std::move(in_dimensions)),
    description{VK_STRUCTURE_TYPE_TENSOR_DESCRIPTION_ARM}
{
	description.dimensionCount     = dimensions.size();
	description.pDimensions        = dimensions.data();        // Note we point to the dimensions array stored in this object, not the one passed in (which is already empty!)
	get_create_info().pDescription = &description;

	alloc_create_info.usage = VMA_MEMORY_USAGE_UNKNOWN;        // The default value set by the base class of 'VMA_MEMORY_USAGE_AUTO' won't work for tensors

	description.tiling = VK_TENSOR_TILING_LINEAR_ARM;
	description.usage  = VK_TENSOR_USAGE_SHADER_BIT_ARM;
	description.format = VK_FORMAT_R32_SFLOAT;
}

TensorBuilder &TensorBuilder::with_format(VkFormat format)
{
	description.format = format;
	return *this;
}
TensorBuilder &TensorBuilder::with_tiling(VkTensorTilingARM tiling)
{
	description.tiling = tiling;
	return *this;
}
TensorBuilder &TensorBuilder::with_usage(VkTensorUsageFlagsARM usage)
{
	description.usage = usage;
	return *this;
}

TensorDescriptor::TensorDescriptor(const TensorBuilder &builder)
{
	// Note that we need to do a deep copy of this struct as it contains a couple of pointers.
	create_info              = builder.get_create_info();
	description              = *create_info.pDescription;
	create_info.pDescription = &description;
	dimensions.assign(description.pDimensions, description.pDimensions + description.dimensionCount);
	description.pDimensions = dimensions.data();
}

const VkTensorCreateInfoARM &TensorDescriptor::get_create_info() const
{
	return create_info;
}

const VkTensorDescriptionARM &TensorDescriptor::get_description() const
{
	return description;
}

Tensor::Tensor(vkb::core::DeviceC &device, TensorBuilder const &builder) :
    vkb::allocated::AllocatedC<VkTensorARM>(builder.get_allocation_create_info(), nullptr, &device),
    descriptor(builder)
{
	VkTensorARM       tensor = VK_NULL_HANDLE;
	VmaAllocationInfo allocation_info{};
	VmaAllocation     allocation = VK_NULL_HANDLE;
	VK_CHECK(vmaCreateTensor(
	    device.get_handle(),
	    vkb::allocated::get_memory_allocator(),
	    &descriptor.get_create_info(),
	    &builder.get_allocation_create_info(),
	    &tensor,
	    &allocation,
	    &allocation_info));

	set_allocation(allocation);
	post_create(allocation_info);
	set_handle(tensor);
	if (!builder.get_debug_name().empty())
	{
		set_debug_name(builder.get_debug_name());
	}
}

Tensor::~Tensor()
{
	if (get_handle() != VK_NULL_HANDLE && get_allocation() != VK_NULL_HANDLE)
	{
		unmap();
		vmaDestroyTensor(get_device().get_handle(), vkb::allocated::get_memory_allocator(), get_handle(), get_allocation());
		clear();
	}
}

const VkTensorDescriptionARM &Tensor::get_description() const
{
	return descriptor.get_description();
}

VkFormat Tensor::get_format() const
{
	return descriptor.get_create_info().pDescription->format;
};

ExternallyAllocatedTensor::ExternallyAllocatedTensor(vkb::core::DeviceC &device, TensorBuilder const &builder, VkDeviceMemory existing_memory,
                                                     VkDeviceSize existing_memory_offset) :
    vkb::core::VulkanResourceC<VkTensorARM>(VK_NULL_HANDLE, &device),        // Handle will be set later in the constructor
    descriptor(builder)
{
	// Create tensor
	VkTensorARM tensor = VK_NULL_HANDLE;
	VkResult    res    = vkCreateTensorARM(device.get_handle(), &descriptor.get_create_info(), nullptr, &tensor);

	// Bind it to existing memory
	VkBindTensorMemoryInfoARM bind_info = {VK_STRUCTURE_TYPE_BIND_TENSOR_MEMORY_INFO_ARM,
	                                       nullptr, tensor, existing_memory, existing_memory_offset};
	vkBindTensorMemoryARM(device.get_handle(), 1, &bind_info);

	set_handle(tensor);
	if (!builder.get_debug_name().empty())
	{
		set_debug_name(builder.get_debug_name());
	}
}

ExternallyAllocatedTensor::~ExternallyAllocatedTensor()
{
	if (get_handle() != VK_NULL_HANDLE)
	{
		vkDestroyTensorARM(get_device().get_handle(), get_handle(), nullptr);
	}
}

const VkTensorDescriptionARM &ExternallyAllocatedTensor::get_description() const
{
	return descriptor.get_description();
}

VkFormat ExternallyAllocatedTensor::get_format() const
{
	return descriptor.get_create_info().pDescription->format;
};

TensorView::TensorView(Tensor &tensor, VkFormat format) :
    vkb::core::VulkanResourceC<VkTensorViewARM>(VK_NULL_HANDLE, &tensor.get_device())
{
	Init(tensor, format);
}

TensorView::TensorView(ExternallyAllocatedTensor &tensor, VkFormat format) :
    vkb::core::VulkanResourceC<VkTensorViewARM>(VK_NULL_HANDLE, &tensor.get_device())
{
	Init(tensor, format);
}

template <typename T>
void TensorView::Init(T &tensor, VkFormat format)
{
	if (format == VK_FORMAT_UNDEFINED)        // VK_FORMAT_UNDEFINED means to use the same format as the provided tensor.
	{
		format = tensor.get_format();
	}

	VkTensorViewCreateInfoARM view_info{VK_STRUCTURE_TYPE_TENSOR_VIEW_CREATE_INFO_ARM};
	view_info.tensor = tensor.get_handle();
	view_info.format = format;
	VK_CHECK(vkCreateTensorViewARM(get_device().get_handle(), &view_info, nullptr, &get_handle()));
}

template void TensorView::Init<Tensor>(Tensor &, VkFormat);
template void TensorView::Init<ExternallyAllocatedTensor>(ExternallyAllocatedTensor &, VkFormat);

TensorView::~TensorView()
{
	vkDestroyTensorViewARM(get_device().get_handle(), get_handle(), nullptr);
}

DataGraphPipelineLayout::DataGraphPipelineLayout(vkb::core::DeviceC &device, const std::set<uint32_t> &tensor_bindings) :
    vkb::core::VulkanResourceC<VkPipelineLayout>(VK_NULL_HANDLE, &device)
{
	std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
	for (uint32_t binding : tensor_bindings)
	{
		VkDescriptorSetLayoutBinding layout_binding{};
		layout_binding.binding         = binding;
		layout_binding.descriptorCount = 1;
		layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_TENSOR_ARM;
		layout_binding.stageFlags      = VK_SHADER_STAGE_ALL;        // Data graph pipelines don't have shader stages per-se, so VK_SHADER_STAGE_ALL is used.
		layout_bindings.push_back(layout_binding);
	}

	// Create set layout
	VkDescriptorSetLayoutCreateInfo set_layout_create_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	set_layout_create_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
	set_layout_create_info.pBindings    = layout_bindings.data();
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &set_layout_create_info, nullptr, &descriptor_set_layout));

	// Create pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_create_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	pipeline_layout_create_info.setLayoutCount = 1;
	pipeline_layout_create_info.pSetLayouts    = &descriptor_set_layout;
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &get_handle()));
}

DataGraphPipelineLayout::~DataGraphPipelineLayout()
{
	vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
	vkDestroyPipelineLayout(get_device().get_handle(), get_handle(), nullptr);
}

const VkDescriptorSetLayout &DataGraphPipelineLayout::get_descriptor_set_layout() const
{
	return descriptor_set_layout;
}

DataGraphPipeline::DataGraphPipeline(vkb::core::DeviceC                                                           &device,
                                     VkPipelineLayout                                                              layout,
                                     VkShaderModule                                                                shader_module,
                                     const char                                                                   *entry_point,
                                     const std::map<uint32_t, std::map<uint32_t, const VkTensorDescriptionARM *>> &tensor_descriptions,
                                     const std::vector<VkDataGraphPipelineConstantARM *>                          &data_graph_pipeline_constants) :
    vkb::core::VulkanResourceC<VkPipeline>(VK_NULL_HANDLE, &device), shader_module(shader_module)
{
	// Create array of data graph pipeline resource infos (one for each input/output tensor)
	std::vector<VkDataGraphPipelineResourceInfoARM> resource_infos;
	for (const auto &tensor_descriptions_set : tensor_descriptions)
	{
		uint32_t                                                  set_idx                      = tensor_descriptions_set.first;
		const std::map<uint32_t, const VkTensorDescriptionARM *> &tensor_descriptions_this_set = tensor_descriptions_set.second;

		for (const auto &tensor_description_binding : tensor_descriptions_this_set)
		{
			const VkTensorDescriptionARM *tensor_description = tensor_description_binding.second;

			VkDataGraphPipelineResourceInfoARM resource_info = {VK_STRUCTURE_TYPE_DATA_GRAPH_PIPELINE_RESOURCE_INFO_ARM};
			resource_info.pNext                              = tensor_description;
			resource_info.descriptorSet                      = set_idx;
			resource_info.binding                            = tensor_description_binding.first;
			resource_infos.push_back(resource_info);
		}
	}

	// Create data graph pipeline
	VkDataGraphPipelineShaderModuleCreateInfoARM pipeline_shader_module_create_info = {VK_STRUCTURE_TYPE_DATA_GRAPH_PIPELINE_SHADER_MODULE_CREATE_INFO_ARM};
	pipeline_shader_module_create_info.module                                       = shader_module;
	pipeline_shader_module_create_info.pName                                        = entry_point;

	std::vector<VkDataGraphPipelineConstantARM> constants_array(data_graph_pipeline_constants.size());
	if (!data_graph_pipeline_constants.empty())
	{
		for (uint32_t i = 0; i < data_graph_pipeline_constants.size(); i++)
		{
			constants_array[i] = *data_graph_pipeline_constants[i];
		}

		pipeline_shader_module_create_info.constantCount = data_graph_pipeline_constants.size();
		pipeline_shader_module_create_info.pConstants    = constants_array.data();
	}

	VkDataGraphPipelineCreateInfoARM pipeline_create_info{VK_STRUCTURE_TYPE_DATA_GRAPH_PIPELINE_CREATE_INFO_ARM};
	pipeline_create_info.pNext             = &pipeline_shader_module_create_info;
	pipeline_create_info.layout            = layout;
	pipeline_create_info.resourceInfoCount = resource_infos.size();
	pipeline_create_info.pResourceInfos    = resource_infos.data();

	VK_CHECK(vkCreateDataGraphPipelinesARM(get_device().get_handle(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &get_handle()));
}

DataGraphPipeline::~DataGraphPipeline()
{
	vkDestroyShaderModule(get_device().get_handle(), shader_module, nullptr);
	vkDestroyPipeline(get_device().get_handle(), get_handle(), nullptr);
}

DataGraphPipelineSession::DataGraphPipelineSession(vkb::core::DeviceC     &device,
                                                   VkPipeline              data_graph_pipeline,
                                                   VmaAllocationCreateInfo alloc_create_info) :
    vkb::allocated::AllocatedC<VkDataGraphPipelineSessionARM>(alloc_create_info, nullptr, &device)
{
	VkDataGraphPipelineSessionCreateInfoARM pipeline_session_create_info = {};
	pipeline_session_create_info.sType                                   = VK_STRUCTURE_TYPE_DATA_GRAPH_PIPELINE_SESSION_CREATE_INFO_ARM;
	pipeline_session_create_info.dataGraphPipeline                       = data_graph_pipeline;

	VkDataGraphPipelineSessionARM data_graph_pipeline_session = VK_NULL_HANDLE;
	VmaAllocationInfo             allocation_info{};
	VmaAllocation                 allocation = VK_NULL_HANDLE;
	VK_CHECK(vmaCreateDataGraphPipelineSession(
	    device.get_handle(),
	    vkb::allocated::get_memory_allocator(),
	    &pipeline_session_create_info,
	    &alloc_create_info,
	    &data_graph_pipeline_session,
	    &allocation,
	    &allocation_info));

	set_allocation(allocation);
	if (allocation_info.size > 0)        // Sometimes no memory is needed, which is fine.
	{
		post_create(allocation_info);
	}
	set_handle(data_graph_pipeline_session);
}

DataGraphPipelineSession::~DataGraphPipelineSession()
{
	if (get_handle() != VK_NULL_HANDLE)
	{
		vmaDestroyDataGraphPipelineSession(get_device().get_handle(), vkb::allocated::get_memory_allocator(), get_handle(), get_allocation());
		clear();
	}
}

ComputePipelineLayoutWithTensors::ComputePipelineLayoutWithTensors(vkb::core::DeviceC &device, vkb::ShaderModule &shader_module) :
    vkb::core::VulkanResourceC<VkPipelineLayout>(nullptr, &device)
{
	// Create a regular vkb::PipelineLayout to reflect all the regular shader resources except tensors
	std::unique_ptr<vkb::PipelineLayout> layout_without_tensors = std::make_unique<vkb::PipelineLayout>(device, std::vector<vkb::ShaderModule *>{&shader_module});

	// Gather all the binding info that was found
	std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> all_bindings;
	for (const std::pair<uint32_t, std::vector<vkb::ShaderResource>> set_and_resources : layout_without_tensors->get_shader_sets())
	{
		uint32_t set_idx      = set_and_resources.first;
		all_bindings[set_idx] = layout_without_tensors->get_descriptor_set_layout(set_idx).get_bindings();
	}

	// Add tensor resources using reflection of the SPIR-V binary
	spirv_cross::CompilerGLSL          compiler{shader_module.get_binary()};
	spirv_cross::CompilerGLSL::Options opts = compiler.get_common_options();
	opts.enable_420pack_extension           = true;
	compiler.set_common_options(opts);

	spirv_cross::SmallVector<spirv_cross::Resource> tensor_resources = compiler.get_shader_resources().tensors;
	for (const spirv_cross::Resource &tensor_resource : tensor_resources)
	{
		uint32_t set_idx = compiler.get_decoration(tensor_resource.id, spv::DecorationDescriptorSet);
		uint32_t binding = compiler.get_decoration(tensor_resource.id, spv::DecorationBinding);

		VkDescriptorSetLayoutBinding layout_binding{};

		layout_binding.binding         = binding;
		layout_binding.descriptorCount = 1;        // Assume this isn't an array (though this support could be added)
		layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_TENSOR_ARM;
		layout_binding.stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

		all_bindings[set_idx].push_back(layout_binding);
	}

	// Create set layouts now that we have the full set of bindings
	std::vector<VkDescriptorSetLayout> descriptor_set_layouts_array;        // As well as storing a std::map of descriptor set layouts, we need a linear array for use in VkPipelineLayoutCreateInfo
	for (const std::pair<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> &set_idx_and_bindings : all_bindings)
	{
		uint32_t                                         set_idx  = set_idx_and_bindings.first;
		const std::vector<VkDescriptorSetLayoutBinding> &bindings = set_idx_and_bindings.second;

		VkDescriptorSetLayoutCreateInfo create_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		create_info.bindingCount = bindings.size();
		create_info.pBindings    = bindings.data();

		VkDescriptorSetLayout set_layout;
		VK_CHECK(vkCreateDescriptorSetLayout(device.get_handle(), &create_info, nullptr, &set_layout));

		descriptor_set_layouts[set_idx] = set_layout;
		descriptor_set_layouts_array.push_back(set_layout);
	}

	// Create pipeline layout using these layouts
	VkPipelineLayoutCreateInfo create_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	create_info.setLayoutCount = descriptor_set_layouts_array.size();
	create_info.pSetLayouts    = descriptor_set_layouts_array.data();

	// Collect all the push constant shader resources
	std::vector<VkPushConstantRange> push_constant_ranges;
	for (const auto &push_constant_resource : layout_without_tensors->get_resources(vkb::ShaderResourceType::PushConstant))
	{
		push_constant_ranges.push_back({push_constant_resource.stages, push_constant_resource.offset, push_constant_resource.size});
	}
	create_info.pushConstantRangeCount = push_constant_ranges.size();
	create_info.pPushConstantRanges    = push_constant_ranges.data();

	// Finally we can create the pipeline layout
	VK_CHECK(vkCreatePipelineLayout(device.get_handle(), &create_info, nullptr, &get_handle()));
}

ComputePipelineLayoutWithTensors::~ComputePipelineLayoutWithTensors()
{
	for (const std::pair<uint32_t, VkDescriptorSetLayout> &set_idx_and_layout : descriptor_set_layouts)
	{
		vkDestroyDescriptorSetLayout(get_device().get_handle(), set_idx_and_layout.second, nullptr);
	}
	vkDestroyPipelineLayout(get_device().get_handle(), get_handle(), nullptr);
}

const std::map<uint32_t, VkDescriptorSetLayout> &ComputePipelineLayoutWithTensors::get_descriptor_set_layouts() const
{
	return descriptor_set_layouts;
}

ComputePipelineWithTensors::ComputePipelineWithTensors(vkb::core::DeviceC &device, VkPipelineLayout layout, vkb::ShaderModule &shader) :
    vkb::core::VulkanResourceC<VkPipeline>(VK_NULL_HANDLE, &device)
{
	// Create shader module
	VkShaderModuleCreateInfo module_create_info{};
	module_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_create_info.codeSize = shader.get_binary().size() * sizeof(uint32_t);
	module_create_info.pCode    = shader.get_binary().data();
	VK_CHECK(vkCreateShaderModule(device.get_handle(), &module_create_info, NULL, &shader_module));

	// Create compute pipeline
	VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
	stage.pName  = shader.get_entry_point().c_str();
	stage.module = shader_module;

	VkComputePipelineCreateInfo create_info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
	create_info.layout = layout;
	create_info.stage  = stage;

	VK_CHECK(vkCreateComputePipelines(device.get_handle(), VK_NULL_HANDLE, 1, &create_info, nullptr, &get_handle()));
}

ComputePipelineWithTensors::~ComputePipelineWithTensors()
{
	vkDestroyShaderModule(get_device().get_handle(), shader_module, nullptr);
	vkDestroyPipeline(get_device().get_handle(), get_handle(), nullptr);
}

BlitSubpass::BlitSubpass(vkb::rendering::RenderContextC &renderContext, vkb::core::ImageView *source) :
    vkb::rendering::SubpassC(renderContext, vkb::ShaderSource{"tensor_and_data_graph/glsl/fullscreen.vert.spv"}, vkb::ShaderSource{"tensor_and_data_graph/glsl/blit.frag.spv"}),
    source(source)
{
}

void BlitSubpass::prepare()
{
	vkb::ShaderModule &fullscreen_vert =
	    get_render_context().get_device().get_resource_cache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, vkb::ShaderSource{"tensor_and_data_graph/glsl/fullscreen.vert.spv"});
	vkb::ShaderModule &blit_frag =
	    get_render_context().get_device().get_resource_cache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, vkb::ShaderSource{"tensor_and_data_graph/glsl/blit.frag.spv"});
	pipeline_layout = &get_render_context().get_device().get_resource_cache().request_pipeline_layout({&fullscreen_vert, &blit_frag});

	VkSamplerCreateInfo sampler_create_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	sampler_create_info.minFilter    = VK_FILTER_LINEAR;
	sampler_create_info.magFilter    = VK_FILTER_LINEAR;
	sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler                          = std::make_unique<vkb::core::Sampler>(get_render_context().get_device(), sampler_create_info);
}

void BlitSubpass::set_source(vkb::core::ImageView *source)
{
	this->source = source;
}

void BlitSubpass::draw(vkb::core::CommandBufferC &command_buffer)
{
	vkb::RasterizationState rasterization_state = {};
	rasterization_state.cull_mode               = VK_CULL_MODE_NONE;
	command_buffer.set_rasterization_state(rasterization_state);

	vkb::DepthStencilState depth_stencil_state = {};
	depth_stencil_state.depth_test_enable      = VK_FALSE;
	command_buffer.set_depth_stencil_state(depth_stencil_state);

	command_buffer.bind_pipeline_layout(*pipeline_layout);
	command_buffer.bind_image(*source, *sampler, 0, 0, 0);
	command_buffer.draw(3, 1, 0, 0);
}
