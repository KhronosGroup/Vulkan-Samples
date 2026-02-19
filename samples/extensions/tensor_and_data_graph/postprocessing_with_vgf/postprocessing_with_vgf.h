/* Copyright (c) 2025-2026, Arm Limited and Contributors
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

#include "rendering/render_pipeline.h"

#include "../tensor_and_data_graph_common.h"
#include "vulkan_sample.h"

/**
 * @struct TensorInfo
 * @brief Describes a tensor's binding, shape, and format.
 */
struct TensorInfo
{
	uint32_t             binding;           ///< Binding index in the descriptor set.
	std::vector<int64_t> dimensions;        ///< Tensor shape dimensions.
	VkFormat             format;            ///< Format of the tensor data.
	std::vector<int8_t>  data;              ///< Constant data.
};

/**
 * @struct VgfData
 * @brief Represents the deserialized contents of a VGF file.
 *
 * This structure encapsulates all relevant information extracted from a VGF binary,
 * including tensor metadata, shader code and the entry point name.
 */
struct VgfData
{
	std::vector<TensorInfo> input_tensor_infos;
	std::vector<TensorInfo> output_tensor_infos;
	std::vector<uint32_t>   code;
	std::string             entry_point;
};

/**
 * @brief Demonstrates how to use the VGF format, which stores information about the model such as SPIR-V,
          input information, output information and constant data used to run a data graph pipeline.
 * @details A 3D scene is rendered (using the existing Vulkan Sample framework) to an offscreen Render Target (`scene_render_target`),
 *			whose colour attachment is aliased to the same memory as a Tensor (`input_tensor`). This Tensor is then used as the input
 *			to a Data Graph Pipeline (`data_graph_pipeline`), which implements a simple sharpening filter using a convolution layer.
 *			The output of this Data Graph Pipeline is written to another Tensor (`output_tensor`), which is aliased to the same memory
 *			as an Image (`output_image`), which is then used to blit the results to the Swapchain.
 *
 *			The VGF file configures the `input_tensor`, `output_tensor` and the `constant_tensors` and contains the SPIR-V required
 *			to create the VkShaderModule used by the `data_graph_pipeline`.
 *
 *			As a diagram, this looks like:
 *
 *					scene rendering -> scene_render_target                      output_image -> blit -> swapchain
 *                                            ||                                     ||
 *                                       input_tensor -> data_graph_pipeline -> output_tensor
 *										       \                ||					/
 *												\		 SPIR-V & constants		   /
 *												 \		        ||                /
 *										          \--------  vgf_data  ----------/
 *                                                              ||
 *														     load VGF
 *
 *		    Because the common Vulkan Samples framework code is not aware of the Tensor resource type or Data Graph Pipelines,
 *			generic functionality for these concepts has been added to a new tensor_and_data_graph_common.h/cpp file, which this sample
 *			(and other tensor and data graph samples) makes use of.
 */
class PostprocessingWithVgf : public vkb::VulkanSampleC
{
  public:
	PostprocessingWithVgf();

	~PostprocessingWithVgf() override;

	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;

	bool prepare(const vkb::ApplicationOptions &options) override;

	void draw_renderpass(vkb::core::CommandBufferC &command_buffer, vkb::RenderTarget &render_target) override;

	void draw_gui() override;

  private:
	VgfData load_vgf(const std::string &vgf_file_path);

	void prepare_scene_render_target(uint32_t width, uint32_t height);

	// Determines if this sample will render directly to the (aliased) input tensor, otherwise it will render
	// to a separate, dedicated image which is then copied to the input tensor.
	//
	// Although rendering to an image aliased as a texture is a perfectly valid (and encouraged!) use of
	// the extension APIs, the current implementation of the emulation layers do not have good support for this
	// kind of use and so the additional copy is a temporary workaround.
	// As the emulation layers are currently the only way to run this sample, the default behaviour
	// is to use this workaround so that the sample produces the expected output. The more faithful
	// and performant path of rendering directly to the aliased tensor is still present and instructive, but
	// cannot be executed reliably yet.
	//
	// Note: it is still possible to run and validate the 'render to aliased tensor' path using the emulation
	// layers but it requires some small changes to remove the depth attachment from the scene render target.
	// The z-order will clearly be broken, but the aliasing will work as expected.
#define TENSOR_IMAGE_ALIASING_RENDER_TO_ALIASED_IMAGE 0

#if !TENSOR_IMAGE_ALIASING_RENDER_TO_ALIASED_IMAGE
	void prepare_input_image();
#endif
	void prepare_output_image();

	void prepare_input_tensor();
	void prepare_output_tensor();

	void prepare_descriptor_pool();

	void prepare_data_graph_pipeline();

	void prepare_data_graph_pipeline_descriptor_set();

	bool enable_neural_network = true;

	std::unique_ptr<vkb::RenderTarget> scene_render_target;

#if !TENSOR_IMAGE_ALIASING_RENDER_TO_ALIASED_IMAGE
	std::unique_ptr<vkb::core::Image> input_image;
#endif

	std::unique_ptr<ExternallyAllocatedTensor> input_tensor;
	std::unique_ptr<TensorView>                input_tensor_view;

	std::unique_ptr<vkb::core::Image>     output_image;
	std::unique_ptr<vkb::core::ImageView> output_image_view;

	std::unique_ptr<ExternallyAllocatedTensor> output_tensor;
	std::unique_ptr<TensorView>                output_tensor_view;

	std::unique_ptr<vkb::RenderPipeline> blit_pipeline;

	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

	std::vector<std::unique_ptr<PipelineConstantTensor<int8_t>>> constant_tensors;

	std::unique_ptr<DataGraphPipelineLayout>  data_graph_pipeline_layout;
	std::unique_ptr<DataGraphPipeline>        data_graph_pipeline;
	std::unique_ptr<DataGraphPipelineSession> data_graph_pipeline_session;

	VkDescriptorSet data_graph_pipeline_descriptor_set = VK_NULL_HANDLE;

	VgfData vgf_data;
};

std::unique_ptr<vkb::VulkanSampleC> create_postprocessing_with_vgf();
