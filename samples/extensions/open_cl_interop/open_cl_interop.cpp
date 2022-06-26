/* Copyright (c) 2022, Sascha Willems
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

#include "open_cl_interop.h"

#include "common/vk_common.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"

#define CL_FUNCTION_DEFINITIONS
#include <open_cl_utils.h>
#include <strstream>

#ifdef VK_USE_PLATFORM_WIN32_KHR
#	include <aclapi.h>
#endif

struct CLData
{
	cl_context       context{nullptr};
	cl_device_id     device_id{nullptr};
	cl_command_queue command_queue{nullptr};
	cl_program       program{nullptr};
	cl_kernel        kernel{nullptr};
	cl_mem           image{nullptr};
};

OpenCLInterop::OpenCLInterop()
{
	zoom  = -3.5f;
	title = "Interoperability with OpenCL";

	add_device_extension(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
	add_device_extension(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
#ifdef _WIN32
	add_device_extension(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
	add_device_extension(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
#else
	add_device_extension(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
	add_device_extension(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
#endif
	add_instance_extension(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
	add_instance_extension(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
}

OpenCLInterop::~OpenCLInterop()
{
	if (device)
	{
		vkDestroyPipeline(device->get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(device->get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(device->get_handle(), descriptor_set_layout, nullptr);
		vkDestroyFence(device->get_handle(), rendering_finished_fence, nullptr);
		vkDestroySampler(device->get_handle(), shared_texture.sampler, nullptr);
		vkDestroyImageView(device->get_handle(), shared_texture.view, nullptr);
		vkDestroyImage(device->get_handle(), shared_texture.image, nullptr);
		vkFreeMemory(device->get_handle(), shared_texture.memory, nullptr);
	}

	if (cl_data)
	{
		clReleaseMemObject(cl_data->image);
		clReleaseContext(cl_data->context);
		delete cl_data;
	}

	unload_opencl();
}

bool OpenCLInterop::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	cl_data = new CLData{};

	// Fetch platform specific function pointers for external objects
#ifdef _WIN32
	vkGetMemoryWin32HandleKHR = reinterpret_cast<PFN_vkGetMemoryWin32HandleKHR>(vkGetInstanceProcAddr(instance->get_handle(), "vkGetMemoryWin32HandleKHR"));
	if (!vkGetMemoryWin32HandleKHR)
	{
		throw std::runtime_error("Could not get function pointer for \"vkGetMemoryWin32HandleKHR\"");
	}
#else
	vkGetMemoryFdKHR = reinterpret_cast<PFN_vkGetMemoryFdKHR>(vkGetInstanceProcAddr(instance->get_handle(), "vkGetMemoryFdKHR"));
	if (!vkGetMemoryFdKHR)
	{
		throw std::runtime_error("Could not get function pointer for \"vkGetMemoryFdKHR\"");
	}
#endif

	prepare_open_cl_resources();
	prepare_shared_resources();
	generate_quad();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();

	auto fence_create_info = vkb::initializers::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	vkCreateFence(device->get_handle(), &fence_create_info, nullptr, &rendering_finished_fence);

	prepared = true;
	return true;
}

void OpenCLInterop::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}

	total_time_passed += delta_time;

	// Wait until the Vulkan command buffer displaying the image has finished execution, so we can start writing to it from OpenCL
	vkWaitForFences(device->get_handle(), 1, &rendering_finished_fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(device->get_handle(), 1, &rendering_finished_fence);

	// Update the image from OpenCL
	update_texture_from_open_cl();

	// Wait until the image update is finished
	// @todo: Use semaphores instead
	clFlush(cl_data->command_queue);
	clFinish(cl_data->command_queue);

	// Display the image using Vulkan
	ApiVulkanSample::prepare_frame();

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, rendering_finished_fence));

	ApiVulkanSample::submit_frame();
}

void OpenCLInterop::view_changed()
{
	update_uniform_buffers();
}

void OpenCLInterop::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = default_clear_color;
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); i++)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer->get(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(draw_cmd_buffers[i], index_count, 1, 0, 0, 0);

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void OpenCLInterop::generate_quad()
{
	std::vector<VertexStructure> vertices =
	    {
	        {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	        {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

	std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
	index_count                   = static_cast<uint32_t>(indices.size());

	auto vertex_buffer_size = vkb::to_u32(vertices.size() * sizeof(VertexStructure));
	auto index_buffer_size  = vkb::to_u32(indices.size() * sizeof(uint32_t));

	// Create buffers
	// For the sake of simplicity we won't stage the vertex data to the gpu memory
	// Vertex buffer
	vertex_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                    vertex_buffer_size,
	                                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                   index_buffer_size,
	                                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                                   VMA_MEMORY_USAGE_CPU_TO_GPU);

	index_buffer->update(indices.data(), index_buffer_size);
}

void OpenCLInterop::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes =
	    {
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()),
	                                                                                                        pool_sizes.data(),
	                                                                                                        2);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void OpenCLInterop::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings{
	    // Binding 0 : Vertex shader uniform buffer
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_VERTEX_BIT,
	        0),
	    // Binding 1 : Fragment shader image sampler
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        VK_SHADER_STAGE_FRAGMENT_BIT,
	        1),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout =
	    vkb::initializers::descriptor_set_layout_create_info(
	        set_layout_bindings.data(),
	        vkb::to_u32(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr,
	                                     &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(
	    &descriptor_set_layout, 1);

	VK_CHECK(
	    vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr,
	                           &pipeline_layout));
}

void OpenCLInterop::setup_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniform_buffer_vs);

	// Setup a descriptor image info for the current texture to be used as a combined image sampler
	VkDescriptorImageInfo image_descriptor;
	image_descriptor.imageView   = shared_texture.view;
	image_descriptor.sampler     = shared_texture.sampler;
	image_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	std::vector<VkWriteDescriptorSet> write_descriptor_sets =
	    {
	        // Binding 0 : Vertex shader uniform buffer
	        vkb::initializers::write_descriptor_set(
	            descriptor_set,
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            0,
	            &buffer_descriptor),
	        // Binding 1 : Fragment shader texture sampler
	        //	Fragment shader: layout (binding = 1) uniform sampler2D samplerColor;
	        vkb::initializers::write_descriptor_set(
	            descriptor_set,
	            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,        // The descriptor set will use a combined image sampler (sampler and image could be split)
	            1,                                                // Shader binding point 1
	            &image_descriptor)                                // Pointer to the descriptor image for our texture
	    };

	vkUpdateDescriptorSets(get_device().get_handle(), vkb::to_u32(write_descriptor_sets.size()),
	                       write_descriptor_sets.data(), 0, nullptr);
}

void OpenCLInterop::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	        0,
	        VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_NONE,
	        VK_FRONT_FACE_COUNTER_CLOCKWISE,
	        0);

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        0xf,
	        VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

	// Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE,
	        VK_TRUE,
	        VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        vkb::to_u32(dynamic_state_enables.size()),
	        0);

	// Load shaders
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

	shader_stages[0] = load_shader("texture_loading/texture.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("texture_loading/texture.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(VertexStructure),
	                                                        VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT,
	                                                          offsetof(VertexStructure, pos)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT,
	                                                          offsetof(VertexStructure, uv)),
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32B32_SFLOAT,
	                                                          offsetof(VertexStructure,
	                                                                   normal)),
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = vkb::to_u32(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = vkb::to_u32(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(
	        pipeline_layout,
	        render_pass,
	        0);

	pipeline_create_info.pVertexInputState   = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.stageCount          = vkb::to_u32(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1,
	                                   &pipeline_create_info, nullptr, &pipeline));
}

void OpenCLInterop::prepare_uniform_buffers()
{
	// Vertex shader uniform buffer block
	uniform_buffer_vs = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                        sizeof(ubo_vs),
	                                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void OpenCLInterop::update_uniform_buffers()
{
	// Vertex shader
	ubo_vs.projection = glm::perspective(glm::radians(60.0f), (float) width / (float) height, 0.001f, 256.0f);

	glm::mat4 view_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));

	ubo_vs.model = view_matrix * glm::translate(glm::mat4(1.0f), camera_pos);
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo_vs.view_pos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);

	uniform_buffer_vs->convert_and_update(ubo_vs);
}

// @todo: document
HANDLE OpenCLInterop::get_vulkan_image_handle(VkExternalMemoryHandleTypeFlagsKHR external_memory_handle_type)
{
	HANDLE                        handle;
	VkMemoryGetWin32HandleInfoKHR win32_handle_info = {};
	win32_handle_info.sType                         = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
	win32_handle_info.memory                        = shared_texture.memory;
	win32_handle_info.handleType                    = (VkExternalMemoryHandleTypeFlagBitsKHR) external_memory_handle_type;
	vkGetMemoryWin32HandleKHR(device->get_handle(), &win32_handle_info, &handle);
	return handle;
}

void OpenCLInterop::prepare_shared_resources()
{
	// This texture will be shared between both APIs: OpenCL fills it and Vulkan uses it for rendering
	shared_texture.width  = 256;
	shared_texture.height = 256;
	shared_texture.depth  = 1;

	auto device_handle = get_device().get_handle();

	// Setting up Vulkan resources (image, memory, image view and sampler)

	VkImageCreateInfo image_create_info = vkb::initializers::image_create_info();
	image_create_info.imageType         = VK_IMAGE_TYPE_2D;
	image_create_info.format            = VK_FORMAT_R8G8B8A8_UINT;
	image_create_info.mipLevels         = 1;
	image_create_info.arrayLayers       = 1;
	image_create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.extent            = {shared_texture.width, shared_texture.height, shared_texture.depth};
	image_create_info.usage             = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	// @todo: comment
	VkExternalMemoryImageCreateInfo external_memory_image_info{};
	external_memory_image_info.sType       = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
	external_memory_image_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
	image_create_info.pNext                = &external_memory_image_info;

	VK_CHECK(vkCreateImage(get_device().get_handle(), &image_create_info, nullptr, &shared_texture.image));

	VkMemoryRequirements memory_requirements{};
	vkGetImageMemoryRequirements(device->get_handle(), shared_texture.image, &memory_requirements);

#ifdef _WIN32
	// On Windows, we need to enable some security settings to allow api interop
	// @todo: add proper explanation why this is needed
	// Spec: For handles of the following types: VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT The implementation must ensure the access rights allow read and write access to the memory.
	SECURITY_ATTRIBUTES security_attributes{};
	SECURITY_DESCRIPTOR security_descriptor{};
	PSID                sid{nullptr};
	PACL                acl{nullptr};

	InitializeSecurityDescriptor(&security_descriptor, SECURITY_DESCRIPTOR_REVISION);
	SID_IDENTIFIER_AUTHORITY sid_identifier_authority = SECURITY_WORLD_SID_AUTHORITY;
	AllocateAndInitializeSid(&sid_identifier_authority, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &sid);

	EXPLICIT_ACCESS explicit_access{};
	explicit_access.grfAccessPermissions = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
	explicit_access.grfAccessMode        = SET_ACCESS;
	explicit_access.grfInheritance       = INHERIT_ONLY;
	explicit_access.Trustee.TrusteeForm  = TRUSTEE_IS_SID;
	explicit_access.Trustee.TrusteeType  = TRUSTEE_IS_WELL_KNOWN_GROUP;
	explicit_access.Trustee.ptstrName    = (LPTSTR) &sid;
	SetEntriesInAcl(1, &explicit_access, nullptr, &acl);
	SetSecurityDescriptorDacl(&security_descriptor, true, acl, false);
	security_attributes.nLength              = sizeof(security_attributes);
	security_attributes.lpSecurityDescriptor = &security_descriptor;
	security_attributes.bInheritHandle       = true;

	VkExportMemoryWin32HandleInfoKHR export_memory_win32_handle_info{};
	export_memory_win32_handle_info.sType       = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
	export_memory_win32_handle_info.pAttributes = &security_attributes;
	// @todo: document
	export_memory_win32_handle_info.dwAccess = 0x80000000L | 1;
	//vulkanExportMemoryWin32HandleInfoKHR.dwAccess = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
#endif
	VkExportMemoryAllocateInfoKHR export_memory_allocate_info{};
	export_memory_allocate_info.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
#ifdef _WIN32
	export_memory_allocate_info.pNext       = &export_memory_win32_handle_info;
	export_memory_allocate_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
	export_memory_allocate_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
#endif

	VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
	memory_allocate_info.pNext                = &export_memory_allocate_info;
	memory_allocate_info.allocationSize       = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex      = device->get_memory_type(memory_requirements.memoryTypeBits, 0);

	VK_CHECK(vkAllocateMemory(device_handle, &memory_allocate_info, nullptr, &shared_texture.memory));
	VK_CHECK(vkBindImageMemory(device_handle, shared_texture.image, shared_texture.memory, 0));

	VkSamplerCreateInfo sampler_create_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	sampler_create_info.magFilter   = VK_FILTER_LINEAR;
	sampler_create_info.minFilter   = VK_FILTER_LINEAR;
	sampler_create_info.mipmapMode  = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_create_info.maxLod      = (float) 1;
	sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	vkCreateSampler(device_handle, &sampler_create_info, nullptr, &shared_texture.sampler);

	VkImageViewCreateInfo view_create_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	view_create_info.viewType         = VK_IMAGE_VIEW_TYPE_2D;
	view_create_info.image            = shared_texture.image;
	view_create_info.format           = VK_FORMAT_R8G8B8A8_UNORM;
	view_create_info.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	vkCreateImageView(device_handle, &view_create_info, nullptr, &shared_texture.view);

	VkCommandBuffer copy_command = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkImageSubresourceRange subresource_range = {};
	subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel            = 0;
	subresource_range.levelCount              = 1;
	subresource_range.layerCount              = 1;

	VkImageMemoryBarrier image_memory_barrier = vkb::initializers::image_memory_barrier();
	image_memory_barrier.image                = shared_texture.image;
	image_memory_barrier.subresourceRange     = subresource_range;
	image_memory_barrier.srcAccessMask        = 0;
	image_memory_barrier.dstAccessMask        = VK_ACCESS_SHADER_READ_BIT;
	image_memory_barrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barrier.newLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	vkCmdPipelineBarrier(
	    copy_command,
	    VK_PIPELINE_STAGE_TRANSFER_BIT,
	    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	    0,
	    0, nullptr,
	    0, nullptr,
	    1, &image_memory_barrier);

	device->flush_command_buffer(copy_command, queue, true);

#ifdef _WIN32
	FreeSid(sid);
	LocalFree(acl);
#endif

	// Setting up OpenCL resources
	cl_image_format cl_img_fmt{};
	cl_img_fmt.image_channel_order     = CL_RGBA;
	cl_img_fmt.image_channel_data_type = CL_UNSIGNED_INT8;

	cl_image_desc cl_img_desc{};
	cl_img_desc.image_width       = shared_texture.width;
	cl_img_desc.image_height      = shared_texture.height;
	cl_img_desc.image_depth       = 0;
	cl_img_desc.image_type        = CL_MEM_OBJECT_IMAGE2D;
	cl_img_desc.image_array_size  = 0;
	cl_img_desc.image_row_pitch   = 0;        // Row pitch set to zero as host_ptr is NULL
	cl_img_desc.image_slice_pitch = cl_img_desc.image_row_pitch * cl_img_desc.image_height;
	cl_img_desc.num_mip_levels    = 1;
	cl_img_desc.num_samples       = 0;
	cl_img_desc.buffer            = NULL;

	void *cl_handle = NULL;

	std::vector<cl_mem_properties> mem_properties;

	cl_device_id devList[] = {cl_data->device_id, NULL};

#ifdef _WIN32
	cl_handle = get_vulkan_image_handle(VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT);
	mem_properties.push_back((cl_mem_properties) CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR);
	mem_properties.push_back((cl_mem_properties) cl_handle);
#else
	cl_handle                               = get_vulkan_image_handle(CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR);
	mem_properties.push_back((cl_mem_properties_khr) CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR);
	mem_properties.push_back((cl_mem_properties_khr) cl_handle);
#endif
	mem_properties.push_back((cl_mem_properties) CL_DEVICE_HANDLE_LIST_KHR);
	mem_properties.push_back((cl_mem_properties) cl_data->device_id);
	mem_properties.push_back((cl_mem_properties) 0x2052);        // @todo: there is a bug in either the drivers or the headers here with CL_DEVICE_HANDLE_LIST_END_KHR
	mem_properties.push_back(0);

	int cl_result;
	cl_data->image = clCreateImageWithProperties(cl_data->context,
	                                             mem_properties.data(),
	                                             CL_MEM_READ_WRITE,
	                                             &cl_img_fmt,
	                                             &cl_img_desc,
	                                             NULL,
	                                             &cl_result);
	if (CL_SUCCESS != cl_result)
	{
		std::cout << "Could not create OpenCL image, error: " << cl_result << std::endl;
		throw std::runtime_error("clCreateImageWithProperties failed");
	}
	std::cout << "clCreateImageWithProperties passed with extMemProperties\n";
}

std::vector<std::string> get_available_open_cl_extensions(cl_platform_id platform_id)
{
	size_t extensions_info_size = 0;
	clGetPlatformInfo(platform_id, CL_PLATFORM_EXTENSIONS, 0, nullptr, &extensions_info_size);

	std::vector<char> extensions_info(extensions_info_size, '\0');
	clGetPlatformInfo(platform_id, CL_PLATFORM_EXTENSIONS, extensions_info_size, extensions_info.data(), nullptr);

	std::istrstream          extensions_info_stream(extensions_info.data(), extensions_info.size());
	std::vector<std::string> extensions = std::vector<std::string>(std::istream_iterator<std::string>{extensions_info_stream}, std::istream_iterator<std::string>());

	// Remove trailing zeroes from all extensions (which otherwise may make support checks fail)
	for (auto &extension : extensions)
	{
		extension.erase(std::find(extension.begin(), extension.end(), '\0'), extension.end());
	}

	return extensions;
}

void OpenCLInterop::prepare_open_cl_resources()
{
	cl_platform_id platform_id = load_opencl();
	if (platform_id == nullptr)
	{
		throw std::runtime_error("Could not load OpenCL library");
	}

	// Vulkan/OpenCL interop also requires some extensions on the OpenCL side
	std::vector<std::string> available_extensions = get_available_open_cl_extensions(platform_id);
	// Platform independent OpenCL extensions for interop
	std::vector<std::string> required_extensions{"cl_khr_external_memory", "cl_khr_external_semaphore"};
	// Platform specific OpenCL extensions for interop
#if defined(_WIN32)
	required_extensions.push_back("cl_khr_external_memory_win32");
#else
	required_extensions.push_back("cl_khr_external_memory_opaque_fd");
#endif
	for (auto extension : required_extensions)
	{
		if (std::find(available_extensions.begin(), available_extensions.end(), extension) == available_extensions.end())
		{
			LOGE("Required OpenCL extension '{}' is not supported by the selected device", extension);
			throw std::runtime_error("Required OpenCL extension(s) not suppored");
		}
	}

	cl_uint num_devices;
	clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &cl_data->device_id, &num_devices);

	cl_int result    = CL_SUCCESS;
	cl_data->context = clCreateContext(NULL, 1, &cl_data->device_id, NULL, NULL, &result);

	if (result != CL_SUCCESS)
	{
		throw std::runtime_error("Could not create OpenCL context");
	}

	cl_data->command_queue = clCreateCommandQueue(cl_data->context, cl_data->device_id, 0, &result);

	auto   kernel_source      = vkb::fs::read_shader("open_cl_interop/procedural_texture.cl");
	auto   kernel_source_data = kernel_source.data();
	size_t kernel_source_size = kernel_source.size();

	cl_data->program = clCreateProgramWithSource(cl_data->context, 1, &kernel_source_data, &kernel_source_size, &result);
	clBuildProgram(cl_data->program, 1, &cl_data->device_id, NULL, NULL, NULL);
	cl_data->kernel = clCreateKernel(cl_data->program, "generate_texture", &result);

	if (result != CL_SUCCESS)
	{
		throw std::runtime_error("Could not create OpenCL kernel");
	}
}

void OpenCLInterop::update_texture_from_open_cl()
{
	cl_int cl_result;

	cl_result = clSetKernelArg(cl_data->kernel, 0, sizeof(cl_mem), &cl_data->image);
	cl_result |= clSetKernelArg(cl_data->kernel, 1, sizeof(float), &total_time_passed);

	if (cl_result != CL_SUCCESS)
	{
		LOGE("Could not set OpenCL kernel arguments, error code: {}", cl_result);
	}

	std::array<size_t, 2> global_size = {shared_texture.width, shared_texture.height};
	std::array<size_t, 2> local_size  = {16, 16};

	cl_result = clEnqueueNDRangeKernel(cl_data->command_queue, cl_data->kernel, global_size.size(), NULL, global_size.data(), local_size.data(), 0, NULL, NULL);

	if (cl_result != CL_SUCCESS)
	{
		LOGE("Could not execute OpenCL kernel, error code: {}", cl_result);
	}
}

std::unique_ptr<vkb::VulkanSample> create_open_cl_interop()
{
	return std::make_unique<OpenCLInterop>();
}
