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

#pragma once

#include "vulkan_sample.h"

#include "../tensor_and_data_graph_common.h"

#include <memory>
#include <vector>

/**
 * @brief Demonstrates how to use the VK_ARM_tensors and VK_ARM_data_graph extensions in a simple example
 *        which runs a trivial neural network on a small, hardcoded tensor.
 * @details A `Tensor` resource (`input_tensor`) is created and initial contents are uploaded. This is a new resource type analogous
 *          to Images and Buffers. This is read as an input by a `Data Graph Pipeline` (`data_graph_pipeline`), which is a new type of pipeline analogous
 *			to Graphics Pipelines and Compute Pipelines. The Data Graph Pipeline runs a neural network and produces an output which is written
 *          into the `output_tensor` Tensor.
 *          In order to visualize the results of this pipeline, there is a Compute Pipeline (`visualization_pipeline`) which copies the
 *          contents of `input_tensor` and `output_tensor` into an Image (`output_image`) which is blitted to the Swapchain.
 *
 *			As a diagram, this looks like:
 *
 *					input_tensor -> data_graph_pipeline -> output_tensor
 *                         \                                   \.
 *                          \-----------------------------------\--------> visualization_pipeline -> output_image -> blit -> swapchain
 *
 *		    Because the common Vulkan Samples framework code is not aware of the Tensor resource type or Data Graph Pipelines,
 *			generic functionality for these concepts has been added to a new tensor_and_data_graph_common.h/cpp file, which this sample
 *			(and other tensor and data graph samples) makes use of.
 */
class SimpleTensorAndDataGraph : public vkb::VulkanSampleC
{
  public:
	SimpleTensorAndDataGraph();
	~SimpleTensorAndDataGraph() override;

	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;

	bool prepare(const vkb::ApplicationOptions &options) override;

	bool resize(uint32_t width, uint32_t height) override;

	void draw_renderpass(vkb::core::CommandBufferC &command_buffer, RenderTargetType &render_target) override;

	void draw_gui() override;

  private:
	void prepare_descriptor_pool();

	void prepare_input_tensor();
	void prepare_output_tensor();
	void prepare_output_image(uint32_t width, uint32_t height);

	void prepare_data_graph_pipeline();
	void prepare_data_graph_pipeline_descriptor_set();

	void prepare_visualization_pipeline();
	void prepare_visualization_pipeline_descriptor_set();

	std::unique_ptr<Tensor>     input_tensor;
	std::unique_ptr<TensorView> input_tensor_view;

	std::unique_ptr<Tensor>     output_tensor;
	std::unique_ptr<TensorView> output_tensor_view;

	std::unique_ptr<vkb::core::Image>     output_image;
	std::unique_ptr<vkb::core::ImageView> output_image_view;

	// Common descriptor pool which can allocate descriptors for tensors and images.
	// We're only allocating a small number of descriptors of a few types, so this simple approach works quite well.
	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

	std::unique_ptr<DataGraphPipelineLayout>  data_graph_pipeline_layout;
	std::unique_ptr<DataGraphPipeline>        data_graph_pipeline;
	std::unique_ptr<DataGraphPipelineSession> data_graph_pipeline_session;
	VkDescriptorSet                           data_graph_pipeline_descriptor_set = VK_NULL_HANDLE;

	std::unique_ptr<ComputePipelineLayoutWithTensors> visualization_pipeline_layout;
	std::unique_ptr<ComputePipelineWithTensors>       visualization_pipeline;
	VkDescriptorSet                                   visualization_pipeline_descriptor_set = VK_NULL_HANDLE;
};

std::unique_ptr<SimpleTensorAndDataGraph> create_simple_tensor_and_data_graph();
