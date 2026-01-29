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

#include "postprocessing_with_vgf.h"

#include "common/vk_common.h"
#include "filesystem/filesystem.hpp"
#include "gltf_loader.h"
#include "gui.h"
#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/perspective_camera.h"
#include "stats/stats.h"

#include <vgf/decoder.h>

PostprocessingWithVgf::PostprocessingWithVgf()
{
	set_api_version(VK_API_VERSION_1_3);        // Required by the emulation layers

	// Declare that we need the data graph and tensor extensions
	add_device_extension("VK_ARM_tensors");
	add_device_extension("VK_ARM_data_graph");
	// These extensions are dependencies of the above, so we need to add them too.
	add_device_extension("VK_KHR_maintenance5");
	add_device_extension("VK_KHR_deferred_host_operations");
}

PostprocessingWithVgf::~PostprocessingWithVgf()
{
	if (data_graph_pipeline_descriptor_set != VK_NULL_HANDLE)
	{
		vkFreeDescriptorSets(get_device().get_handle(), descriptor_pool, 1, &data_graph_pipeline_descriptor_set);
	}
	if (descriptor_pool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, nullptr);
	}

	// Make sure resources created in the render pipeline are destroyed before the Device gets destroyed.
	// TODO: Could move this to the base VulkanSample class and upstream this patch.
	set_render_pipeline(nullptr);
}

/**
 * @brief Overridden to declare that we require some physical device features to be enabled.
 */
void PostprocessingWithVgf::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceVulkan12Features, shaderInt8);
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceVulkan13Features, synchronization2);

	// Enable the features for tensors and data graphs which we intend to use.
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceTensorFeaturesARM, tensors);
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceTensorFeaturesARM, shaderTensorAccess);
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceDataGraphFeaturesARM, dataGraph);
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceDataGraphFeaturesARM, dataGraphShaderModule);

	// Update-after-bind is required for the emulation layer
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceVulkan12Features, descriptorBindingUniformBufferUpdateAfterBind);

	// Enable Int16 and Int64, if available.
	if (gpu.get_features().shaderInt16)
	{
		gpu.get_mutable_requested_features().shaderInt16 = VK_TRUE;
	}
	else
	{
		throw std::runtime_error("Required feature VkPhysicalDeviceFeatures::shaderInt16 is not supported.");
	}

	if (gpu.get_features().shaderInt64)
	{
		gpu.get_mutable_requested_features().shaderInt64 = VK_TRUE;
	}
	else
	{
		throw std::runtime_error("Required feature VkPhysicalDeviceFeatures::shaderInt64 is not supported.");
	}
}

bool PostprocessingWithVgf::prepare(const vkb::ApplicationOptions &options)
{
	if (!VulkanSample::prepare(options))
	{
		return false;
	}

	// Workaround for emulation layer issue, remove once fixed.
	volkLoadDevice(get_device().get_handle());

	// Load a 3D to be rendered and set up a camera to view it
	load_scene("scenes/sponza/Sponza01.gltf");
	auto            &camera_node = vkb::add_free_camera(get_scene(), "main_camera", get_render_context().get_surface_extent());
	vkb::sg::Camera &camera      = camera_node.get_component<vkb::sg::Camera>();

	// Create a forward rendering pipeline to render the scene.
	vkb::ShaderSource vert_shader("base.vert.spv");
	vkb::ShaderSource frag_shader("base.frag.spv");
	auto              scene_subpass = std::make_unique<vkb::rendering::subpasses::ForwardSubpassC>(get_render_context(), std::move(vert_shader), std::move(frag_shader), get_scene(), camera);

	auto render_pipeline = std::make_unique<vkb::RenderPipeline>();
	render_pipeline->add_subpass(std::move(scene_subpass));
	render_pipeline->prepare();

	set_render_pipeline(std::move(render_pipeline));

	// Load data from VGF file.
	vgf_data = load_vgf("shaders/tensor_and_data_graph/postprocessing_with_vgf/vgf/simple_conv2d_rescale_graph.vgf");

	// Create Vulkan resources (see individual functions for details)
	// All resources are created with a size of 1280x720 which is what the VGF expects.
	prepare_scene_render_target(1280, 720);
#if !TENSOR_IMAGE_ALIASING_RENDER_TO_ALIASED_IMAGE
	prepare_input_image();
#endif
	prepare_output_image();
	prepare_input_tensor();
	prepare_output_tensor();
	prepare_descriptor_pool();
	prepare_data_graph_pipeline();
	prepare_data_graph_pipeline_descriptor_set();

	// Create a RenderPipeline to blit `output_image` to the swapchain
	blit_pipeline = std::make_unique<vkb::RenderPipeline>();
	blit_pipeline->add_subpass(std::make_unique<BlitSubpass>(get_render_context(), output_image_view.get()));
	blit_pipeline->prepare();

	// Create a GUI so that we can toggle the neural network on and off (see draw_gui() function)
	create_gui(*window, &get_stats());

	return true;
}

VgfData PostprocessingWithVgf::load_vgf(const std::string &vgf_file_path)
{
	std::vector<uint8_t> vgf_buffer = vkb::filesystem::get()->read_file_binary(vgf_file_path);

	if (vgf_buffer.size() == 0)
	{
		throw std::runtime_error("Error loading VGF file: " + vgf_file_path);
	}

	// Parse VGF header which contains details of other sections in the file.
	std::vector<uint8_t>          header_decoder_memory(mlsdk_decoder_header_decoder_mem_reqs());
	mlsdk_decoder_header_decoder *header_decoder =
	    mlsdk_decoder_create_header_decoder(vgf_buffer.data(), header_decoder_memory.data());

	if (!mlsdk_decoder_is_header_valid(header_decoder))
	{
		throw std::runtime_error("VGF header is not valid.");
	}
	if (!mlsdk_decoder_is_header_compatible(header_decoder))
	{
		throw std::runtime_error("VGF header is not compatible.");
	}

	// Create decoder objects for each section in the VGF that we care about:
	//		Module Table:
	//			Each module is either a compute shader or a data graph.
	//			The order of these is arbitrary and there is a further information in the VGF
	//			that describes how to run these.
	//		Model Resource Table:
	//			This is a list of tensor descriptions (data formats, size etc.) which is indexed
	//			into by other fields in the VGF.
	//		Model Sequence:
	//			This defines the order that the modules should be executed in as well as their inputs and outputs.
	//		Constant table:
	//			Contains the raw constant data for all constant tensors used in the model.
	mlsdk_decoder_vgf_section_info section_infos[4];
	for (mlsdk_decoder_section section_type = mlsdk_decoder_section_modules;
	     section_type <= mlsdk_decoder_section_constants;
	     section_type = mlsdk_decoder_section(section_type + 1))
	{
		mlsdk_decoder_get_header_section_info(header_decoder, section_type, &section_infos[section_type]);

		if (section_infos[section_type].offset + section_infos[section_type].size > vgf_buffer.size())
		{
			throw std::runtime_error("Corrupt VGF header (section out of bounds).");
		}
	}

	// Get the decoders
	std::vector<uint8_t> module_table_decoder_memory(mlsdk_decoder_module_table_decoder_mem_reqs());
	std::vector<uint8_t> model_resource_table_decoder_memory(mlsdk_decoder_model_resource_table_decoder_mem_reqs());
	std::vector<uint8_t> model_sequence_decoder_memory(mlsdk_decoder_model_sequence_decoder_mem_reqs());
	std::vector<uint8_t> constant_table_decoder_memory(mlsdk_decoder_constant_table_decoder_mem_reqs());

	mlsdk_decoder_module_table_decoder *module_table_decoder =
	    mlsdk_decoder_create_module_table_decoder(
	        vgf_buffer.data() + section_infos[mlsdk_decoder_section_modules].offset,
	        module_table_decoder_memory.data());

	mlsdk_decoder_model_resource_table_decoder *model_resource_table_decoder =
	    mlsdk_decoder_create_model_resource_table_decoder(
	        vgf_buffer.data() + section_infos[mlsdk_decoder_section_resources].offset,
	        model_resource_table_decoder_memory.data());

	mlsdk_decoder_model_sequence_decoder *model_sequence_decoder =
	    mlsdk_decoder_create_model_sequence_decoder(
	        vgf_buffer.data() + section_infos[mlsdk_decoder_section_model_sequence].offset,
	        model_sequence_decoder_memory.data());

	mlsdk_decoder_constant_table_decoder *constant_table_decoder =
	    mlsdk_decoder_create_constant_table_decoder(
	        vgf_buffer.data() + section_infos[mlsdk_decoder_section_constants].offset,
	        constant_table_decoder_memory.data());

	if (!module_table_decoder)
		throw std::runtime_error("Failed to create module table decoder.");
	if (!model_resource_table_decoder)
		throw std::runtime_error("Failed to create module resource table decoder.");
	if (!model_sequence_decoder)
		throw std::runtime_error("Failed to create module sequence decoder.");
	if (!constant_table_decoder)
		throw std::runtime_error("Failed to create constant table decoder.");

	size_t num_modules = mlsdk_decoder_get_module_table_num_entries(module_table_decoder);
	if (num_modules != 1)
	{
		throw std::runtime_error("Only a single module VGF is supported.");
	}

	std::vector<TensorInfo> all_tensor_infos;
	std::vector<TensorInfo> input_tensor_infos;
	std::vector<TensorInfo> output_tensor_infos;

	size_t num_resource_entries = mlsdk_decoder_get_model_resource_table_num_entries(model_resource_table_decoder);
	all_tensor_infos.reserve(num_resource_entries);

	// Get all resources TensorInfo
	for (int resource_idx = 0; resource_idx < num_resource_entries; ++resource_idx)
	{
		mlsdk_vk_format vk_format = mlsdk_decoder_get_vk_format(model_resource_table_decoder, resource_idx);

		mlsdk_decoder_tensor_dimensions dims_raw;
		mlsdk_decoder_model_resource_table_get_tensor_shape(model_resource_table_decoder, resource_idx, &dims_raw);
		std::vector<int64_t> tensor_shape(dims_raw.data, dims_raw.data + dims_raw.size);

		TensorInfo tensor_info;
		tensor_info.binding    = resource_idx;
		tensor_info.dimensions = tensor_shape;
		tensor_info.format     = static_cast<VkFormat>(vk_format);

		all_tensor_infos.push_back(tensor_info);
	}

	// Get the constants used in model.
	size_t num_model_constants = mlsdk_decoder_get_constant_table_num_entries(constant_table_decoder);

	mlsdk_decoder_constant_indexes constant_indexes;
	mlsdk_decoder_model_sequence_get_segment_constant_indexes(model_sequence_decoder, 0, &constant_indexes);

	for (uint32_t idx = 0; idx < constant_indexes.size; ++idx)
	{
		int model_constant_idx = constant_indexes.data[idx];
		if (model_constant_idx >= num_model_constants)
		{
			throw std::runtime_error("Corrupt VGF (segment constant idx out of bounds).");
		}

		uint32_t resource_index = mlsdk_decoder_constant_table_get_mrt_index(constant_table_decoder, model_constant_idx);
		if (resource_index >= num_resource_entries)
		{
			throw std::runtime_error("Corrupt VGF (constant resource idx out of bounds)");
		}

		mlsdk_decoder_constant_data constant_data;
		mlsdk_decoder_constant_table_get_data(constant_table_decoder, model_constant_idx, &constant_data);

		std::vector<int8_t> vector_data(constant_data.data, constant_data.data + constant_data.size);

		// Now that we have the constant data and tensor info, we can populate the PipelineConstantTensor.
		constant_tensors.push_back(std::make_unique<PipelineConstantTensor<int8_t>>());

		constant_tensors[idx]->dimensions = all_tensor_infos[resource_index].dimensions;
		constant_tensors[idx]->constant_data.resize(all_tensor_infos[resource_index].dimensions.size());
		constant_tensors[idx]->constant_data = std::move(vector_data);

		constant_tensors[idx]->tensor_description = {
		    VK_STRUCTURE_TYPE_TENSOR_DESCRIPTION_ARM,
		    nullptr,
		    VK_TENSOR_TILING_LINEAR_ARM,
		    all_tensor_infos[resource_index].format,
		    4,        // dimensions
		    constant_tensors[idx]->dimensions.data(),
		    nullptr,        // pStrides
		    VK_TENSOR_USAGE_DATA_GRAPH_BIT_ARM,
		};

		constant_tensors[0]->pipeline_constant = {
		    VK_STRUCTURE_TYPE_DATA_GRAPH_PIPELINE_CONSTANT_ARM,
		    &constant_tensors[idx]->tensor_description,
		    idx,                                               // Matches the unique identifier encoded in OpGraphConstantARM in the SPIR-V module
		    constant_tensors[idx]->constant_data.data()        // Host pointer to raw data
		};
	}

	// Input
	{
		uint32_t resource_index = mlsdk_decoder_binding_slot_mrt_index(model_sequence_decoder,
		                                                               mlsdk_decoder_model_sequence_get_input_binding_slot(model_sequence_decoder), 0);
		uint32_t binding_id     = mlsdk_decoder_binding_slot_binding_id(model_sequence_decoder,
		                                                                mlsdk_decoder_model_sequence_get_input_binding_slot(model_sequence_decoder), 0);

		all_tensor_infos[resource_index].binding = binding_id;
		input_tensor_infos.push_back(all_tensor_infos[resource_index]);
	}

	// Output
	{
		uint32_t resource_index = mlsdk_decoder_binding_slot_mrt_index(model_sequence_decoder,
		                                                               mlsdk_decoder_model_sequence_get_output_binding_slot(model_sequence_decoder), 0);
		uint32_t binding_id     = mlsdk_decoder_binding_slot_binding_id(model_sequence_decoder,
		                                                                mlsdk_decoder_model_sequence_get_output_binding_slot(model_sequence_decoder), 0);

		all_tensor_infos[resource_index].binding = binding_id;
		output_tensor_infos.push_back(all_tensor_infos[resource_index]);
	}

	int32_t module_index = mlsdk_decoder_model_sequence_get_segment_module_index(model_sequence_decoder, 0);

	mlsdk_decoder_spirv_code spirv_code;
	mlsdk_decoder_get_module_code(module_table_decoder, module_index, &spirv_code);
	if (!spirv_code.code || spirv_code.words == 0)
	{
		throw std::runtime_error("Missing SPIRV code for module.");
	}

	std::vector<uint32_t> code(spirv_code.code, spirv_code.code + spirv_code.words);
	const char           *entry_point = mlsdk_decoder_get_module_entry_point(module_table_decoder, 0);

	return {input_tensor_infos, output_tensor_infos, code, entry_point};
}

/**
 * Creates a RenderTarget with a single colour and depth attachment which we will render the scene into.
 * The colour attachment will be aliased as a tensor input to the neural network, so needs some special flags.
 */
void PostprocessingWithVgf::prepare_scene_render_target(uint32_t width, uint32_t height)
{
	vkb::core::Image colour_image = vkb::core::ImageBuilder(width, height)
	                                    .with_format(VK_FORMAT_R8G8B8A8_UNORM)
#if TENSOR_IMAGE_ALIASING_RENDER_TO_ALIASED_IMAGE
	                                    // Extra flags are required to allow aliasing of this image as a tensor.
	                                    .with_usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TENSOR_ALIASING_BIT_ARM)
	                                    .with_vma_flags(VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT)
#else
	                                    // No aliasing of this image - we will copy it instead
	                                    .with_usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
#endif
	                                    .with_debug_name("SceneRenderColour")
	                                    .build(get_device().get_device());

	vkb::core::Image depth_image = vkb::core::ImageBuilder(width, height)
	                                   .with_format(vkb::get_suitable_depth_format(get_device().get_gpu().get_handle()))
	                                   .with_usage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	                                   .with_debug_name("SceneRenderDepth")
	                                   .build(get_device().get_device());

	std::vector<vkb::core::Image> images;
	images.push_back(std::move(colour_image));
	images.push_back(std::move(depth_image));

	scene_render_target = std::make_unique<vkb::RenderTarget>(std::move(images));
}

#if !TENSOR_IMAGE_ALIASING_RENDER_TO_ALIASED_IMAGE

/**
 * In the case that we are using the workaround where we perform an additional copy then this
 * function creates the additional image which we will copy the rendered scene into.
 * This image will then be aliased as the tensor input to the neural network (rather than the
 * scene render target being aliased directly), and needs some special flags.
 */
void PostprocessingWithVgf::prepare_input_image()
{
	input_image = std::make_unique<vkb::core::Image>(vkb::core::ImageBuilder(scene_render_target->get_extent().width, scene_render_target->get_extent().height)
	                                                     .with_format(VK_FORMAT_R8G8B8A8_UNORM)
	                                                     // Extra flags are required to allow aliasing of this image as a tensor.
	                                                     .with_usage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TENSOR_ALIASING_BIT_ARM)
	                                                     .with_vma_flags(VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT)
	                                                     .with_debug_name("InputImage")
	                                                     .build(get_device().get_device()));
}

#endif

/**
 * Creates an image to use as the output of the neural network.
 * This will be aliased as the output tensor, so needs some special flags.
 */
void PostprocessingWithVgf::prepare_output_image()
{
	output_image      = std::make_unique<vkb::core::Image>(vkb::core::ImageBuilder(scene_render_target->get_extent().width, scene_render_target->get_extent().height)
                                                          .with_format(VK_FORMAT_R8G8B8A8_UNORM)
                                                          // Extra flags are required to allow aliasing of this image as a tensor.
                                                          .with_usage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TENSOR_ALIASING_BIT_ARM)
                                                          .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
                                                          .with_vma_flags(VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT)
                                                          .with_debug_name("OutputImage")
                                                          .build(get_device().get_device()));
	output_image_view = std::make_unique<vkb::core::ImageView>(*output_image, VK_IMAGE_VIEW_TYPE_2D);
}

/*
 * Creates the Tensor used as input to the neural network, aliasing the same memory as the colour
 * attachment which the scene is rendered into.
 * Also creates a Tensor View (analogous to an Image View).
 */
void PostprocessingWithVgf::prepare_input_tensor()
{
#if TENSOR_IMAGE_ALIASING_RENDER_TO_ALIASED_IMAGE
	const vkb::core::Image &image_to_alias = scene_render_target->get_views().at(0).get_image();
#else
	const vkb::core::Image &image_to_alias = *input_image;
#endif

	input_tensor      = std::make_unique<ExternallyAllocatedTensor>(get_device(),
                                                               TensorBuilder({vgf_data.input_tensor_infos[0].dimensions})
                                                                   .with_usage(VK_TENSOR_USAGE_DATA_GRAPH_BIT_ARM | VK_TENSOR_USAGE_IMAGE_ALIASING_BIT_ARM)
                                                                   .with_format(vgf_data.input_tensor_infos[0].format)
                                                                   .with_tiling(VK_TENSOR_TILING_OPTIMAL_ARM),
                                                               image_to_alias.get_memory(), image_to_alias.get_memory_offset());
	input_tensor_view = std::make_unique<TensorView>(*input_tensor);
}

/*
 * Creates the Tensor used as output of the neural network, aliasing the same memory as the network_output_image,
 * which will be blitted to the screen.
 * Also creates a Tensor Views (analogous to an Image View).
 */
void PostprocessingWithVgf::prepare_output_tensor()
{
	const vkb::core::Image &image_to_alias = *output_image;

	output_tensor      = std::make_unique<ExternallyAllocatedTensor>(get_device(),
                                                                TensorBuilder({vgf_data.output_tensor_infos[0].dimensions})
                                                                    .with_usage(VK_TENSOR_USAGE_DATA_GRAPH_BIT_ARM | VK_TENSOR_USAGE_IMAGE_ALIASING_BIT_ARM)
                                                                    .with_format(vgf_data.output_tensor_infos[0].format)
                                                                    .with_tiling(VK_TENSOR_TILING_OPTIMAL_ARM),
                                                                image_to_alias.get_memory(), image_to_alias.get_memory_offset());
	output_tensor_view = std::make_unique<TensorView>(*output_tensor);
}

/*
 * Creates a descriptor pool which can be used to allocate descriptors for tensor bindings.
 * Note we can't use vkb::DescriptorPool because it doesn't know about tensors.
 */
void PostprocessingWithVgf::prepare_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> descriptor_pool_sizes = {
	    {VK_DESCRIPTOR_TYPE_TENSOR_ARM, 10},        // Fairly arbitrary count
	};

	VkDescriptorPoolCreateInfo create_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	create_info.poolSizeCount = descriptor_pool_sizes.size();
	create_info.pPoolSizes    = descriptor_pool_sizes.data();
	create_info.maxSets       = 10;        // Fairly arbitrary
	create_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &create_info, nullptr, &descriptor_pool));
}

/*
 * Creates the Pipeline Layout, a Data Graph Pipeline and a Data Graph Pipeline Session used to run the neural network.
 */
void PostprocessingWithVgf::prepare_data_graph_pipeline()
{
	// Create the Pipeline Layout.
	// The neural network has its input tensor on binding 0 and its output tensor at binding 1.
	std::set<uint32_t> tensor_bindings = {0, 1};
	data_graph_pipeline_layout         = std::make_unique<DataGraphPipelineLayout>(get_device(), tensor_bindings);

	// Create a Pipeline from the layout.
	std::map<uint32_t, std::map<uint32_t, const VkTensorDescriptionARM *>> tensor_descriptions;
	// All bindings are in set 0
	tensor_descriptions[0] =
	    {
	        // Binding 0 is the input tensor
	        {0, &input_tensor->get_description()},
	        // Binding 1 is the output tensor
	        {1, &output_tensor->get_description()}};

	// Add constant tensors, which was prepared and stored earlier.
	std::vector<VkDataGraphPipelineConstantARM *> data_graph_pipeline_constants;
	for (const auto &tensor : constant_tensors)
	{
		data_graph_pipeline_constants.push_back(&tensor->pipeline_constant);
	}

	VkShaderModule shader_module = vkb::load_shader_from_vector(vgf_data.code, get_device().get_handle());

	data_graph_pipeline =
	    std::make_unique<DataGraphPipeline>(get_device(),
	                                        data_graph_pipeline_layout->get_handle(),
	                                        shader_module,
	                                        vgf_data.entry_point.c_str(),
	                                        tensor_descriptions,
	                                        data_graph_pipeline_constants);

	// Create a Pipeline Session for the Pipeline
	VmaAllocationCreateInfo alloc_create_info = {};
	data_graph_pipeline_session               = std::make_unique<DataGraphPipelineSession>(get_device(), data_graph_pipeline->get_handle(), alloc_create_info);
}

/*
 * Allocates and fills in a Descriptor Set to provide bindings to the Data Graph Pipeline.
 */
void PostprocessingWithVgf::prepare_data_graph_pipeline_descriptor_set()
{
	// Allocate descriptor set using the layout of the Data Graph Pipeline
	VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	alloc_info.descriptorPool              = descriptor_pool;
	alloc_info.descriptorSetCount          = 1;
	alloc_info.pSetLayouts                 = &data_graph_pipeline_layout->get_descriptor_set_layout();
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &data_graph_pipeline_descriptor_set));

	// Write bindings to it, telling it which tensors to use as input and output
	std::map<uint32_t, VkWriteDescriptorSetTensorARM> tensor_bindings =
	    {
	        // Binding 0 is the input tensor
	        {0, VkWriteDescriptorSetTensorARM{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_TENSOR_ARM, nullptr, 1, &input_tensor_view->get_handle()}},
	        // Binding 1 is the output tensor
	        {1, VkWriteDescriptorSetTensorARM{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_TENSOR_ARM, nullptr, 1, &output_tensor_view->get_handle()}}};
	write_descriptor_set(get_device().get_handle(), data_graph_pipeline_descriptor_set, {}, tensor_bindings);
}

void PostprocessingWithVgf::draw_renderpass(vkb::core::CommandBufferC &command_buffer, vkb::RenderTarget &render_target)
{
	if (!enable_neural_network)
	{
		// If the neural network is disabled, use the default behaviour which is to render
		// the scene directly to the default render target (the swapchain)
		vkb::VulkanSampleC::draw_renderpass(command_buffer, render_target);
		return;
	}

	// When using the neural network, render the scene into the separate render target
	uint32_t render_width  = scene_render_target->get_extent().width;
	uint32_t render_height = scene_render_target->get_extent().height;

	command_buffer.set_viewport(0, {{0.0f, 0.0f, static_cast<float>(render_width), static_cast<float>(render_height), 0.0f, 1.0f}});
	command_buffer.set_scissor(0, {{0, 0, render_width, render_height}});

	// Barriers and layout transitions to get the render target's attachments ready for rendering
	{
		const VkImageMemoryBarrier2 imageBarriers[2] = {
		    // Colour attachment
		    {
		        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		        nullptr,
#if TENSOR_IMAGE_ALIASING_RENDER_TO_ALIASED_IMAGE
		        // When rendering to an aliased tensor, the render target image would have previously been used as the input to the data graph pipeline
		        VK_PIPELINE_STAGE_2_DATA_GRAPH_BIT_ARM,        // srcStageMask
		        VK_ACCESS_2_DATA_GRAPH_READ_BIT_ARM,           // srcAccessMask
#else
		        // When rendering to a separate image, the render target image would have previously been used as a transfer source
		        VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
		        VK_ACCESS_2_TRANSFER_READ_BIT,         // srcAccessMask
#endif
		        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,                          // dstStageMask
		        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,                                 // dstAccessMask
		        VK_IMAGE_LAYOUT_UNDEFINED,                                              // oldLayout
		        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,                               // newLayout
		        0,                                                                      // srcQueueFamilyIndex
		        0,                                                                      // dstQueueFamilyIndex
		        scene_render_target->get_views().at(0).get_image().get_handle(),        // image
		        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}},
		    // Depth attachment
		    {
		        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		        nullptr,
		        // The depth attachment would have last been used in the previous frame's rendering
		        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,                                                           // srcStageMask
		        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,        // srcAccessMask
		        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,                                                            // dstStageMask
		        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,        // dstAccessMask
		        VK_IMAGE_LAYOUT_UNDEFINED,                                                                             // oldLayout
		        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,                                                              // newLayout
		        0,                                                                                                     // srcQueueFamilyIndex
		        0,                                                                                                     // dstQueueFamilyIndex
		        scene_render_target->get_views().at(1).get_image().get_handle(),
		        VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}}};

		VkDependencyInfo dependencyInfo        = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
		dependencyInfo.imageMemoryBarrierCount = 2;
		dependencyInfo.pImageMemoryBarriers    = &imageBarriers[0];
		vkCmdPipelineBarrier2(command_buffer.get_handle(), &dependencyInfo);
	}

	// Render the scene into scene_render_target
	get_render_pipeline().draw(command_buffer, *scene_render_target);
	command_buffer.end_render_pass();

#if !TENSOR_IMAGE_ALIASING_RENDER_TO_ALIASED_IMAGE
	// Barriers and layout transitions for copying the rendered scene into input_image
	// (We only do this if we are not rendering directly to the aliased tensor)
	{
		const VkImageMemoryBarrier2 imageBarriers[2] = {
		    // Source image - the color image from the scene_render_target
		    {
		        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		        nullptr,
		        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,        // srcStageMask
		        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,               // srcAccessMask
		        VK_PIPELINE_STAGE_TRANSFER_BIT,                       // dstStageMask
		        VK_ACCESS_2_TRANSFER_READ_BIT,                        // dstAccessMask
		        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,             // oldLayout
		        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                 // newLayout
		        0,                                                    // srcQueueFamilyIndex
		        0,                                                    // dstQueueFamilyIndex
		        scene_render_target->get_views().at(0).get_image().get_handle(),
		        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}},
		    // Destination image - the input_image for the neural network
		    {
		        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		        nullptr,
		        VK_PIPELINE_STAGE_2_DATA_GRAPH_BIT_ARM,        // srcStageMask
		        VK_ACCESS_2_DATA_GRAPH_READ_BIT_ARM,           // srcAccessMask
		        VK_PIPELINE_STAGE_TRANSFER_BIT,                // dstStageMask
		        VK_ACCESS_2_TRANSFER_WRITE_BIT,                // dstAccessMask
		        VK_IMAGE_LAYOUT_UNDEFINED,                     // oldLayout
		        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,          // newLayout
		        0,                                             // srcQueueFamilyIndex
		        0,                                             // dstQueueFamilyIndex
		        input_image->get_handle(),
		        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}}};

		VkDependencyInfo dependencyInfo        = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
		dependencyInfo.imageMemoryBarrierCount = 2;
		dependencyInfo.pImageMemoryBarriers    = &imageBarriers[0];
		vkCmdPipelineBarrier2(command_buffer.get_handle(), &dependencyInfo);
	}

	// Copy the rendered scene into input_image
	// (We only do this if we are not rendering directly to the aliased tensor)
	{
		VkImageCopy image_copy;
		image_copy.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		image_copy.dstOffset      = {0, 0, 0};
		image_copy.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		image_copy.extent         = {render_width, render_height, 1};
		image_copy.srcOffset      = {0, 0, 0};
		command_buffer.copy_image(scene_render_target->get_views().at(0).get_image(), *input_image, {image_copy});
	}
#endif

	// Barriers and layout transitions for network inputs and outputs to be used in data graph pipeline execution
	{
		{
			const VkImageMemoryBarrier2 imageBarriers[2] = {
#if TENSOR_IMAGE_ALIASING_RENDER_TO_ALIASED_IMAGE
				// Input tensor (which is aliased as the scene_render_target)
				{
				    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				    nullptr,
				    // Previously was rendered to as a color attachment
				    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,        // srcStageMask
				    VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,               // srcAccessMask
				    VK_PIPELINE_STAGE_2_DATA_GRAPH_BIT_ARM,               // dstStageMask
				    VK_ACCESS_2_DATA_GRAPH_READ_BIT_ARM,                  // dstAccessMask
				    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,             // oldLayout

				    // Transition to the special layout for tensor aliasing
				    VK_IMAGE_LAYOUT_TENSOR_ALIASING_ARM,        // newLayout
				    0,                                          // srcQueueFamilyIndex
				    0,                                          // dstQueueFamilyIndex
				    scene_render_target->get_views().at(0).get_image().get_handle(),
				    VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}},
#else
				// Input tensor (which is aliased as input_image)
				{
				    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				    nullptr,
				    // Previously was copied to
				    VK_PIPELINE_STAGE_TRANSFER_BIT,                // srcStageMask
				    VK_ACCESS_2_TRANSFER_WRITE_BIT,                // srcAccessMask
				    VK_PIPELINE_STAGE_2_DATA_GRAPH_BIT_ARM,        // dstStageMask
				    VK_ACCESS_2_DATA_GRAPH_READ_BIT_ARM,           // dstAccessMask
				    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,          // oldLayout

				    // Transition to the special layout for tensor aliasing
				    VK_IMAGE_LAYOUT_TENSOR_ALIASING_ARM,        // newLayout
				    0,                                          // srcQueueFamilyIndex
				    0,                                          // dstQueueFamilyIndex
				    input_image->get_handle(),
				    VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}},
#endif
				// Output tensor (which is aliased as output_image)
				{
				    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				    nullptr,
				    // Previously was read by the blit shader
				    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,         // srcStageMask
				    VK_ACCESS_2_SHADER_READ_BIT,                   // srcAccessMask
				    VK_PIPELINE_STAGE_2_DATA_GRAPH_BIT_ARM,        // dstStageMask
				    VK_ACCESS_2_DATA_GRAPH_WRITE_BIT_ARM,          // dstAccessMask
				    VK_IMAGE_LAYOUT_UNDEFINED,                     // oldLayout

				    // Transition to the special layout for tensor aliasing
				    VK_IMAGE_LAYOUT_TENSOR_ALIASING_ARM,        // newLayout
				    0,                                          // srcQueueFamilyIndex
				    0,                                          // dstQueueFamilyIndex
				    output_image->get_handle(),
				    VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}}
			};

			VkDependencyInfo dependencyInfo        = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
			dependencyInfo.imageMemoryBarrierCount = 2;
			dependencyInfo.pImageMemoryBarriers    = &imageBarriers[0];
			vkCmdPipelineBarrier2(command_buffer.get_handle(), &dependencyInfo);
		}
	}

	// Bind and run data graph pipeline.
	vkCmdBindPipeline(command_buffer.get_handle(), VK_PIPELINE_BIND_POINT_DATA_GRAPH_ARM, data_graph_pipeline->get_handle());
	vkCmdBindDescriptorSets(command_buffer.get_handle(), VK_PIPELINE_BIND_POINT_DATA_GRAPH_ARM, data_graph_pipeline_layout->get_handle(),
	                        0, 1, &data_graph_pipeline_descriptor_set, 0, nullptr);
	vkCmdDispatchDataGraphARM(command_buffer.get_handle(), data_graph_pipeline_session->get_handle(), VK_NULL_HANDLE);

	// Barrier and layout transition for output_image to be a shader input
	{
		const VkImageMemoryBarrier2 imageBarrier = {
		    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		    nullptr,
		    // Was previously written to by the data graph pipeline
		    VK_PIPELINE_STAGE_2_DATA_GRAPH_BIT_ARM,          // srcStageMask
		    VK_ACCESS_2_DATA_GRAPH_WRITE_BIT_ARM,            // srcAccessMask
		    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,           // dstStageMask
		    VK_ACCESS_2_SHADER_READ_BIT,                     // dstAccessMask
		    VK_IMAGE_LAYOUT_TENSOR_ALIASING_ARM,             // oldLayout
		    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,        // newLayout
		    0,                                               // srcQueueFamilyIndex
		    0,                                               // dstQueueFamilyIndex
		    output_image->get_handle(),
		    VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

		VkDependencyInfo dependencyInfo        = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers    = &imageBarrier;
		vkCmdPipelineBarrier2(command_buffer.get_handle(), &dependencyInfo);
	}

	// Blit output_image to the screen and draw the GUI
	uint32_t screen_width  = render_target.get_extent().width;
	uint32_t screen_height = render_target.get_extent().height;
	command_buffer.set_viewport(0, {{0.0f, 0.0f, static_cast<float>(screen_width), static_cast<float>(screen_height), 0.0f, 1.0f}});
	command_buffer.set_scissor(0, {{0, 0, screen_width, screen_height}});

	blit_pipeline->draw(command_buffer, render_target);

	get_gui().draw(command_buffer);

	command_buffer.end_render_pass();
}

void PostprocessingWithVgf::draw_gui()
{
	// Define a checkbox to toggle the neural network on and off, so that you can see the effect of the edge enhancement network.
	get_gui().show_options_window(
	    [this]() {
		    ImGui::Checkbox("Enable Neural Network", &enable_neural_network);
	    },
	    1);
}

std::unique_ptr<vkb::VulkanSampleC> create_postprocessing_with_vgf()
{
	return std::make_unique<PostprocessingWithVgf>();
}
