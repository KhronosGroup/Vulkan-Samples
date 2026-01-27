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

#include "vulkan_sample.h"

#include "../tensor_and_data_graph_common.h"

#include <memory>
#include <vector>

/**
 * @brief Demonstrates how to use the VK_ARM_tensors and VK_ARM_data_graph extensions in a simple example
 *        which runs a trivial convolutional neural network with the input being generated via a compute shader.
 * @details A `Tensor` resource (`input_tensor`) is created by the preprocessing compute shader. This is read as an input by a `Data Graph Pipeline`
 *          (`data_graph_pipeline`).
 *          The Data Graph Pipeline runs a neural network and produces an output which is written into the `output_tensor` Tensor.
 *          Two constant tensors (`constant_weights_tensor` & `constant_bias_tensor`) are also passed into the Convolution2d operator in the form of weights and bias.
 *          The `output_tensor` is then copied to `postprocessed_tensor` by another compute shader simply for demonstrative purposes.
 *          In reality this shader would be used for postprocessing, hence the shader name (`postprocessing.comp`).
 *          In order to visualize the results of this pipeline, there is a Compute Pipeline (`visualization_pipeline`) which copies the
 *          contents of the `input_tensor`, `output_tensor` and `postprocessed_tensor` into an Image (`output_image`) which is blitted to the Swapchain.
 *
 *			As a diagram, this looks like:
 *
 *                                    constant_weights_tensor     constant_bias_tensor
 *                                                       \         /
 *			preprocessing_pipeline -> input_tensor -> data_graph_pipeline -> output_tensor -> postprocessing_pipeline -> postprocessed_tensor
 *                                          \                                 											\
 *                                           \--------------------------------------------------------------> visualization_pipeline -> output_image -> blit -> swapchain
 *
 *		    Because the common Vulkan Samples framework code is not aware of the Tensor resource type or Data Graph Pipelines,
 *			generic functionality for these concepts has been added to a new tensor_and_data_graph_common.h/cpp file, which this sample
 *			(and other tensor and data graph samples) makes use of.
 */
class ComputeShadersWithTensors : public vkb::VulkanSampleC
{
  public:
	ComputeShadersWithTensors();
	~ComputeShadersWithTensors();

	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;

	bool prepare(const vkb::ApplicationOptions &options) override;

	bool resize(uint32_t width, uint32_t height) override;

	void draw_renderpass(vkb::core::CommandBufferC &command_buffer, RenderTargetType &render_target) override;

	void draw_gui() override;

  private:
	// from vkb::VulkanSample
	uint32_t get_api_version() const override;

  private:
	void prepare_descriptor_pool();

	void prepare_input_tensor();
	void prepare_weights_tensor();
	void prepare_bias_tensor();
	void prepare_output_tensors();
	void prepare_output_image(uint32_t width, uint32_t height);

	void prepare_data_graph_pipeline();
	void prepare_data_graph_pipeline_descriptor_set();

	void prepare_preprocessing_pipeline();
	void prepare_preprocessing_pipeline_descriptor_set();

	void prepare_postprocessing_pipeline();
	void prepare_postprocessing_pipeline_descriptor_set();

	void prepare_visualization_pipeline();
	void prepare_visualization_pipeline_descriptor_set();

	std::unique_ptr<Tensor>     input_tensor;
	std::unique_ptr<TensorView> input_tensor_view;

	std::unique_ptr<Tensor>     output_tensor;
	std::unique_ptr<TensorView> output_tensor_view;

	std::unique_ptr<Tensor>     postprocessed_tensor;
	std::unique_ptr<TensorView> postprocessed_tensor_view;

	std::unique_ptr<vkb::core::Image>     output_image;
	std::unique_ptr<vkb::core::ImageView> output_image_view;

	// Structs to hold everything needed for constant tensors (Weights and Bias)
	std::unique_ptr<PipelineConstantTensor<float>> weights_constant_tensor;
	std::unique_ptr<PipelineConstantTensor<float>> bias_constant_tensor;

	// Common descriptor pool which can allocate descriptors for tensors and images.
	// We're only allocating a small number of descriptors of a few types, so this simple approach works quite well.
	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

	std::unique_ptr<DataGraphPipelineLayout>  data_graph_pipeline_layout;
	std::unique_ptr<DataGraphPipeline>        data_graph_pipeline;
	std::unique_ptr<DataGraphPipelineSession> data_graph_pipeline_session;
	VkDescriptorSet                           data_graph_pipeline_descriptor_set = VK_NULL_HANDLE;

	std::unique_ptr<ComputePipelineLayoutWithTensors> preprocessing_pipeline_layout;
	std::unique_ptr<ComputePipelineWithTensors>       preprocessing_pipeline;
	VkDescriptorSet                                   preprocessing_pipeline_descriptor_set = VK_NULL_HANDLE;

	std::unique_ptr<ComputePipelineLayoutWithTensors> postprocessing_pipeline_layout;
	std::unique_ptr<ComputePipelineWithTensors>       postprocessing_pipeline;
	VkDescriptorSet                                   postprocessing_pipeline_descriptor_set = VK_NULL_HANDLE;

	std::unique_ptr<ComputePipelineLayoutWithTensors> visualization_pipeline_layout;
	std::unique_ptr<ComputePipelineWithTensors>       visualization_pipeline;
	VkDescriptorSet                                   visualization_pipeline_descriptor_set = VK_NULL_HANDLE;

	// Used for gradual increase in input shader. Initial time is set in constructor.
	std::chrono::time_point<std::chrono::system_clock> time;
};

std::unique_ptr<ComputeShadersWithTensors> create_compute_shaders_with_tensors();
