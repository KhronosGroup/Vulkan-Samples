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
 * @file Contains helper functions and types for working with tensors and graph pipelines.
 *       Many of these are analogous with the Image and Buffer equivalents in the sample framework.
 */

#pragma once

#include <core/allocated.h>
#include <core/sampler.h>
#include <rendering/subpass.h>

// Extend the type mapping from VkType to vk::Type for some Tensor-related structs, so that we can use some templated
// types in the sample framework.
namespace vkb
{
namespace detail
{
template <>
struct HPPType<VkTensorCreateInfoARM>
{
	using Type = vk::TensorCreateInfoARM;
};
template <>
struct HPPType<VkTensorARM>
{
	using Type = vk::TensorARM;
};
template <>
struct HPPType<VkTensorViewARM>
{
	using Type = vk::TensorViewARM;
};
template <>
struct HPPType<VkDataGraphPipelineSessionARM>
{
	using Type = vk::DataGraphPipelineSessionARM;
};
}        // namespace detail
}        // namespace vkb

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
	TensorBuilder(std::vector<uint64_t> in_dimensions);

	TensorBuilder &with_format(VkFormat format);
	TensorBuilder &with_tiling(VkTensorTilingARM tiling);
	TensorBuilder &with_usage(VkTensorUsageFlagsARM usage);

  private:
	// VkTensorCreateInfoARM (stored in the base class) has a pointer to a VkTensorDescriptionARM,
	// so we need to store that struct separately so that it outlives the pointer.
	VkTensorDescriptionARM description;
	// The description points to a dimensions array, so we need to store that array separately so that it outlives the pointer.
	std::vector<uint64_t> dimensions;
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
	VkTensorCreateInfoARM create_info;
	// create_info has a pointer to a VkTensorDescriptionARM, so we need to store that struct separately so that it outlives the pointer.
	VkTensorDescriptionARM description;
	// The description points to a dimensions array, so we need to store that array separately so that it outlives the pointer.
	std::vector<uint64_t> dimensions;
};

/*
 * @brief Helper class to create and manage the lifetime of a VkTensorARM resource. Analogous to vkb::ImageView/BufferView.
 */
class TensorView : public vkb::core::VulkanResourceC<VkTensorViewARM>
{
  public:
	TensorView(Tensor &tensor, VkFormat format = VK_FORMAT_UNDEFINED);        // VK_FORMAT_UNDEFINED means to use the same format as the provided tensor.
	~TensorView();
};

/*
 * @brief Helper struct to hold the resources needed for a constant tensor.
 */
template <typename DataType>
struct PipelineConstantTensor
{
	std::vector<uint64_t>          dimensions;
	std::vector<DataType>          constant_data;
	VkTensorDescriptionARM         tensor_description{};
	VkDataGraphPipelineConstantARM pipeline_constant{};
};

/*
 * @brief Helper class to create and manage the lifetime of a VkPipelineLayout resource for a Data Graph Pipeline. Analogous to vkb::PipelineLayout.
 * @detail This class only supports a single descriptor set, but the underyling APIs do support multiple desriptor sets.
           We also create and manage this corresponding VkDescriptorSetLayout.
           Typically, layout creation would be done using reflection on the assembled SPIR-V shader code, however spirv-cross does not yet support
           reflection on graph shaders. Therefore the caller must provide the layout themselves, but thankfully this is quite minimal (we just need to know
           the binding points for tensors).
 */
class GraphPipelineLayout : public vkb::core::VulkanResourceC<VkPipelineLayout>
{
  public:
	/*
	 * @brief Creates a GraphPipelineLayout. Assumes all tensor bindings are in the first descriptor set.
	 * @param tensor_bindings The binding numbers for every tensor, which are assumed to be in the first descriptor set.
	 */
	GraphPipelineLayout(vkb::core::DeviceC &device, std::set<uint32_t> tensor_bindings);
	~GraphPipelineLayout();

	const VkDescriptorSetLayout &get_descriptor_set_layout() const;

  private:
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
};

/*
 * @brief Helper class to create and manage the lifetime of a VkPipeline resource for a Data Graph Pipeline. Similar to vkb::ComputePipeline.
 */
class GraphPipeline : public vkb::core::VulkanResourceC<VkPipeline>
{
  public:
	/*
	 * @brief Creates a GraphPipeline.
	 * @param tensor_descriptions Descriptions (shape, format, etc.) for each tensor that will be bound to this pipeline.
	 *                            The first key in the map is the set number and the second key is the binding number.
	 */
	GraphPipeline(vkb::core::DeviceC	                                        &device,
	              VkPipelineLayout                                               layout,
	              const std::string	                                            &shader_spv_binary_path,
	              const char	                                                *entry_point,
	              std::map<uint32_t, std::map<uint32_t, VkTensorDescriptionARM>> tensor_descriptions,
	              std::vector<VkDataGraphPipelineConstantARM *>                  graph_pipeline_constants = std::vector<VkDataGraphPipelineConstantARM *>());
	~GraphPipeline();

  private:
	VkShaderModule shader_module = VK_NULL_HANDLE;
};

/*
 * @brief Helper class to create and manage the lifetime of a VkDataGraphPipelineSessionARM resource.
 * @detail Unlike compute and graphics pipelines, data graph pipelines require additional state to be stored (e.g. for intermediate results). This is stored
 *		   separately to the pipeline itself in a new 'pipeline session' resource. This requires memory to be allocated and bound to it (similar to a buffer).
 */
class GraphPipelineSession : public vkb::allocated::AllocatedC<VkDataGraphPipelineSessionARM>
{
  public:
	GraphPipelineSession(vkb::core::DeviceC &device, VkPipeline graph_pipeline, VmaAllocationCreateInfo alloc_create_info);
	~GraphPipelineSession();
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
	BlitSubpass(vkb::RenderContext &renderContext);

	void prepare() override;

	void set_source(vkb::core::ImageView *source);

	void draw(vkb::core::CommandBufferC &command_buffer) override;

  private:
	vkb::PipelineLayout                *pipeline_layout = nullptr;
	vkb::core::ImageView               *source          = nullptr;
	std::unique_ptr<vkb::core::Sampler> sampler         = nullptr;
};
