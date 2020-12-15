/* Copyright (c) 2020, Arm Limited and Contributors
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

#include "buffer_device_address.h"

BufferDeviceAddress::BufferDeviceAddress()
{
	title = "Buffer device address";

	// Need to enable buffer device address extension.
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

	// Provides support for VkAllocateMemoryFlagsInfo. Otherwise, core in Vulkan 1.1.
	add_device_extension(VK_KHR_DEVICE_GROUP_EXTENSION_NAME);
	// Required by VK_KHR_device_group.
	add_instance_extension(VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME);
}

BufferDeviceAddress::~BufferDeviceAddress()
{
	if (device)
	{
		VkDevice vk_device = get_device().get_handle();
		vkDestroyPipelineLayout(vk_device, pipelines.compute_pipeline_layout, nullptr);
		vkDestroyPipelineLayout(vk_device, pipelines.graphics_pipeline_layout, nullptr);
		vkDestroyPipeline(vk_device, pipelines.bindless_vbo_pipeline, nullptr);
		vkDestroyPipeline(vk_device, pipelines.compute_update_pipeline, nullptr);

		for (auto &buffer : test_buffers)
		{
			vkDestroyBuffer(vk_device, buffer.buffer, nullptr);
			vkFreeMemory(vk_device, buffer.memory, nullptr);
		}
		vkDestroyBuffer(vk_device, pointer_buffer.buffer, nullptr);
		vkFreeMemory(vk_device, pointer_buffer.memory, nullptr);
	}
}

void BufferDeviceAddress::build_command_buffers()
{
}

void BufferDeviceAddress::on_update_ui_overlay(vkb::Drawer &)
{
}

bool BufferDeviceAddress::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	create_vbo_buffers();
	index_buffer = create_index_buffer();
	create_pipelines();

	return true;
}

struct PushCompute
{
	// This type is 8 bytes, and maps to a buffer_reference in Vulkan GLSL.
	VkDeviceAddress table;
	float           fract_time;
};

struct PushVertex
{
	glm::mat4       view_projection;
	VkDeviceAddress table;
};

VkPipelineLayout BufferDeviceAddress::create_pipeline_layout(bool graphics)
{
	// For simplicity, we avoid any use of descriptor sets here.
	// We can just push a single pointer instead, which references all the buffers we need to work with.
	VkPipelineLayout layout{};

	VkPipelineLayoutCreateInfo layout_create_info = vkb::initializers::pipeline_layout_create_info(nullptr, 0);

	const std::vector<VkPushConstantRange> ranges = {
	    vkb::initializers::push_constant_range(graphics ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_COMPUTE_BIT,
	                                           graphics ? sizeof(PushVertex) : sizeof(PushCompute), 0),
	};
	layout_create_info.pushConstantRangeCount = uint32_t(ranges.size());
	layout_create_info.pPushConstantRanges    = ranges.data();
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_create_info, nullptr, &layout));
	return layout;
}

void BufferDeviceAddress::create_compute_pipeline()
{
	pipelines.compute_pipeline_layout = create_pipeline_layout(false);
	VkComputePipelineCreateInfo info  = vkb::initializers::compute_pipeline_create_info(pipelines.compute_pipeline_layout);
	info.stage                        = load_shader("buffer_device_address/update_vbo.comp", VK_SHADER_STAGE_COMPUTE_BIT);
	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &pipelines.compute_update_pipeline));
}

void BufferDeviceAddress::create_graphics_pipeline()
{
	pipelines.graphics_pipeline_layout = create_pipeline_layout(true);
	VkGraphicsPipelineCreateInfo info  = vkb::initializers::pipeline_create_info(pipelines.graphics_pipeline_layout, render_pass);

	// No VBOs, everything is fetched from buffer device addresses.
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();

	// Going to render a simple quad mesh here with index buffer strip and primitive restart,
	// otherwise nothing interesting here.
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0, VK_TRUE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_GREATER);
	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState>      dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables);

	info.pVertexInputState   = &vertex_input_state;
	info.pInputAssemblyState = &input_assembly_state;
	info.pRasterizationState = &rasterization_state;
	info.pColorBlendState    = &color_blend_state;
	info.pDepthStencilState  = &depth_stencil_state;
	info.pViewportState      = &viewport_state;
	info.pMultisampleState   = &multisample_state;
	info.pDynamicState       = &dynamic_state;

	VkPipelineShaderStageCreateInfo stages[2];
	info.pStages    = stages;
	info.stageCount = 2;

	stages[0] = load_shader("buffer_device_address/render.vert", VK_SHADER_STAGE_VERTEX_BIT);
	stages[1] = load_shader("buffer_device_address/render.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &pipelines.bindless_vbo_pipeline));
}

void BufferDeviceAddress::create_pipelines()
{
	create_compute_pipeline();
	create_graphics_pipeline();
}

// A straight forward way of creating a "tessellated" quad mesh.
// Choose a low resolution per mesh so it's more visible in the vertex shader what is happening.
static constexpr unsigned mesh_width             = 16;
static constexpr unsigned mesh_height            = 16;
static constexpr unsigned mesh_strips            = mesh_height - 1;
static constexpr unsigned mesh_indices_per_strip = 2 * mesh_width;
static constexpr unsigned mesh_num_indices       = mesh_strips * (mesh_indices_per_strip + 1);        // Add one index to handle primitive restart.

std::unique_ptr<vkb::core::Buffer> BufferDeviceAddress::create_index_buffer()
{
	constexpr size_t size = mesh_num_indices * sizeof(uint16_t);

	// Build a simple subdivided quad mesh. We can tweak the vertices later in compute to create a simple cloth-y/wave-like effect.

	auto index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                        size,
	                                                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                        VMA_MEMORY_USAGE_GPU_ONLY);

	auto staging_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                          size,
	                                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	                                                          VMA_MEMORY_USAGE_CPU_TO_GPU);

	auto *buffer = reinterpret_cast<uint16_t *>(staging_buffer->map());
	for (unsigned strip = 0; strip < mesh_strips; strip++)
	{
		for (unsigned x = 0; x < mesh_width; x++)
		{
			*buffer++ = strip * mesh_width + x;
			*buffer++ = (strip + 1) * mesh_width + x;
		}
		*buffer++ = 0xffff;
	}
	staging_buffer->unmap();

	auto &cmd = get_device().request_command_buffer();
	cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	cmd.copy_buffer(*staging_buffer, *index_buffer, size);

	vkb::BufferMemoryBarrier memory_barrier;
	memory_barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
	memory_barrier.dst_access_mask = VK_ACCESS_INDEX_READ_BIT;
	memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
	memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
	cmd.buffer_memory_barrier(*index_buffer, 0, VK_WHOLE_SIZE, memory_barrier);
	VK_CHECK(cmd.end());

	// Not very optimal, but it's the simplest solution.
	get_device().get_suitable_graphics_queue().submit(cmd, VK_NULL_HANDLE);
	get_device().get_suitable_graphics_queue().wait_idle();
	return index_buffer;
}

void BufferDeviceAddress::create_vbo_buffers()
{
	test_buffers.resize(64);
	for (auto &buffer : test_buffers)
	{
		buffer = create_vbo_buffer();
	}

	pointer_buffer = create_pointer_buffer();
}

BufferDeviceAddress::TestBuffer BufferDeviceAddress::create_vbo_buffer()
{
	TestBuffer buffer;

	// Here we represent each "meshlet" as its own buffer to demonstrate maximum allocation flexibility.
	VkDevice         device    = get_device().get_handle();
	constexpr size_t mesh_size = mesh_width * mesh_height * sizeof(glm::vec2);

	// To be able to query the buffer device address, we must use the SHADER_DEVICE_ADDRESS_BIT usage flag.
	// STORAGE_BUFFER is also required.
	VkBufferCreateInfo create_info = vkb::initializers::buffer_create_info(
	    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, mesh_size);

	VK_CHECK(vkCreateBuffer(device, &create_info, nullptr, &buffer.buffer));

	VkMemoryAllocateInfo memory_allocation_info = vkb::initializers::memory_allocate_info();
	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, buffer.buffer, &memory_requirements);

	// Another change is that the memory we allocate must be marked as buffer device address capable.
	VkMemoryAllocateFlagsInfoKHR flags_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR};
	flags_info.flags             = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
	memory_allocation_info.pNext = &flags_info;

	memory_allocation_info.allocationSize  = memory_requirements.size;
	memory_allocation_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocation_info, nullptr, &buffer.memory));
	VK_CHECK(vkBindBufferMemory(get_device().get_handle(), buffer.buffer, buffer.memory, 0));

	// Once we've bound the buffer, we query the buffer device address.
	// We can now place this address (or any offset of said address) into a buffer and access data as a raw pointer in shaders.
	VkBufferDeviceAddressInfoKHR address_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR};
	address_info.buffer = buffer.buffer;
	buffer.gpu_address  = vkGetBufferDeviceAddressKHR(device, &address_info);

	// The buffer content will be computed at runtime, so don't upload anything.

	return buffer;
}

BufferDeviceAddress::TestBuffer BufferDeviceAddress::create_pointer_buffer()
{
	// Just like create_vbo_buffer(), we create a buffer which holds other pointers.
	TestBuffer buffer;

	VkDevice device      = get_device().get_handle();
	size_t   buffer_size = test_buffers.size() * sizeof(VkDeviceAddress);

	// We use TRANSFER_DST since we will upload to the buffer later.
	VkBufferCreateInfo create_info = vkb::initializers::buffer_create_info(
	    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, buffer_size);
	VK_CHECK(vkCreateBuffer(device, &create_info, nullptr, &buffer.buffer));

	VkMemoryAllocateInfo         memory_allocation_info = vkb::initializers::memory_allocate_info();
	VkMemoryAllocateFlagsInfoKHR flags_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR};
	VkMemoryRequirements         memory_requirements;
	vkGetBufferMemoryRequirements(device, buffer.buffer, &memory_requirements);

	flags_info.flags             = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
	memory_allocation_info.pNext = &flags_info;

	memory_allocation_info.allocationSize  = memory_requirements.size;
	memory_allocation_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocation_info, nullptr, &buffer.memory));
	VK_CHECK(vkBindBufferMemory(get_device().get_handle(), buffer.buffer, buffer.memory, 0));

	VkBufferDeviceAddressInfoKHR address_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR};
	address_info.buffer = buffer.buffer;
	buffer.gpu_address  = vkGetBufferDeviceAddressKHR(device, &address_info);

	return buffer;
}

void BufferDeviceAddress::update_pointer_buffer(VkCommandBuffer cmd)
{
	// Wait with updating the pointer buffer until previous frame's vertex shading is complete.
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
	                     0, nullptr, 0, nullptr, 0, nullptr);

	std::vector<VkDeviceAddress> pointers;
	pointers.reserve(test_buffers.size());
	for (auto &test_buffer : test_buffers)
	{
		pointers.push_back(test_buffer.gpu_address);
	}
	// Simple approach. A proxy for a compute shader which culls meshlets.
	vkCmdUpdateBuffer(cmd, pointer_buffer.buffer, 0, test_buffers.size() * sizeof(VkDeviceAddress), pointers.data());

	VkMemoryBarrier global_memory_barrier = vkb::initializers::memory_barrier();
	global_memory_barrier.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
	global_memory_barrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
	                     1, &global_memory_barrier, 0, nullptr, 0, nullptr);
}

void BufferDeviceAddress::update_meshlets(VkCommandBuffer cmd)
{
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute_update_pipeline);

	PushCompute push_compute{};

	// Here we push a pointer to a buffer, which holds pointers to all the VBO "meshlets".
	push_compute.table = pointer_buffer.gpu_address;

	// So we can create a wave-like animation.
	push_compute.fract_time = accumulated_time;

	vkCmdPushConstants(cmd, pipelines.compute_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT,
	                   0, sizeof(push_compute), &push_compute);

	// Write-after-read hazard is implicitly handled by the earlier pointer buffer update where
	// we did VERTEX -> TRANSFER -> COMPUTE chain of barriers.

	// Update all meshlets.
	vkCmdDispatch(cmd, mesh_width / 8, mesh_height / 8, uint32_t(test_buffers.size()));

	VkMemoryBarrier global_memory_barrier = vkb::initializers::memory_barrier();
	global_memory_barrier.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
	global_memory_barrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
	                     0, 1, &global_memory_barrier, 0, nullptr, 0, nullptr);
}

void BufferDeviceAddress::render(float delta_time)
{
	ApiVulkanSample::prepare_frame();

	VK_CHECK(vkWaitForFences(get_device().get_handle(), 1, &wait_fences[current_buffer], VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(get_device().get_handle(), 1, &wait_fences[current_buffer]));

	VkViewport viewport = {0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f};
	VkRect2D   scissor  = {{0, 0}, {width, height}};

	auto cmd         = draw_cmd_buffers[current_buffer];
	auto begin_info  = vkb::initializers::command_buffer_begin_info();
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &begin_info);

	// First thing is to update the pointer buffer.
	// We could use a compute shader here if we're doing
	// GPU-driven rendering for example.
	update_pointer_buffer(cmd);

	// Arbitrary value between 0 and 1 to create some animation.
	accumulated_time += 0.2f * delta_time;
	accumulated_time = glm::fract(accumulated_time);

	// Update VBOs through buffer_device_address.
	update_meshlets(cmd);

	VkRenderPassBeginInfo render_pass_begin    = vkb::initializers::render_pass_begin_info();
	render_pass_begin.renderPass               = render_pass;
	render_pass_begin.renderArea.extent.width  = width;
	render_pass_begin.renderArea.extent.height = height;
	render_pass_begin.clearValueCount          = 2;
	VkClearValue clears[2]                     = {};
	clears[0].color.float32[0]                 = 0.2f;
	clears[0].color.float32[1]                 = 0.3f;
	clears[0].color.float32[2]                 = 0.4f;
	render_pass_begin.pClearValues             = clears;
	render_pass_begin.framebuffer              = framebuffers[current_buffer];

	vkCmdBeginRenderPass(cmd, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.bindless_vbo_pipeline);
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	PushVertex push_vertex{};

	// Create an ad-hoc perspective matrix.
	push_vertex.view_projection =
	    glm::perspective(0.5f * glm::pi<float>(), float(width) / float(height), 1.0f, 100.0f) *
	    glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Push pointer to array of meshlets.
	// Every instance renders its own meshlet.
	push_vertex.table = pointer_buffer.gpu_address;
	vkCmdPushConstants(cmd, pipelines.graphics_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_vertex), &push_vertex);

	vkCmdBindIndexBuffer(cmd, index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(cmd, mesh_num_indices, uint32_t(test_buffers.size()), 0, 0, 0);

	draw_ui(cmd);

	vkCmdEndRenderPass(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, wait_fences[current_buffer]));
	ApiVulkanSample::submit_frame();
}

void BufferDeviceAddress::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Need to enable the bufferDeviceAddress feature.
	auto &features = gpu.request_extension_features<VkPhysicalDeviceBufferDeviceAddressFeaturesKHR>(
	    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR);
	features.bufferDeviceAddress = VK_TRUE;
}

std::unique_ptr<vkb::VulkanSample> create_buffer_device_address()
{
	return std::make_unique<BufferDeviceAddress>();
}
