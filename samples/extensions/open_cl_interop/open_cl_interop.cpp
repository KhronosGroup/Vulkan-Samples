/* Copyright (c) 2023, Sascha Willems
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

#include <sstream>

#ifdef VK_USE_PLATFORM_WIN32_KHR
#	include <aclapi.h>
#	include <dxgi1_2.h>
#endif

#ifdef _WIN32
// On Windows, we need to enable some security settings to allow api interop
// The spec states: For handles of the following types: VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT The implementation must ensure the access rights allow read and write access to the memory.
// This class sets up the structures required for tis
class WinSecurityAttributes
{
  private:
	SECURITY_ATTRIBUTES  security_attributes;
	PSECURITY_DESCRIPTOR security_descriptor;

  public:
	WinSecurityAttributes();
	~WinSecurityAttributes();
	SECURITY_ATTRIBUTES *operator&();
};

WinSecurityAttributes::WinSecurityAttributes()
{
	security_descriptor = (PSECURITY_DESCRIPTOR) calloc(1, SECURITY_DESCRIPTOR_MIN_LENGTH + 2 * sizeof(void **));

	PSID *ppSID = (PSID *) ((PBYTE) security_descriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);
	PACL *ppACL = (PACL *) ((PBYTE) ppSID + sizeof(PSID *));

	InitializeSecurityDescriptor(security_descriptor, SECURITY_DESCRIPTOR_REVISION);

	SID_IDENTIFIER_AUTHORITY sid_identifier_authority = SECURITY_WORLD_SID_AUTHORITY;
	AllocateAndInitializeSid(&sid_identifier_authority, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, ppSID);

	EXPLICIT_ACCESS explicit_access{};
	ZeroMemory(&explicit_access, sizeof(EXPLICIT_ACCESS));
	explicit_access.grfAccessPermissions = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
	explicit_access.grfAccessMode        = SET_ACCESS;
	explicit_access.grfInheritance       = INHERIT_ONLY;
	explicit_access.Trustee.TrusteeForm  = TRUSTEE_IS_SID;
	explicit_access.Trustee.TrusteeType  = TRUSTEE_IS_WELL_KNOWN_GROUP;
	explicit_access.Trustee.ptstrName    = (LPTSTR) *ppSID;
	SetEntriesInAcl(1, &explicit_access, nullptr, ppACL);

	SetSecurityDescriptorDacl(security_descriptor, TRUE, *ppACL, FALSE);

	security_attributes.nLength              = sizeof(SECURITY_ATTRIBUTES);
	security_attributes.lpSecurityDescriptor = security_descriptor;
	security_attributes.bInheritHandle       = TRUE;
}

SECURITY_ATTRIBUTES *WinSecurityAttributes::operator&()
{
	return &security_attributes;
}

WinSecurityAttributes::~WinSecurityAttributes()
{
	PSID *ppSID = (PSID *) ((PBYTE) security_descriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);
	PACL *ppACL = (PACL *) ((PBYTE) ppSID + sizeof(PSID *));

	if (*ppSID)
	{
		FreeSid(*ppSID);
	}

	if (*ppACL)
	{
		LocalFree(*ppACL);
	}

	free(security_descriptor);
}
#endif

OpenCLInterop::OpenCLInterop()
{
	zoom  = -3.5f;
	title = "Interoperability with OpenCL";

	// To use external memory and semaphores, we need to enable several extensions, both on the device as well as the instance
	add_device_extension(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
	add_device_extension(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
	// Some of the extensions are platform dependent
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
		vkDestroySemaphore(device->get_handle(), cl_update_vk_semaphore, nullptr);
		vkDestroySemaphore(device->get_handle(), vk_update_cl_semaphore, nullptr);
		vkDestroySampler(device->get_handle(), shared_image.sampler, nullptr);
		vkDestroyImageView(device->get_handle(), shared_image.view, nullptr);
		vkDestroyImage(device->get_handle(), shared_image.image, nullptr);
		vkFreeMemory(device->get_handle(), shared_image.memory, nullptr);
	}

	if (opencl_objects.initialized)
	{
		clReleaseMemObject(opencl_objects.image);
		clReleaseContext(opencl_objects.context);
		clReleaseSemaphoreKHR(opencl_objects.cl_update_vk_semaphore);
		clReleaseSemaphoreKHR(opencl_objects.vk_update_cl_semaphore);
	}

	unload_opencl();
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

	ApiVulkanSample::prepare_frame();

	std::vector<VkPipelineStageFlags> wait_stages{};
	std::vector<VkSemaphore>          wait_semaphores{};
	std::vector<VkSemaphore>          signal_semaphores{};

	VkSubmitInfo submit_info       = {};
	submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	submit_info.commandBufferCount = 1;

	// As we have no way to manually signal the semaphores, we need to distinguish between the first and consecutive submits
	// The first submit can't wait on the (yet) unsignaled OpenCL semaphore, so we only wait for that after the first submit

	if (first_submit)
	{
		first_submit      = false;
		wait_stages       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		wait_semaphores   = {semaphores.acquired_image_ready};
		signal_semaphores = {semaphores.render_complete, vk_update_cl_semaphore};
	}
	else
	{
		wait_stages       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
		wait_semaphores   = {semaphores.acquired_image_ready, cl_update_vk_semaphore};
		signal_semaphores = {semaphores.render_complete, vk_update_cl_semaphore};
	}

	submit_info.pWaitDstStageMask    = wait_stages.data();
	submit_info.waitSemaphoreCount   = static_cast<uint32_t>(wait_semaphores.size());
	submit_info.pWaitSemaphores      = wait_semaphores.data();
	submit_info.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size());
	submit_info.pSignalSemaphores    = signal_semaphores.data();

	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, rendering_finished_fence));

	ApiVulkanSample::submit_frame();

	// Update the image from OpenCL

	// To make sure OpenCL won't start updating the image until Vulkan has finished rendering to it, we wait for the Vulkan->OpenCL semaphore
	CL_CHECK(clEnqueueWaitSemaphoresKHR(opencl_objects.command_queue, 1, &opencl_objects.vk_update_cl_semaphore, nullptr, 0, nullptr, nullptr));
	// We also need to acquire the image (resource) so we can update it with OpenCL
	CL_CHECK(clEnqueueAcquireExternalMemObjectsKHR(opencl_objects.command_queue, 1, &opencl_objects.image, 0, nullptr, nullptr));

	std::array<size_t, 2> global_size = {shared_image.width, shared_image.height};
	std::array<size_t, 2> local_size  = {16, 16};

	CL_CHECK(clSetKernelArg(opencl_objects.kernel, 0, sizeof(cl_mem), &opencl_objects.image));
	CL_CHECK(clSetKernelArg(opencl_objects.kernel, 1, sizeof(float), &total_time_passed));
	CL_CHECK(clEnqueueNDRangeKernel(opencl_objects.command_queue, opencl_objects.kernel, global_size.size(), nullptr, global_size.data(), local_size.data(), 0, nullptr, nullptr));

	// Release the image (resource) to Vulkan
	CL_CHECK(clEnqueueReleaseExternalMemObjectsKHR(opencl_objects.command_queue, 1, &opencl_objects.image, 0, nullptr, nullptr));
	// Signal a semaphore that the next Vulkan frame can wait on (first_submit != false)
	CL_CHECK(clEnqueueSignalSemaphoresKHR(opencl_objects.command_queue, 1, &opencl_objects.cl_update_vk_semaphore, nullptr, 0, nullptr, nullptr));
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
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	    // Binding 1 : Fragment shader image sampler
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	};
	VkDescriptorSetLayoutCreateInfo descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), vkb::to_u32(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layout));
	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void OpenCLInterop::setup_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniform_buffer_vs);

	// Setup a descriptor image info for the current texture to be used as a combined image sampler
	VkDescriptorImageInfo image_descriptor;
	image_descriptor.imageView   = shared_image.view;
	image_descriptor.sampler     = shared_image.sampler;
	image_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	std::vector<VkWriteDescriptorSet> write_descriptor_sets{
	    // Binding 0 : Vertex shader uniform buffer
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &buffer_descriptor),
	    // Binding 1 : Fragment shader texture sampler
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &image_descriptor),
	};
	vkUpdateDescriptorSets(get_device().get_handle(), vkb::to_u32(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
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

	shader_stages[0] = load_shader("open_cl_interop/texture_display.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("open_cl_interop/texture_display.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(VertexStructure),
	                                                        VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexStructure, pos)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexStructure, uv)),
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexStructure, normal)),
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = vkb::to_u32(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = vkb::to_u32(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout, render_pass, 0);

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

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
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
	ubo_vs.projection     = glm::perspective(glm::radians(60.0f), (float) width / (float) height, 0.001f, 256.0f);
	glm::mat4 view_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));
	ubo_vs.model          = view_matrix * glm::translate(glm::mat4(1.0f), camera_pos);
	ubo_vs.model          = glm::rotate(ubo_vs.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	ubo_vs.model          = glm::rotate(ubo_vs.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo_vs.model          = glm::rotate(ubo_vs.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo_vs.view_pos       = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);
	uniform_buffer_vs->convert_and_update(ubo_vs);
}

// These functions wraps the platform specific functions to get platform handles for Vulkan memory objects (e.g. the memory backing the image) and semaphores

#ifdef _WIN32
HANDLE OpenCLInterop::get_vulkan_memory_handle(VkDeviceMemory memory)
{
	HANDLE                        handle;
	VkMemoryGetWin32HandleInfoKHR win32_handle_info{};
	win32_handle_info.sType      = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
	win32_handle_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
	win32_handle_info.memory     = memory;
	vkGetMemoryWin32HandleKHR(device->get_handle(), &win32_handle_info, &handle);
	return handle;
}

HANDLE OpenCLInterop::get_vulkan_semaphore_handle(VkSemaphore &sempahore)
{
	HANDLE                           handle;
	VkSemaphoreGetWin32HandleInfoKHR win32_handle_info{};
	win32_handle_info.sType      = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
	win32_handle_info.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
	win32_handle_info.semaphore  = sempahore;
	vkGetSemaphoreWin32HandleKHR(device->get_handle(), &win32_handle_info, &handle);
	return handle;
}
#else
int OpenCLInterop::get_vulkan_memory_handle(VkDeviceMemory memory)
{
	int fd;
	VkMemoryGetFdInfoKHR fd_info{};
	fd_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
	fd_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
	fd_info.memory = memory;
	vkGetMemoryFdKHR(device->get_handle(), &fd_info, &fd);
	return fd;
}

int OpenCLInterop::get_vulkan_semaphore_handle(VkSemaphore &sempahore)
{
	int fd;
	VkSemaphoreGetFdInfoKHR fd_info{};
	fd_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
	fd_info.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
	fd_info.semaphore = sempahore;
	vkGetSemaphoreFdKHR(device->get_handle(), &fd_info, &fd);
	return fd;
}
#endif

void OpenCLInterop::prepare_shared_image()
{
	// This texture will be shared between both APIs: OpenCL fills it and Vulkan uses it for rendering
	shared_image.width  = 512;
	shared_image.height = 512;

	// We need to select the external handle type based on our target platform
	// Note: Windows 8 and older requires the _KMT suffixed handle type, which we don't support in this sample
	VkExternalMemoryHandleTypeFlagBits external_handle_type{};
#ifdef _WIN32
	external_handle_type = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
#else
	external_handle_type = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
#endif

	auto device_handle = get_device().get_handle();

	// Setting up Vulkan resources (image, memory, image view and sampler)

	VkImageCreateInfo image_create_info = vkb::initializers::image_create_info();
	image_create_info.imageType         = VK_IMAGE_TYPE_2D;
	image_create_info.format            = VK_FORMAT_R8G8B8A8_UNORM;
	image_create_info.mipLevels         = 1;
	image_create_info.arrayLayers       = 1;
	image_create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.extent            = {shared_image.width, shared_image.height, 1};
	image_create_info.usage             = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	VkExternalMemoryImageCreateInfo external_memory_image_info{};
	external_memory_image_info.sType       = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
	external_memory_image_info.handleTypes = external_handle_type;

	image_create_info.pNext = &external_memory_image_info;
	VK_CHECK(vkCreateImage(get_device().get_handle(), &image_create_info, nullptr, &shared_image.image));

	VkMemoryRequirements memory_requirements{};
	vkGetImageMemoryRequirements(device->get_handle(), shared_image.image, &memory_requirements);

	VkExportMemoryAllocateInfoKHR export_memory_allocate_info{};
	export_memory_allocate_info.sType       = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
	export_memory_allocate_info.handleTypes = external_handle_type;

#ifdef _WIN32
	WinSecurityAttributes            win_security_attributes;
	VkExportMemoryWin32HandleInfoKHR export_memory_win32_handle_info{};
	export_memory_win32_handle_info.sType       = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
	export_memory_win32_handle_info.pAttributes = &win_security_attributes;
	export_memory_win32_handle_info.dwAccess    = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
	export_memory_allocate_info.pNext           = &export_memory_win32_handle_info;
#endif

	VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
	memory_allocate_info.pNext                = &export_memory_allocate_info;
	memory_allocate_info.allocationSize       = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex      = device->get_memory_type(memory_requirements.memoryTypeBits, 0);

	VK_CHECK(vkAllocateMemory(device_handle, &memory_allocate_info, nullptr, &shared_image.memory));
	VK_CHECK(vkBindImageMemory(device_handle, shared_image.image, shared_image.memory, 0));

	VkSamplerCreateInfo sampler_create_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	sampler_create_info.magFilter   = VK_FILTER_LINEAR;
	sampler_create_info.minFilter   = VK_FILTER_LINEAR;
	sampler_create_info.mipmapMode  = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_create_info.maxLod      = (float) 1;
	sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	vkCreateSampler(device_handle, &sampler_create_info, nullptr, &shared_image.sampler);

	VkImageViewCreateInfo view_create_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	view_create_info.viewType         = VK_IMAGE_VIEW_TYPE_2D;
	view_create_info.image            = shared_image.image;
	view_create_info.format           = VK_FORMAT_R8G8B8A8_UNORM;
	view_create_info.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	vkCreateImageView(device_handle, &view_create_info, nullptr, &shared_image.view);

	VkCommandBuffer copy_command = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkImageSubresourceRange subresource_range = {};
	subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel            = 0;
	subresource_range.levelCount              = 1;
	subresource_range.layerCount              = 1;

	VkImageMemoryBarrier image_memory_barrier = vkb::initializers::image_memory_barrier();
	image_memory_barrier.image                = shared_image.image;
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

	// Import the image into OpenCL

	std::vector<cl_mem_properties> mem_properties;

#ifdef _WIN32
	HANDLE handle = get_vulkan_memory_handle(shared_image.memory);
	mem_properties.push_back((cl_mem_properties) CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR);
	mem_properties.push_back((cl_mem_properties) handle);
#else
	int fd = get_vulkan_memory_handle(shared_image.memory);
	mem_properties.push_back((cl_mem_properties) CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR);
	mem_properties.push_back((cl_mem_properties) fd);
#endif
	mem_properties.push_back((cl_mem_properties) CL_DEVICE_HANDLE_LIST_KHR);
	mem_properties.push_back((cl_mem_properties) opencl_objects.device_id);
	mem_properties.push_back((cl_mem_properties) CL_DEVICE_HANDLE_LIST_END_KHR);
	mem_properties.push_back(0);

	cl_image_format cl_img_fmt{};
	cl_img_fmt.image_channel_order     = CL_RGBA;
	cl_img_fmt.image_channel_data_type = CL_UNSIGNED_INT8;

	cl_image_desc cl_img_desc{};
	cl_img_desc.image_width       = shared_image.width;
	cl_img_desc.image_height      = shared_image.height;
	cl_img_desc.image_type        = CL_MEM_OBJECT_IMAGE2D;
	cl_img_desc.image_slice_pitch = cl_img_desc.image_row_pitch * cl_img_desc.image_height;
	cl_img_desc.num_mip_levels    = 1;
	cl_img_desc.buffer            = nullptr;

	int cl_result;
	opencl_objects.image = clCreateImageWithProperties(opencl_objects.context,
	                                                   mem_properties.data(),
	                                                   CL_MEM_READ_WRITE,
	                                                   &cl_img_fmt,
	                                                   &cl_img_desc,
	                                                   NULL,
	                                                   &cl_result);
	CL_CHECK(cl_result);
}

void OpenCLInterop::prepare_sync_objects()
{
	// Just as the image, we also create the semaphores in Vulkan and export them
	VkExportSemaphoreCreateInfoKHR export_semaphore_create_info{};
	export_semaphore_create_info.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR;

#ifdef _WIN32
	WinSecurityAttributes               win_security_attributes;
	VkExportSemaphoreWin32HandleInfoKHR export_semaphore_handle_info{};
	export_semaphore_handle_info.sType       = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
	export_semaphore_handle_info.pAttributes = &win_security_attributes;
	export_semaphore_handle_info.dwAccess    = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;

	export_semaphore_create_info.pNext       = &export_semaphore_handle_info;
	export_semaphore_create_info.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
	export_semaphore_create_info.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

	VkSemaphoreCreateInfo semaphore_create_info{};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_create_info.pNext = &export_semaphore_create_info;

	VK_CHECK(vkCreateSemaphore(device->get_handle(), &semaphore_create_info, nullptr, &cl_update_vk_semaphore));
	VK_CHECK(vkCreateSemaphore(device->get_handle(), &semaphore_create_info, nullptr, &vk_update_cl_semaphore));

	// We also need a fence for the Vulkan side of things, which is not shared with OpenCL
	VkFenceCreateInfo fence_create_info = vkb::initializers::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	vkCreateFence(device->get_handle(), &fence_create_info, nullptr, &rendering_finished_fence);

	// Import the Vulkan sempahores into OpenCL
	std::vector<cl_semaphore_properties_khr> semaphore_properties{
	    (cl_semaphore_properties_khr) CL_SEMAPHORE_TYPE_KHR,
	    (cl_semaphore_properties_khr) CL_SEMAPHORE_TYPE_BINARY_KHR,
	    (cl_semaphore_properties_khr) CL_DEVICE_HANDLE_LIST_KHR,
	    (cl_semaphore_properties_khr) opencl_objects.device_id,
	    (cl_semaphore_properties_khr) CL_DEVICE_HANDLE_LIST_END_KHR,
	};

	// CL to VK semaphore

#ifdef _WIN32
	semaphore_properties.push_back((cl_semaphore_properties_khr) CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KHR);
	HANDLE handle = get_vulkan_semaphore_handle(cl_update_vk_semaphore);
	semaphore_properties.push_back((cl_semaphore_properties_khr) handle);
#else
	semaphore_properties.push_back((cl_semaphore_properties_khr) CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR);
	int fd = get_vulkan_semaphore_handle(cl_update_vk_semaphore);
	semaphore_properties.push_back((cl_semaphore_properties_khr) fd);
#endif
	semaphore_properties.push_back(0);

	cl_int cl_result;

	opencl_objects.cl_update_vk_semaphore = clCreateSemaphoreWithPropertiesKHR(opencl_objects.context, semaphore_properties.data(), &cl_result);
	CL_CHECK(cl_result);

	// Remove the last two entries so we can push the next handle and zero terminator to the properties list and re-use the other values
	semaphore_properties.pop_back();
	semaphore_properties.pop_back();

	// VK to CL semaphore
#ifdef _WIN32
	handle = get_vulkan_semaphore_handle(vk_update_cl_semaphore);
	semaphore_properties.push_back((cl_semaphore_properties_khr) handle);
#else
	fd = get_vulkan_semaphore_handle(vk_update_cl_semaphore);
	semaphore_properties.push_back((cl_semaphore_properties_khr) fd);
#endif
	semaphore_properties.push_back(0);

	opencl_objects.vk_update_cl_semaphore = clCreateSemaphoreWithPropertiesKHR(opencl_objects.context, semaphore_properties.data(), &cl_result);
	CL_CHECK(cl_result);
}

void OpenCLInterop::prepare_opencl_resources()
{
	load_opencl();

	// We need to ensure that we get the same device in OpenCL as we got in Vulkan
	// To do this, we compare the unique device identifier of the current Vulkan implementation with the list of available
	// devices in OpenCL and then select the OpenCL platform that matches
	// See https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceIDProperties.html

	// Get the UUID of the current Vulkan device
	VkPhysicalDeviceIDPropertiesKHR physical_device_id_propreties{};
	physical_device_id_propreties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
	VkPhysicalDeviceProperties2 physical_device_properties_2{};
	physical_device_properties_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
	physical_device_properties_2.pNext = &physical_device_id_propreties;
	vkGetPhysicalDeviceProperties2KHR(device->get_gpu().get_handle(), &physical_device_properties_2);

	// We also need to make sure the OpenCL platform/device supports all the extensions required in this sample
	std::vector<std::string> required_extensions{
	    // Platform independent OpenCL extensions for interop and for getting the device
	    "cl_khr_external_memory", 
		"cl_khr_external_semaphore",
		// Extension required to read the uuid of a device (see below for more information on why this is required)
	    "cl_khr_device_uuid"
	};
	// Platform specific OpenCL extensions for interop
#if defined(_WIN32)
	required_extensions.push_back("cl_khr_external_memory_win32");
	required_extensions.push_back("cl_khr_external_semaphore_win32");
#else
	required_extensions.push_back("cl_khr_external_memory_opaque_fd");
	required_extensions.push_back("cl_khr_external_semaphore_opaque_fd");
#endif

	// Iterate over all available OpenCL platforms and find the first that fits our requirements (extensions, device UUID)

	cl_uint num_platforms;
	clGetPlatformIDs_ptr(0, nullptr, &num_platforms);

	std::vector<cl_platform_id> platform_ids(num_platforms);
	clGetPlatformIDs_ptr(num_platforms, platform_ids.data(), nullptr);

	cl_platform_id selected_platform_id{nullptr};
	cl_device_id   selected_device_id{nullptr};

	for (auto &platform_id : platform_ids)
	{
		cl_uint        num_devices;
		clGetDeviceIDs_ptr(platform_id, CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices);
		std::vector<cl_device_id> device_ids(num_devices);
		clGetDeviceIDs_ptr(platform_id, CL_DEVICE_TYPE_ALL, num_devices, device_ids.data(), nullptr);

		// Check if this platform supports all required extensions
		size_t extension_string_size = 0;
		clGetPlatformInfo(platform_id, CL_PLATFORM_EXTENSIONS, 0, nullptr, &extension_string_size);

		std::string extension_string(extension_string_size, ' ');
		clGetPlatformInfo(platform_id, CL_PLATFORM_EXTENSIONS, extension_string_size, &extension_string[0], nullptr);

		std::vector<std::string> available_extensions{};
		std::stringstream        extension_stream(extension_string);
		std::string              extension;
		while (std::getline(extension_stream, extension, ' '))
		{
			// Remove trailing zeroes from all extensions (which otherwise may make support checks fail)
			extension.erase(std::find(extension.begin(), extension.end(), '\0'), extension.end());
			available_extensions.push_back(extension);
		}

		bool extensions_present = true;

		for (auto extension : required_extensions)
		{
			if (std::find(available_extensions.begin(), available_extensions.end(), extension) == available_extensions.end())
			{
				extensions_present = false;
				break;
			}
		}

		if (!extensions_present)
		{
			continue;
		}	

		// Check every device of this platform and see if it matches our Vulkan device UUID
		selected_device_id = nullptr;
		for (auto &device_id : device_ids)
		{
			cl_uchar uuid[CL_UUID_SIZE_KHR];
			clGetDeviceInfo_ptr(device_id, CL_DEVICE_UUID_KHR, sizeof(uuid), &uuid, nullptr);

			bool device_uuid_match = true;

			for (uint32_t i = 0; i < CL_UUID_SIZE_KHR; i++)
			{
				if (uuid[i] != physical_device_id_propreties.deviceUUID[i])
				{
					device_uuid_match = false;
					break;
				}
			}

			if (!device_uuid_match)
			{
				continue;
			}

			// We found a device with a matching UUID, so use it
			selected_device_id = device_id;
			break;
		}

		// We found a platform that supportes the required extensions and has a device with a matching UUID
		if (selected_device_id)
		{
			selected_platform_id = platform_id;
			break;
		}
	}

	if ((selected_platform_id == nullptr) || (selected_device_id == nullptr))
	{
		const std::string message{"Could not find an OpenCL platform + device that matches the required extensions and also matches the Vulkan device UUID "};
		LOGE(message);
		throw std::runtime_error(message);
	}

	opencl_objects.device_id = selected_device_id;

	cl_int cl_result;

	opencl_objects.context = clCreateContext(NULL, 1, &opencl_objects.device_id, nullptr, nullptr, &cl_result);
	CL_CHECK(cl_result);

	opencl_objects.command_queue = clCreateCommandQueue(opencl_objects.context, opencl_objects.device_id, 0, &cl_result);
	CL_CHECK(cl_result);

	auto   kernel_source      = vkb::fs::read_shader("open_cl_interop/procedural_texture.cl");
	auto   kernel_source_data = kernel_source.data();
	size_t kernel_source_size = kernel_source.size();

	opencl_objects.program = clCreateProgramWithSource(opencl_objects.context, 1, &kernel_source_data, &kernel_source_size, &cl_result);
	CL_CHECK(cl_result);

	CL_CHECK(clBuildProgram(opencl_objects.program, 1, &opencl_objects.device_id, nullptr, nullptr, nullptr));

	opencl_objects.kernel = clCreateKernel(opencl_objects.program, "generate_texture", &cl_result);
	CL_CHECK(cl_result);
}

bool OpenCLInterop::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	prepare_opencl_resources();
	prepare_sync_objects();
	prepare_shared_image();
	generate_quad();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();

	opencl_objects.initialized = true;
	prepared                   = true;
	return true;
}

std::unique_ptr<vkb::VulkanSample> create_open_cl_interop()
{
	return std::make_unique<OpenCLInterop>();
}
