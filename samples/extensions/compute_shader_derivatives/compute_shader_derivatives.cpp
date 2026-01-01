/* Copyright (c) 2025, Holochip Inc.
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

#include "compute_shader_derivatives.h"

#include <cstdio>

#include "common/vk_common.h"
#include "common/vk_initializers.h"
#include "core/util/logging.hpp"

ComputeShaderDerivatives::ComputeShaderDerivatives()
{
	title = "Compute shader derivatives (VK_KHR_compute_shader_derivatives)";

	// Use Vulkan 1.2 instance so SPIR-V 1.4 modules produced by Slang are valid under validation
	set_api_version(VK_API_VERSION_1_2);

	// Needed for feature chaining
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	// Device extension providing the feature
	add_device_extension(VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME);
	// Toolchains may still emit SPV_NV_compute_shader_derivatives; enable NV extension if available to satisfy validation
	add_device_extension(VK_NV_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME, /*optional*/ true);
	// Shader draw parameters (required for SV_VertexID in Slang-generated vertex shader SPIR-V)
	add_device_extension(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME, /*optional*/ true);
}

ComputeShaderDerivatives::~ComputeShaderDerivatives()
{
	if (has_device())
	{
		VkDevice device = get_device().get_handle();

		// Compute pipeline resources
		if (compute_pipeline)
		{
			vkDestroyPipeline(device, compute_pipeline, nullptr);
		}
		if (compute_pipeline_layout)
		{
			vkDestroyPipelineLayout(device, compute_pipeline_layout, nullptr);
		}
		if (compute_descriptor_pool)
		{
			vkDestroyDescriptorPool(device, compute_descriptor_pool, nullptr);
		}
		if (compute_descriptor_set_layout)
		{
			vkDestroyDescriptorSetLayout(device, compute_descriptor_set_layout, nullptr);
		}

		// Graphics pipeline resources
		if (graphics_pipeline)
		{
			vkDestroyPipeline(device, graphics_pipeline, nullptr);
		}
		if (graphics_pipeline_layout)
		{
			vkDestroyPipelineLayout(device, graphics_pipeline_layout, nullptr);
		}
		if (graphics_descriptor_pool)
		{
			vkDestroyDescriptorPool(device, graphics_descriptor_pool, nullptr);
		}
		if (graphics_descriptor_set_layout)
		{
			vkDestroyDescriptorSetLayout(device, graphics_descriptor_set_layout, nullptr);
		}

		// Storage image resources
		if (storage_image_sampler)
		{
			vkDestroySampler(device, storage_image_sampler, nullptr);
		}
		if (storage_image_view)
		{
			vkDestroyImageView(device, storage_image_view, nullptr);
		}
		if (storage_image)
		{
			vkDestroyImage(device, storage_image, nullptr);
		}
		if (storage_image_memory)
		{
			vkFreeMemory(device, storage_image_memory, nullptr);
		}
	}
}

void ComputeShaderDerivatives::create_storage_image()
{
	auto device = get_device().get_handle();

	// Create storage image for compute shader output
	VkImageCreateInfo image_ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	image_ci.imageType     = VK_IMAGE_TYPE_2D;
	image_ci.format        = VK_FORMAT_R8G8B8A8_UNORM;
	image_ci.extent.width  = image_width;
	image_ci.extent.height = image_height;
	image_ci.extent.depth  = 1;
	image_ci.mipLevels     = 1;
	image_ci.arrayLayers   = 1;
	image_ci.samples       = VK_SAMPLE_COUNT_1_BIT;
	image_ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
	image_ci.usage         = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK(vkCreateImage(device, &image_ci, nullptr, &storage_image));

	VkMemoryRequirements mem_req{};
	vkGetImageMemoryRequirements(device, storage_image, &mem_req);

	VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	alloc_info.allocationSize  = mem_req.size;
	alloc_info.memoryTypeIndex = get_device().get_gpu().get_memory_type(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &storage_image_memory));
	VK_CHECK(vkBindImageMemory(device, storage_image, storage_image_memory, 0));

	// Create image view
	VkImageViewCreateInfo view_ci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	view_ci.image                           = storage_image;
	view_ci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	view_ci.format                          = VK_FORMAT_R8G8B8A8_UNORM;
	view_ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	view_ci.subresourceRange.baseMipLevel   = 0;
	view_ci.subresourceRange.levelCount     = 1;
	view_ci.subresourceRange.baseArrayLayer = 0;
	view_ci.subresourceRange.layerCount     = 1;
	VK_CHECK(vkCreateImageView(device, &view_ci, nullptr, &storage_image_view));

	// Create sampler for graphics pipeline
	VkSamplerCreateInfo sampler_ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	sampler_ci.magFilter    = VK_FILTER_LINEAR;
	sampler_ci.minFilter    = VK_FILTER_LINEAR;
	sampler_ci.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VK_CHECK(vkCreateSampler(device, &sampler_ci, nullptr, &storage_image_sampler));
}

void ComputeShaderDerivatives::create_output_buffer_and_descriptors()
{
	auto device = get_device().get_handle();

	// Create descriptor pool for compute: 1 storage image only
	VkDescriptorPoolSize pool_size{};
	pool_size.type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	pool_size.descriptorCount = 1;

	VkDescriptorPoolCreateInfo pool_ci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	pool_ci.maxSets       = 1;
	pool_ci.poolSizeCount = 1;
	pool_ci.pPoolSizes    = &pool_size;
	VK_CHECK(vkCreateDescriptorPool(device, &pool_ci, nullptr, &compute_descriptor_pool));
}

void ComputeShaderDerivatives::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	// Require quads derivative group (the sample shader uses layout(derivative_group_quadsNV/derivative_group_quads_khr))
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR, computeDerivativeGroupQuads);
	// Users may switch to the linear mode by changing the shader qualifier

	// Storage image read/write without format (required for storage images without explicit format qualifiers)
	gpu.get_mutable_requested_features().shaderStorageImageReadWithoutFormat  = VK_TRUE;
	gpu.get_mutable_requested_features().shaderStorageImageWriteWithoutFormat = VK_TRUE;
}

void ComputeShaderDerivatives::create_compute_pipeline()
{
	auto device = get_device().get_handle();

	// Descriptor set layout: binding 0 = storage image
	VkDescriptorSetLayoutBinding binding{};
	binding.binding            = 0;
	binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	binding.descriptorCount    = 1;
	binding.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
	binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo set_layout_ci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	set_layout_ci.bindingCount = 1;
	set_layout_ci.pBindings    = &binding;
	VK_CHECK(vkCreateDescriptorSetLayout(device, &set_layout_ci, nullptr, &compute_descriptor_set_layout));

	// Pipeline layout uses the descriptor set layout at set 0
	VkPipelineLayoutCreateInfo layout_ci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	layout_ci.setLayoutCount = 1;
	layout_ci.pSetLayouts    = &compute_descriptor_set_layout;
	VK_CHECK(vkCreatePipelineLayout(device, &layout_ci, nullptr, &compute_pipeline_layout));

	// Allocate descriptor set
	VkDescriptorSetAllocateInfo alloc_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	alloc_info.descriptorPool     = compute_descriptor_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts        = &compute_descriptor_set_layout;
	VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, &compute_descriptor_set));

	// Update descriptor: storage image only
	VkDescriptorImageInfo image_info{};
	image_info.imageView   = storage_image_view;
	image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkWriteDescriptorSet write{};
	write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext            = nullptr;
	write.dstSet           = compute_descriptor_set;
	write.dstBinding       = 0;
	write.dstArrayElement  = 0;
	write.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	write.descriptorCount  = 1;
	write.pImageInfo       = &image_info;
	write.pBufferInfo      = nullptr;
	write.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

	// Load compute shader
	VkPipelineShaderStageCreateInfo stage = load_shader("compute_shader_derivatives/slang/derivatives.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

	VkComputePipelineCreateInfo compute_ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
	compute_ci.stage  = stage;
	compute_ci.layout = compute_pipeline_layout;
	VK_CHECK(vkCreateComputePipelines(device, pipeline_cache, 1, &compute_ci, nullptr, &compute_pipeline));
}

void ComputeShaderDerivatives::create_graphics_pipeline()
{
	auto device = get_device().get_handle();

	// Create descriptor pool for graphics: 1 combined image sampler
	VkDescriptorPoolSize pool_size{};
	pool_size.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_size.descriptorCount = 1;

	VkDescriptorPoolCreateInfo pool_ci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	pool_ci.maxSets       = 1;
	pool_ci.poolSizeCount = 1;
	pool_ci.pPoolSizes    = &pool_size;
	VK_CHECK(vkCreateDescriptorPool(device, &pool_ci, nullptr, &graphics_descriptor_pool));

	// Descriptor set layout: binding 0 = combined image sampler
	VkDescriptorSetLayoutBinding binding{};
	binding.binding            = 0;
	binding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount    = 1;
	binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
	binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo set_layout_ci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	set_layout_ci.bindingCount = 1;
	set_layout_ci.pBindings    = &binding;
	VK_CHECK(vkCreateDescriptorSetLayout(device, &set_layout_ci, nullptr, &graphics_descriptor_set_layout));

	// Pipeline layout
	VkPipelineLayoutCreateInfo layout_ci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	layout_ci.setLayoutCount = 1;
	layout_ci.pSetLayouts    = &graphics_descriptor_set_layout;
	VK_CHECK(vkCreatePipelineLayout(device, &layout_ci, nullptr, &graphics_pipeline_layout));

	// Allocate descriptor set
	VkDescriptorSetAllocateInfo alloc_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	alloc_info.descriptorPool     = graphics_descriptor_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts        = &graphics_descriptor_set_layout;
	VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, &graphics_descriptor_set));

	// Update descriptor set with storage image sampler
	VkDescriptorImageInfo image_info{};
	image_info.sampler     = storage_image_sampler;
	image_info.imageView   = storage_image_view;
	image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
	write.dstSet          = graphics_descriptor_set;
	write.dstBinding      = 0;
	write.dstArrayElement = 0;
	write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = 1;
	write.pImageInfo      = &image_info;
	vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

	// Load shaders for fullscreen quad
	VkPipelineShaderStageCreateInfo shader_stages[2];
	shader_stages[0] = load_shader("compute_shader_derivatives/slang/fullscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("compute_shader_derivatives/slang/fullscreen.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Vertex input state: no vertex buffers (fullscreen triangle generated in vertex shader)
	VkPipelineVertexInputStateCreateInfo vertex_input_ci{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly_ci{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Viewport and scissor (dynamic)
	VkPipelineViewportStateCreateInfo viewport_ci{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewport_ci.viewportCount = 1;
	viewport_ci.scissorCount  = 1;

	// Rasterization
	VkPipelineRasterizationStateCreateInfo rasterization_ci{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	rasterization_ci.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_ci.cullMode    = VK_CULL_MODE_NONE;
	rasterization_ci.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization_ci.lineWidth   = 1.0f;

	// Multisample
	VkPipelineMultisampleStateCreateInfo multisample_ci{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	multisample_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Depth stencil
	VkPipelineDepthStencilStateCreateInfo depth_stencil_ci{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

	// Color blend
	VkPipelineColorBlendAttachmentState blend_attachment{};
	blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo color_blend_ci{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	color_blend_ci.attachmentCount = 1;
	color_blend_ci.pAttachments    = &blend_attachment;

	// Dynamic state
	VkDynamicState                   dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_ci{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	dynamic_ci.dynamicStateCount = 2;
	dynamic_ci.pDynamicStates    = dynamic_states;

	// Create graphics pipeline
	VkGraphicsPipelineCreateInfo pipeline_ci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	pipeline_ci.stageCount          = 2;
	pipeline_ci.pStages             = shader_stages;
	pipeline_ci.pVertexInputState   = &vertex_input_ci;
	pipeline_ci.pInputAssemblyState = &input_assembly_ci;
	pipeline_ci.pViewportState      = &viewport_ci;
	pipeline_ci.pRasterizationState = &rasterization_ci;
	pipeline_ci.pMultisampleState   = &multisample_ci;
	pipeline_ci.pDepthStencilState  = &depth_stencil_ci;
	pipeline_ci.pColorBlendState    = &color_blend_ci;
	pipeline_ci.pDynamicState       = &dynamic_ci;
	pipeline_ci.layout              = graphics_pipeline_layout;
	pipeline_ci.renderPass          = render_pass;
	pipeline_ci.subpass             = 0;

	VK_CHECK(vkCreateGraphicsPipelines(device, pipeline_cache, 1, &pipeline_ci, nullptr, &graphics_pipeline));
}

bool ComputeShaderDerivatives::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	// Create resources in order: image, buffer, then pipelines
	create_storage_image();
	create_output_buffer_and_descriptors();
	create_compute_pipeline();
	create_graphics_pipeline();

	prepared = true;
	return true;
}

void ComputeShaderDerivatives::build_command_buffers()
{
	// Not used; this sample records per-frame in render()
}

void ComputeShaderDerivatives::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}

	// Acquire swapchain image and signal acquired_image_ready
	prepare_frame();

	// Recreate and record the current frame's command buffer
	recreate_current_command_buffer();
	VkCommandBuffer cmd = draw_cmd_buffers[current_buffer];

	VkCommandBufferBeginInfo begin_info = vkb::initializers::command_buffer_begin_info();
	VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

	// Transition storage image to GENERAL layout for compute shader write
	VkImageMemoryBarrier image_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	image_barrier.srcAccessMask               = 0;
	image_barrier.dstAccessMask               = VK_ACCESS_SHADER_WRITE_BIT;
	image_barrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
	image_barrier.newLayout                   = VK_IMAGE_LAYOUT_GENERAL;
	image_barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
	image_barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
	image_barrier.image                       = storage_image;
	image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_barrier.subresourceRange.levelCount = 1;
	image_barrier.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	                     0, 0, nullptr, 0, nullptr, 1, &image_barrier);

	// Dispatch compute shader: 512x512 image with 8x8 local size = 64x64 workgroups
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_layout, 0, 1, &compute_descriptor_set, 0, nullptr);
	vkCmdDispatch(cmd, image_width / 8, image_height / 8, 1);

	// Barrier: compute write → fragment shader read
	VkImageMemoryBarrier compute_to_frag_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	compute_to_frag_barrier.srcAccessMask               = VK_ACCESS_SHADER_WRITE_BIT;
	compute_to_frag_barrier.dstAccessMask               = VK_ACCESS_SHADER_READ_BIT;
	compute_to_frag_barrier.oldLayout                   = VK_IMAGE_LAYOUT_GENERAL;
	compute_to_frag_barrier.newLayout                   = VK_IMAGE_LAYOUT_GENERAL;
	compute_to_frag_barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
	compute_to_frag_barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
	compute_to_frag_barrier.image                       = storage_image;
	compute_to_frag_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	compute_to_frag_barrier.subresourceRange.levelCount = 1;
	compute_to_frag_barrier.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, nullptr, 0, nullptr, 1, &compute_to_frag_barrier);

	// Begin render pass to display the computed image and GUI
	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};        // Clear to black (will be covered by image)
	clear_values[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.framebuffer              = framebuffers[current_buffer];
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	// Set viewport and scissor for graphics rendering
	VkViewport viewport{};
	viewport.width    = static_cast<float>(width);
	viewport.height   = static_cast<float>(height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.extent.width  = width;
	scissor.extent.height = height;
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	// Render the computed image as a fullscreen quad
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_layout, 0, 1, &graphics_descriptor_set, 0, nullptr);
	vkCmdDraw(cmd, 3, 1, 0, 0);        // Draw fullscreen triangle (3 vertices)

	// Draw the GUI overlay on top
	draw_ui(cmd);

	vkCmdEndRenderPass(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	// Submit: wait on acquire semaphore, signal render_complete for present
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo         submit_info{};
	submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount   = 1;
	submit_info.pWaitSemaphores      = &semaphores.acquired_image_ready;
	submit_info.pWaitDstStageMask    = &wait_stage;
	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &cmd;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = &semaphores.render_complete;

	VkQueue queue = get_device().get_queue_by_present(0).get_handle();
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	// Present (waits on render_complete)
	submit_frame();
}

void ComputeShaderDerivatives::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Compute Shader Derivatives"))
	{
		drawer.text("Visualization:");
		drawer.text("- Blue: Base procedural radial pattern");
		drawer.text("- Red/Yellow: Edges (high gradient magnitude)");
		drawer.text("- Gradient magnitude = sqrt(dx^2 + dy^2)");
		drawer.text("");

		drawer.text("This demonstrates edge detection using compute shader");
		drawer.text("derivatives, useful for LOD selection, filtering, and");
		drawer.text("spatial analysis in compute pipelines.");
	}
}

std::unique_ptr<vkb::Application> create_compute_shader_derivatives()
{
	return std::make_unique<ComputeShaderDerivatives>();
}
