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

/*
 * @file Contains helper functions and types for working with tensors and data graph pipelines.
 *       Many of these are analogous with the Image and Buffer equivalents in the sample framework.
 */

#pragma once

#include <core/allocated.h>
#include <core/command_buffer.h>
#include <core/sampler.h>
#include <rendering/subpass.h>
#include <resource_cache.h>

/**
 * Simple wrapper around an array of elements of type T which interprets it as a multidimensional array,
 * allowing convenient access to elements using a multidimensional index.
 */
template <typename T>
struct MultidimensionalArrayView
{
	T                   *data;
	std::vector<int64_t> dimensions;

	MultidimensionalArrayView(T *data, const std::vector<int64_t> &dimensions) :
	    data(data), dimensions(dimensions)
	{
	}

	T &operator[](std::initializer_list<int64_t> indices)
	{
		if (indices.size() != dimensions.size())
		{
			throw std::runtime_error("Number of indices must match number of dimensions");
		}
		size_t index      = 0;
		size_t multiplier = 1;
		// Calculate the index based on the provided indices and dimensions
		for (int i = dimensions.size() - 1; i >= 0; --i)
		{
			index += *(indices.begin() + i) * multiplier;
			multiplier *= dimensions[i];
		}
		return data[index];
	}
};

/*
 ** @brief Helper function to write a series of image and tensor bindings to a descriptor set. Does not support descriptor arrays.
 */
void write_descriptor_set(VkDevice                                                 device,
                          VkDescriptorSet                                          set,
                          const std::map<uint32_t, VkDescriptorImageInfo>         &image_bindings,
                          const std::map<uint32_t, VkWriteDescriptorSetTensorARM> &tensor_bindings);

/*
 * @brief Creates a Tensor resource and backs it with memory. Analogous to vmaCreateImage/Buffer.
 * @detail When finished, destroy the tensor and its memory using vmaDestroyTensor.
 */
VkResult vmaCreateTensor(VkDevice                       device,
                         VmaAllocator                   allocator,
                         const VkTensorCreateInfoARM   *pTensorCreateInfo,
                         const VmaAllocationCreateInfo *pAllocationCreateInfo,
                         VkTensorARM                   *pTensor,
                         VmaAllocation                 *pAllocation,
                         VmaAllocationInfo             *pAllocationInfo);

/*
 ** @brief Destroys a Tensor resource and its backing memory, which were created from vmaCreateTensor. Analogous to vmaDestroyImage/Buffer.
 */
void vmaDestroyTensor(VkDevice      device,
                      VmaAllocator  allocator,
                      VkTensorARM   tensor,
                      VmaAllocation allocation);

/*
 * @brief Creates a VkDataGraphPipelineSessionARM resource and backs it with memory. Analogous to vmaCreateImage/Buffer.
 * @detail When finished, destroy the session and its memory using vmaDestroyDataGraphPipelineSession.
 */
VkResult vmaCreateDataGraphPipelineSession(VkDevice                                       device,
                                           VmaAllocator                                   allocator,
                                           const VkDataGraphPipelineSessionCreateInfoARM *pDataGraphPipelineSessionCreateInfo,
                                           const VmaAllocationCreateInfo                 *pAllocationCreateInfo,
                                           VkDataGraphPipelineSessionARM                 *pDataGraphPipelineSession,
                                           VmaAllocation                                 *pAllocation,
                                           VmaAllocationInfo                             *pAllocationInfo);

/*
 ** @brief Destroys a DataGraphPipelineSession resource and its backing memory, which were created from vmaCreateDataGraphPipelineSession. Analogous to vmaDestroyImage/Buffer.
 */
void vmaDestroyDataGraphPipelineSession(VkDevice                      device,
                                        VmaAllocator                  allocator,
                                        VkDataGraphPipelineSessionARM session,
                                        VmaAllocation                 allocation);

/*
 * @brief Helper class to describe a Tensor resource that is to be created (see Tensor constructor below). Analogous to vkb::ImageBuilder/BufferBuilder.
 */
class TensorBuilder : public vkb::allocated::BuilderBaseC<TensorBuilder, VkTensorCreateInfoARM>
{
  public:
	TensorBuilder(std::vector<int64_t> in_dimensions);

	TensorBuilder &with_format(VkFormat format);
	TensorBuilder &with_tiling(VkTensorTilingARM tiling);
	TensorBuilder &with_usage(VkTensorUsageFlagsARM usage);

  private:
	// VkTensorCreateInfoARM (stored in the base class) has a pointer to a VkTensorDescriptionARM,
	// so we need to store that struct separately so that it outlives the pointer.
	VkTensorDescriptionARM description;
	// The description points to a dimensions array, so we need to store that array separately so that it outlives the pointer.
	std::vector<int64_t> dimensions;
};

/*
 * @brief Common descriptor class used by Tensor and ExternallyAllocatedTensor.
 */
class TensorDescriptor
{
  public:
	TensorDescriptor(const TensorBuilder &builder);
	~TensorDescriptor() = default;

	const VkTensorCreateInfoARM &get_create_info() const;

	const VkTensorDescriptionARM &get_description() const;

  private:
	VkTensorCreateInfoARM create_info;
	// create_info has a pointer to a VkTensorDescriptionARM, so we need to store that struct separately so that it outlives the pointer.
	VkTensorDescriptionARM description;
	// The description points to a dimensions array, so we need to store that array separately so that it outlives the pointer.
	std::vector<int64_t> dimensions;
};

/*
 * @brief Helper class to create and manage the lifetime of a VkTensorARM resource. Analogous to vkb::Image/Buffer.
 */
class Tensor : public vkb::allocated::AllocatedC<VkTensorARM>
{
  public:
	Tensor(vkb::core::DeviceC &device, TensorBuilder const &builder);
	~Tensor();

	const VkTensorDescriptionARM &get_description() const;

	VkFormat get_format() const;

  private:
	TensorDescriptor descriptor;
};

/*
 * @brief Helper class to create and manage the lifetime of a VkTensorARM resource, but does not allocate
 * any its own memory. Memory must be provided on construction and is useful for creating tensors that alias existing memory.
 */
class ExternallyAllocatedTensor : public vkb::core::VulkanResourceC<VkTensorARM>
{
  public:
	ExternallyAllocatedTensor(vkb::core::DeviceC &device, TensorBuilder const &builder, VkDeviceMemory existing_memory,
	                          VkDeviceSize existing_memory_offset);
	~ExternallyAllocatedTensor();

	const VkTensorDescriptionARM &get_description() const;

	VkFormat get_format() const;

  private:
	TensorDescriptor descriptor;
};

/*
 * @brief Helper class to create and manage the lifetime of a VkTensorARM resource. Analogous to vkb::ImageView/BufferView.
 */
class TensorView : public vkb::core::VulkanResourceC<VkTensorViewARM>
{
  public:
	TensorView(Tensor &tensor, VkFormat format = VK_FORMAT_UNDEFINED);                           // VK_FORMAT_UNDEFINED means to use the same format as the provided tensor.
	TensorView(ExternallyAllocatedTensor &tensor, VkFormat format = VK_FORMAT_UNDEFINED);        // VK_FORMAT_UNDEFINED means to use the same format as the provided tensor.
	~TensorView();

  private:
	template <typename T>
	void Init(T &tensor, VkFormat format);
};

/*
 * @brief Helper struct to hold the resources needed for a constant tensor.
 */
template <typename DataType>
struct PipelineConstantTensor
{
	std::vector<int64_t>           dimensions;
	std::vector<DataType>          constant_data;
	VkTensorDescriptionARM         tensor_description{};
	VkDataGraphPipelineConstantARM pipeline_constant{};
};

/*
 * @brief Helper class to create and manage the lifetime of a VkPipelineLayout resource for a Data Graph Pipeline. Analogous to vkb::PipelineLayout.
 * @detail This class only supports a single descriptor set, but the underyling APIs do support multiple desriptor sets.
           We also create and manage this corresponding VkDescriptorSetLayout.
           Typically, layout creation would be done using reflection on the assembled SPIR-V shader code, however spirv-cross does not yet support
           reflection on data graph shaders. Therefore the caller must provide the layout themselves, but thankfully this is quite minimal (we just need to know
           the binding points for tensors).
 */
class DataGraphPipelineLayout : public vkb::core::VulkanResourceC<VkPipelineLayout>
{
  public:
	/*
	 * @brief Creates a DataGraphPipelineLayout. Assumes all tensor bindings are in the first descriptor set.
	 * @param tensor_bindings The binding numbers for every tensor, which are assumed to be in the first descriptor set.
	 */
	DataGraphPipelineLayout(vkb::core::DeviceC &device, const std::set<uint32_t> &tensor_bindings);
	~DataGraphPipelineLayout();

	const VkDescriptorSetLayout &get_descriptor_set_layout() const;

  private:
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
};

/*
 * @brief Helper class to create and manage the lifetime of a VkPipeline resource for a Data Graph Pipeline. Similar to vkb::ComputePipeline.
 */
class DataGraphPipeline : public vkb::core::VulkanResourceC<VkPipeline>
{
  public:
	/*
	 * @brief Creates a DataGraphPipeline.
	 * @param tensor_descriptions Descriptions (shape, format, etc.) for each tensor that will be bound to this pipeline.
	 *                            The first key in the map is the set number and the second key is the binding number.
	 */
	DataGraphPipeline(vkb::core::DeviceC                                                           &device,
	                  VkPipelineLayout                                                              layout,
	                  VkShaderModule                                                                shader_module,
	                  const char                                                                   *entry_point,
	                  const std::map<uint32_t, std::map<uint32_t, const VkTensorDescriptionARM *>> &tensor_descriptions,
	                  const std::vector<VkDataGraphPipelineConstantARM *>                          &data_graph_pipeline_constants = std::vector<VkDataGraphPipelineConstantARM *>());
	~DataGraphPipeline();

  private:
	VkShaderModule shader_module = VK_NULL_HANDLE;
};

/*
 * @brief Helper class to create and manage the lifetime of a VkDataGraphPipelineSessionARM resource.
 * @detail Unlike compute and graphics pipelines, data graph pipelines require additional state to be stored (e.g. for intermediate results). This is stored
 *		   separately to the pipeline itself in a new 'pipeline session' resource. This requires memory to be allocated and bound to it (similar to a buffer).
 */
class DataGraphPipelineSession : public vkb::allocated::AllocatedC<VkDataGraphPipelineSessionARM>
{
  public:
	DataGraphPipelineSession(vkb::core::DeviceC &device, VkPipeline data_graph_pipeline, VmaAllocationCreateInfo alloc_create_info);
	~DataGraphPipelineSession();
};

/*
 * @brief Helper class to create and manage the lifetime of a VkPipelineLayout resource for a Compute Pipeline. Similar to vkb::PipelineLayout, but supports Tensor resources.
 * @detail The sample framework's vkb::PipelineLayout class doesn't understand Tensor resources, so can't be used for compute shaders that use tensors.
 *		   This class is a modified copy of vkb::PipelineLayout that does support tensors, albeit with less other features.
 */
class ComputePipelineLayoutWithTensors : public vkb::core::VulkanResourceC<VkPipelineLayout>
{
  public:
	ComputePipelineLayoutWithTensors(vkb::core::DeviceC &device, vkb::ShaderModule &shader_module);
	~ComputePipelineLayoutWithTensors();

	const std::map<uint32_t, VkDescriptorSetLayout> &get_descriptor_set_layouts() const;

  private:
	std::map<uint32_t, VkDescriptorSetLayout> descriptor_set_layouts;
};

/*
 * @brief Helper class to create and manage the lifetime of a VkPipeline resource for a Compute Pipeline. Similar to vkb::ComputePipeline, but supports Tensor resources.
 * @detail The sample framework's vkb::ComputePipeline class (and its dependencies) don't understand Tensor resources, so can't be used for compute shaders that use tensors.
 *		   This class is a modified copy of vkb::ComputePipeline that does support tensors, albeit with less other features.
 *		   We can't use the vkb::PipelineState as that doesn't support tensors, so instead take the VkPipelineLayout and vkb::ShaderModule directly.
 */
class ComputePipelineWithTensors : public vkb::core::VulkanResourceC<VkPipeline>
{
  public:
	ComputePipelineWithTensors(vkb::core::DeviceC &device, VkPipelineLayout layout, vkb::ShaderModule &shader);
	~ComputePipelineWithTensors();

  private:
	VkShaderModule shader_module = VK_NULL_HANDLE;
};

/*
 * @brief Simple subpass for use with vkb::RenderPipeline, which blits an image to the render target (stretching to fit).
 */
class BlitSubpass : public vkb::rendering::SubpassC
{
  public:
	BlitSubpass(vkb::rendering::RenderContextC &renderContext, vkb::core::ImageView *source = nullptr);

	void prepare() override;

	void set_source(vkb::core::ImageView *source);

	void draw(vkb::core::CommandBufferC &command_buffer) override;

  private:
	vkb::PipelineLayout                *pipeline_layout = nullptr;
	vkb::core::ImageView               *source          = nullptr;
	std::unique_ptr<vkb::core::Sampler> sampler         = nullptr;
};
