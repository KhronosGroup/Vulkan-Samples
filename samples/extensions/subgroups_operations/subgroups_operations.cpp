/* Copyright (c) 2023, Mobica Limited
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

#include "subgroups_operations.h"
#include <glsl_compiler.h>

void SubgroupsOperations::Pipeline::destroy(VkDevice device)
{
	if (pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device, pipeline, nullptr);
	}
	if (pipeline_layout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
	}
}

SubgroupsOperations::SubgroupsOperations()
{
	// SPIRV 1.4 requires Vulkan 1.1
	set_api_version(VK_API_VERSION_1_1);

	// Subgroup size control extensions required by this sample
	add_device_extension(VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME);

	// Required for VK_EXT_subgroup_size_control
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

	// Required by VK_KHR_spirv_1_4
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

	// Targeting SPIR-V version
	vkb::GLSLCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);

	title       = "Subgroups operations";
	camera.type = vkb::CameraType::LookAt;

	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 256.0f);
	camera.set_position({-1.98f, -2.24f, -28.55f});
	camera.set_rotation({-24.0f, -7.0f, 0.0f});
}

SubgroupsOperations::~SubgroupsOperations()
{
	if (device)
	{
		compute.pipelines._default.destroy(get_device().get_handle());
		vkDestroyDescriptorSetLayout(get_device().get_handle(), compute.descriptor_set_layout, nullptr);
		vkDestroySemaphore(get_device().get_handle(), compute.semaphore, nullptr);
		vkDestroyCommandPool(get_device().get_handle(), compute.command_pool, nullptr);

		ocean.pipelines._default.destroy(get_device().get_handle());
		ocean.pipelines.wireframe.destroy(get_device().get_handle());
		vkDestroyDescriptorSetLayout(get_device().get_handle(), ocean.descriptor_set_layout, nullptr);
		vkDestroySemaphore(get_device().get_handle(), ocean.semaphore, nullptr);
	}
}

bool SubgroupsOperations::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	ocean.graphics_queue_family_index = get_device().get_queue_family_index(VK_QUEUE_GRAPHICS_BIT);

	load_assets();
	setup_descriptor_pool();
	prepare_uniform_buffers();
	prepare_compute();

	// prepare grpahics pipeline
	create_semaphore();
	create_descriptor_set_layout();
	create_descriptor_set();
	create_pipelines();

	build_command_buffers();

	// signal semaphore
	VkSubmitInfo submit_info         = vkb::initializers::submit_info();
	submit_info.signalSemaphoreCount = 1u;
	submit_info.pSignalSemaphores    = &ocean.semaphore;
	VK_CHECK(vkQueueSubmit(queue, 1u, &submit_info, VK_NULL_HANDLE));
	get_device().wait_idle();

	prepared = true;
	return true;
}

void SubgroupsOperations::prepare_compute()
{
	create_compute_queue();
	create_compute_command_pool();
	create_compute_command_buffer();
	create_compute_descriptor_set_layout();
	create_compute_descriptor_set();
	preapre_compute_pipeline_layout();
	prepare_compute_pipeline();
	build_compute_command_buffer();
}

void SubgroupsOperations::create_compute_queue()
{
	// create compute queue and get family index
	compute.queue_family_index = get_device().get_queue_family_index(VK_QUEUE_COMPUTE_BIT);

	vkGetDeviceQueue(get_device().get_handle(), compute.queue_family_index, 0u, &compute.queue);
}

void SubgroupsOperations::create_compute_command_pool()
{
	VkCommandPoolCreateInfo command_pool_create_info = {};
	command_pool_create_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.queueFamilyIndex        = compute.queue_family_index;
	command_pool_create_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_CHECK(vkCreateCommandPool(get_device().get_handle(), &command_pool_create_info, nullptr, &compute.command_pool));
}

void SubgroupsOperations::create_compute_command_buffer()
{
	// Create a command buffer for compute operations
	VkCommandBufferAllocateInfo command_buffer_allocate_info =
	    vkb::initializers::command_buffer_allocate_info(compute.command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1u);

	VK_CHECK(vkAllocateCommandBuffers(get_device().get_handle(), &command_buffer_allocate_info, &compute.command_buffer));

	// Semaphore for compute & graphics sync
	VkSemaphoreCreateInfo semaphore_create_info = vkb::initializers::semaphore_create_info();
	VK_CHECK(vkCreateSemaphore(get_device().get_handle(), &semaphore_create_info, nullptr, &compute.semaphore));
}

void SubgroupsOperations::create_compute_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindngs = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0u),        // ubo
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1u),        // input vertex
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2u)         // output vertex
	};
	VkDescriptorSetLayoutCreateInfo descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindngs);

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &compute.descriptor_set_layout));
}

void SubgroupsOperations::create_compute_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &compute.descriptor_set_layout, 1u);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &compute.descriptor_set));

	update_compute_descriptor();
}

void SubgroupsOperations::update_compute_descriptor()
{
	VkDescriptorBufferInfo            time_ubo_buffer        = create_descriptor(*compute_ubo);
	VkDescriptorBufferInfo            input_vertices_buffer  = create_descriptor(*input_grid.vertex);
	VkDescriptorBufferInfo            output_vertices_buffer = create_descriptor(*ocean.grid.vertex);
	std::vector<VkWriteDescriptorSet> wirte_descriptor_sets  = {
        vkb::initializers::write_descriptor_set(compute.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, &time_ubo_buffer),
        vkb::initializers::write_descriptor_set(compute.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, &input_vertices_buffer),
        vkb::initializers::write_descriptor_set(compute.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u, &output_vertices_buffer)};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(wirte_descriptor_sets.size()), wirte_descriptor_sets.data(), 0u, nullptr);
}

void SubgroupsOperations::preapre_compute_pipeline_layout()
{
	VkPipelineLayoutCreateInfo compute_pipeline_layout_info = {};
	compute_pipeline_layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	compute_pipeline_layout_info.setLayoutCount             = 1u;
	compute_pipeline_layout_info.pSetLayouts                = &compute.descriptor_set_layout;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &compute_pipeline_layout_info, nullptr, &compute.pipelines._default.pipeline_layout));
}

void SubgroupsOperations::prepare_compute_pipeline()
{
	VkComputePipelineCreateInfo computeInfo = {};
	computeInfo.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computeInfo.layout                      = compute.pipelines._default.pipeline_layout;
	computeInfo.stage                       = load_shader("subgroups_operations/ocean_fft.comp", VK_SHADER_STAGE_COMPUTE_BIT);

	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1u, &computeInfo, nullptr, &compute.pipelines._default.pipeline));
}

void SubgroupsOperations::build_compute_command_buffer()
{
	// record command
	VkCommandBufferBeginInfo begin_info = vkb::initializers::command_buffer_begin_info();
	VK_CHECK(vkBeginCommandBuffer(compute.command_buffer, &begin_info));

	if (compute.queue_family_index != ocean.graphics_queue_family_index)
	{
		VkBufferMemoryBarrier memory_barrier = vkb::initializers::buffer_memory_barrier();
		memory_barrier.srcAccessMask         = 0;
		memory_barrier.dstAccessMask         = VK_ACCESS_SHADER_WRITE_BIT;
		memory_barrier.srcQueueFamilyIndex   = ocean.graphics_queue_family_index;
		memory_barrier.dstQueueFamilyIndex   = compute.queue_family_index;
		memory_barrier.buffer                = ocean.grid.vertex->get_handle();
		memory_barrier.offset                = 0;
		memory_barrier.size                  = ocean.grid.vertex->get_size();

		vkCmdPipelineBarrier(compute.command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &memory_barrier, 0, nullptr);
	}

	vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelines._default.pipeline);
	vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelines._default.pipeline_layout, 0u, 1u, &compute.descriptor_set, 0u, nullptr);

	vkCmdDispatch(compute.command_buffer, grid_size, grid_size, 1u);

	if (compute.queue_family_index != ocean.graphics_queue_family_index)
	{
		VkBufferMemoryBarrier memory_barrier = vkb::initializers::buffer_memory_barrier();
		memory_barrier.srcAccessMask         = 0;
		memory_barrier.dstAccessMask         = VK_ACCESS_SHADER_WRITE_BIT;
		memory_barrier.srcQueueFamilyIndex   = compute.queue_family_index;
		memory_barrier.dstQueueFamilyIndex   = ocean.graphics_queue_family_index;
		memory_barrier.buffer                = ocean.grid.vertex->get_handle();
		memory_barrier.offset                = 0;
		memory_barrier.size                  = ocean.grid.vertex->get_size();

		vkCmdPipelineBarrier(compute.command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 1, &memory_barrier, 0, nullptr);
	}

	VK_CHECK(vkEndCommandBuffer(compute.command_buffer));
}

void SubgroupsOperations::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	if (gpu.get_features().fillModeNonSolid)
	{
		gpu.get_mutable_requested_features().fillModeNonSolid = VK_TRUE;
	}

	subgroups_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
	subgroups_properties.pNext = VK_NULL_HANDLE;

	VkPhysicalDeviceProperties2 device_properties2 = {};
	device_properties2.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	device_properties2.pNext                       = &subgroups_properties;
	vkGetPhysicalDeviceProperties2(gpu.get_handle(), &device_properties2);
}

void SubgroupsOperations::load_assets()
{
	generate_grid();
}

void SubgroupsOperations::prepare_uniform_buffers()
{
	camera_ubo  = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(CameraUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	compute_ubo = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ComputeUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	update_uniform_buffers();
}

void SubgroupsOperations::generate_grid()
{
	std::vector<Vertex>   grid_vertices;
	std::vector<uint32_t> grid_indices;

	for (uint32_t z = 0u; z <= grid_size; ++z)
	{
		for (uint32_t x = 0u; x <= grid_size; ++x)
		{
			Vertex point = {};
			point.pos.x  = static_cast<float>((static_cast<float>(x) / static_cast<float>(grid_size - 1) * 2 - 1) * (grid_size / 2.0f));
			point.pos.z  = static_cast<float>((static_cast<float>(z) / static_cast<float>(grid_size - 1) * 2 - 1) * (grid_size / 2.0f));
			point.pos.y  = 0.0f;
			grid_vertices.push_back(point);
		}
	}

	for (uint32_t z = 0u; z < grid_size; ++z)
	{
		for (uint32_t x = 0u; x < grid_size; ++x)
		{
			uint32_t i0 = z * (grid_size + 1u) + x;
			uint32_t i1 = i0 + 1u;
			uint32_t i2 = i0 + (grid_size + 1u);
			uint32_t i3 = i2 + 1u;

			grid_indices.push_back(i0);
			grid_indices.push_back(i2);
			grid_indices.push_back(i1);

			grid_indices.push_back(i1);
			grid_indices.push_back(i2);
			grid_indices.push_back(i3);
		}
	}

	auto vertex_buffer_size = vkb::to_u32(grid_vertices.size() * sizeof(Vertex));
	auto index_buffer_size  = vkb::to_u32(grid_indices.size() * sizeof(uint32_t));
	ocean.grid.index_count  = vkb::to_u32(grid_indices.size());

	input_grid.vertex = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                        vertex_buffer_size,
	                                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);
	input_grid.vertex->update(grid_vertices.data(), vertex_buffer_size);

	ocean.grid.index = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                       index_buffer_size,
	                                                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                                       VMA_MEMORY_USAGE_CPU_TO_GPU);
	ocean.grid.index->update(grid_indices.data(), index_buffer_size);

	ocean.grid.vertex = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                        vertex_buffer_size,
	                                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                        VMA_MEMORY_USAGE_GPU_ONLY);
}

void SubgroupsOperations::create_semaphore()
{
	// Semaphore for graphics queue
	VkSemaphoreCreateInfo semaphore_create_info = vkb::initializers::semaphore_create_info();
	VK_CHECK(vkCreateSemaphore(get_device().get_handle(), &semaphore_create_info, nullptr, &ocean.semaphore));
}

void SubgroupsOperations::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2u),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u)};
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), 2u);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void SubgroupsOperations::create_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0u)};

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_set_layout_create_info, nullptr, &ocean.descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&ocean.descriptor_set_layout);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &ocean.pipelines._default.pipeline_layout));
}

void SubgroupsOperations::create_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &ocean.descriptor_set_layout, 1u);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &ocean.descriptor_set));

	VkDescriptorBufferInfo            buffer_descriptor     = create_descriptor(*camera_ubo);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(ocean.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, &buffer_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0u, nullptr);
}

void SubgroupsOperations::create_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	        0u,
	        VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_NONE,
	        VK_FRONT_FACE_COUNTER_CLOCKWISE,
	        0u);
	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        0xf,
	        VK_FALSE);
	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1u,
	        &blend_attachment_state);
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE,
	        VK_TRUE,
	        VK_COMPARE_OP_GREATER);
	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1u, 1u, 0u);
	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0u);
	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0u);
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
	shader_stages[0]                                                         = load_shader("subgroups_operations/ocean.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]                                                         = load_shader("subgroups_operations/ocean.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0u, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0u, 0u, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos))};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(ocean.pipelines._default.pipeline_layout, render_pass, 0u);
	pipeline_create_info.pVertexInputState            = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
	pipeline_create_info.pRasterizationState          = &rasterization_state;
	pipeline_create_info.pColorBlendState             = &color_blend_state;
	pipeline_create_info.pMultisampleState            = &multisample_state;
	pipeline_create_info.pViewportState               = &viewport_state;
	pipeline_create_info.pDepthStencilState           = &depth_stencil_state;
	pipeline_create_info.pDynamicState                = &dynamic_state;
	pipeline_create_info.stageCount                   = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages                      = shader_stages.data();
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1u, &pipeline_create_info, nullptr, &ocean.pipelines._default.pipeline));

	if (get_device().get_gpu().get_features().fillModeNonSolid)
	{
		rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;
		VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1u, &pipeline_create_info, nullptr, &ocean.pipelines.wireframe.pipeline));
	}
}

void SubgroupsOperations::update_uniform_buffers()
{
	CameraUbo ubo;
	ubo.model      = glm::mat4(1.0f);
	ubo.model      = glm::translate(ubo.model, glm::vec3(0.0f));
	ubo.view       = camera.matrices.view;
	ubo.projection = camera.matrices.perspective;

	camera_ubo->convert_and_update(ubo);

	ComputeUbo comp_ubo;
	comp_ubo.grid_size = grid_size;
	comp_ubo.time      = float(timer.elapsed<vkb::Timer::Seconds>());
	compute_ubo->convert_and_update(comp_ubo);
}

void SubgroupsOperations::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	std::array<VkClearValue, 2> clear_values;
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0u};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = static_cast<uint32_t>(clear_values.size());
	render_pass_begin_info.pClearValues             = clear_values.data();

	for (uint32_t i = 0u; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];
		auto &cmd_buff                     = draw_cmd_buffers[i];

		VK_CHECK(vkBeginCommandBuffer(cmd_buff, &command_buffer_begin_info));

		if (compute.queue_family_index != ocean.graphics_queue_family_index)
		{
			VkBufferMemoryBarrier memory_barrier = vkb::initializers::buffer_memory_barrier();
			memory_barrier.dstAccessMask         = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			memory_barrier.srcQueueFamilyIndex   = compute.queue_family_index;
			memory_barrier.dstQueueFamilyIndex   = ocean.graphics_queue_family_index;
			memory_barrier.buffer                = ocean.grid.vertex->get_handle();
			memory_barrier.offset                = 0u;
			memory_barrier.size                  = ocean.grid.vertex->get_size();

			vkCmdPipelineBarrier(cmd_buff, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &memory_barrier, 0, nullptr);
		}

		vkCmdBeginRenderPass(cmd_buff, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(cmd_buff, 0u, 1u, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
		vkCmdSetScissor(cmd_buff, 0u, 1u, &scissor);

		// draw ocean
		{
			vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, ocean.pipelines._default.pipeline_layout, 0u, 1u, &ocean.descriptor_set, 0u, nullptr);
			vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, ui.wireframe ? ocean.pipelines.wireframe.pipeline : ocean.pipelines._default.pipeline);

			VkDeviceSize offset[] = {0u};
			vkCmdBindVertexBuffers(cmd_buff, 0u, 1u, ocean.grid.vertex->get(), offset);
			vkCmdBindIndexBuffer(cmd_buff, ocean.grid.index->get_handle(), VkDeviceSize(0), VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(cmd_buff, ocean.grid.index_count, 1u, 0u, 0u, 0u);
		}

		draw_ui(cmd_buff);

		vkCmdEndRenderPass(cmd_buff);

		if (compute.queue_family_index != ocean.graphics_queue_family_index)
		{
			VkBufferMemoryBarrier memory_barrier = vkb::initializers::buffer_memory_barrier();
			memory_barrier.dstAccessMask         = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			memory_barrier.srcQueueFamilyIndex   = ocean.graphics_queue_family_index;
			memory_barrier.dstQueueFamilyIndex   = compute.queue_family_index;
			memory_barrier.buffer                = ocean.grid.vertex->get_handle();
			memory_barrier.offset                = 0u;
			memory_barrier.size                  = ocean.grid.vertex->get_size();
			vkCmdPipelineBarrier(cmd_buff, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &memory_barrier, 0, nullptr);
		}

		VK_CHECK(vkEndCommandBuffer(cmd_buff));
	}
}

void SubgroupsOperations::draw()
{
	VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	// Submit compute commands
	VkSubmitInfo compute_submit_info         = vkb::initializers::submit_info();
	compute_submit_info.commandBufferCount   = 1u;
	compute_submit_info.pCommandBuffers      = &compute.command_buffer;
	compute_submit_info.waitSemaphoreCount   = 1u;
	compute_submit_info.pWaitSemaphores      = &ocean.semaphore;
	compute_submit_info.pWaitDstStageMask    = &wait_stage_mask;
	compute_submit_info.signalSemaphoreCount = 1u;
	compute_submit_info.pSignalSemaphores    = &compute.semaphore;

	VK_CHECK(vkQueueSubmit(compute.queue, 1u, &compute_submit_info, VK_NULL_HANDLE));

	VkPipelineStageFlags graphics_wait_stage_masks[]  = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore          graphics_wait_semaphores[]   = {compute.semaphore, semaphores.acquired_image_ready};
	VkSemaphore          graphics_signal_semaphores[] = {ocean.semaphore, semaphores.render_complete};

	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount   = 1u;
	submit_info.pCommandBuffers      = &draw_cmd_buffers[current_buffer];
	submit_info.waitSemaphoreCount   = 2u;
	submit_info.pWaitSemaphores      = graphics_wait_semaphores;
	submit_info.pWaitDstStageMask    = graphics_wait_stage_masks;
	submit_info.signalSemaphoreCount = 2u;
	submit_info.pSignalSemaphores    = graphics_signal_semaphores;

	VK_CHECK(vkQueueSubmit(queue, 1u, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void SubgroupsOperations::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (get_device().get_gpu().get_features().fillModeNonSolid)
		{
			if (drawer.checkbox("Wireframe", &ui.wireframe))
			{
				build_command_buffers();
			}
		}
	}
}

bool SubgroupsOperations::resize(const uint32_t width, const uint32_t height)
{
	if (!ApiVulkanSample::resize(width, height))
		return false;
	update_uniform_buffers();
	build_compute_command_buffer();
	build_command_buffers();
	return true;
}

void SubgroupsOperations::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	if (!timer.is_running())
		timer.start();

	update_uniform_buffers();
	draw();

	auto elapsed_time = static_cast<float>(timer.elapsed<vkb::Timer::Seconds>());

	if (elapsed_time >= 10.0f)
		timer.lap();
}

std::unique_ptr<vkb::VulkanSample> create_subgroups_operations()
{
	return std::make_unique<SubgroupsOperations>();
}
