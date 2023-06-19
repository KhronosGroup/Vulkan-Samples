/* Copyright (c) 2019-2023, Sascha Willems
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
 * Compute shader N-body simulation using two passes and shared compute shader memory
 */

#include "compute_nbody.h"

#include "benchmark_mode/benchmark_mode.h"

ComputeNBody::ComputeNBody()
{
	title       = "Compute shader N-body system";
	camera.type = vkb::CameraType::LookAt;

	// Note: Using reversed depth-buffer for increased precision, so Z-Near and Z-Far are flipped
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 512.0f, 0.1f);
	camera.set_rotation(glm::vec3(-26.0f, 75.0f, 0.0f));
	camera.set_translation(glm::vec3(0.0f, 0.0f, -14.0f));
	camera.translation_speed = 2.5f;
}

ComputeNBody::~ComputeNBody()
{
	if (device)
	{
		// Graphics
		graphics.uniform_buffer.reset();
		vkDestroyPipeline(get_device().get_handle(), graphics.pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), graphics.pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), graphics.descriptor_set_layout, nullptr);
		vkDestroySemaphore(get_device().get_handle(), graphics.semaphore, nullptr);

		// Compute
		compute.storage_buffer.reset();
		compute.uniform_buffer.reset();
		vkDestroyPipelineLayout(get_device().get_handle(), compute.pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), compute.descriptor_set_layout, nullptr);
		vkDestroyPipeline(get_device().get_handle(), compute.pipeline_calculate, nullptr);
		vkDestroyPipeline(get_device().get_handle(), compute.pipeline_integrate, nullptr);
		vkDestroySemaphore(get_device().get_handle(), compute.semaphore, nullptr);
		vkDestroyCommandPool(get_device().get_handle(), compute.command_pool, nullptr);

		vkDestroySampler(get_device().get_handle(), textures.particle.sampler, nullptr);
		vkDestroySampler(get_device().get_handle(), textures.gradient.sampler, nullptr);
	}
}

void ComputeNBody::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

void ComputeNBody::load_assets()
{
	textures.particle = load_texture("textures/particle_rgba.ktx", vkb::sg::Image::Color);
	textures.gradient = load_texture("textures/particle_gradient_rgba.ktx", vkb::sg::Image::Color);
}

void ComputeNBody::build_command_buffers()
{
	// Destroy command buffers if already present
	if (!check_command_buffers())
	{
		destroy_command_buffers();
		create_command_buffers();
	}

	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		// Set target frame buffer
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		// Acquire
		if (graphics.queue_family_index != compute.queue_family_index)
		{
			VkBufferMemoryBarrier buffer_barrier =
			    {
			        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			        nullptr,
			        0,
			        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			        compute.queue_family_index,
			        graphics.queue_family_index,
			        compute.storage_buffer->get_handle(),
			        0,
			        compute.storage_buffer->get_size()};

			vkCmdPipelineBarrier(
			    draw_cmd_buffers[i],
			    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			    0,
			    0, nullptr,
			    1, &buffer_barrier,
			    0, nullptr);
		}

		// Draw the particle system using the update vertex buffer
		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);
		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipeline);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipeline_layout, 0, 1, &graphics.descriptor_set, 0, NULL);
		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, compute.storage_buffer->get(), offsets);
		vkCmdDraw(draw_cmd_buffers[i], num_particles, 1, 0, 0);
		draw_ui(draw_cmd_buffers[i]);
		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		// Release barrier
		if (graphics.queue_family_index != compute.queue_family_index)
		{
			VkBufferMemoryBarrier buffer_barrier =
			    {
			        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			        nullptr,
			        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			        0,
			        graphics.queue_family_index,
			        compute.queue_family_index,
			        compute.storage_buffer->get_handle(),
			        0,
			        compute.storage_buffer->get_size()};

			vkCmdPipelineBarrier(
			    draw_cmd_buffers[i],
			    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			    0,
			    0, nullptr,
			    1, &buffer_barrier,
			    0, nullptr);
		}

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void ComputeNBody::build_compute_command_buffer()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VK_CHECK(vkBeginCommandBuffer(compute.command_buffer, &command_buffer_begin_info));

	// Acquire
	if (graphics.queue_family_index != compute.queue_family_index)
	{
		VkBufferMemoryBarrier buffer_barrier =
		    {
		        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		        nullptr,
		        0,
		        VK_ACCESS_SHADER_WRITE_BIT,
		        graphics.queue_family_index,
		        compute.queue_family_index,
		        compute.storage_buffer->get_handle(),
		        0,
		        compute.storage_buffer->get_size()};

		vkCmdPipelineBarrier(
		    compute.command_buffer,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		    0,
		    0, nullptr,
		    1, &buffer_barrier,
		    0, nullptr);
	}

	// First pass: Calculate particle movement
	// -------------------------------------------------------------------------------------------------------
	vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline_calculate);
	vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline_layout, 0, 1, &compute.descriptor_set, 0, 0);
	vkCmdDispatch(compute.command_buffer, num_particles / work_group_size, 1, 1);

	// Add memory barrier to ensure that the computer shader has finished writing to the buffer
	VkBufferMemoryBarrier memory_barrier = vkb::initializers::buffer_memory_barrier();
	memory_barrier.buffer                = compute.storage_buffer->get_handle();
	memory_barrier.size                  = compute.storage_buffer->get_size();
	memory_barrier.srcAccessMask         = VK_ACCESS_SHADER_WRITE_BIT;
	memory_barrier.dstAccessMask         = VK_ACCESS_SHADER_READ_BIT;
	memory_barrier.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
	memory_barrier.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;

	vkCmdPipelineBarrier(
	    compute.command_buffer,
	    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	    VK_FLAGS_NONE,
	    0, nullptr,
	    1, &memory_barrier,
	    0, nullptr);

	// Second pass: Integrate particles
	// -------------------------------------------------------------------------------------------------------
	vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline_integrate);
	vkCmdDispatch(compute.command_buffer, num_particles / work_group_size, 1, 1);

	// Release
	if (graphics.queue_family_index != compute.queue_family_index)
	{
		VkBufferMemoryBarrier buffer_barrier =
		    {
		        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		        nullptr,
		        VK_ACCESS_SHADER_WRITE_BIT,
		        0,
		        compute.queue_family_index,
		        graphics.queue_family_index,
		        compute.storage_buffer->get_handle(),
		        0,
		        compute.storage_buffer->get_size()};

		vkCmdPipelineBarrier(
		    compute.command_buffer,
		    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    0,
		    0, nullptr,
		    1, &buffer_barrier,
		    0, nullptr);
	}

	vkEndCommandBuffer(compute.command_buffer);
}

// Setup and fill the compute shader storage buffers containing the particles
void ComputeNBody::prepare_storage_buffers()
{
#if 0
	std::vector<glm::vec3> attractors = {
		glm::vec3(2.5f, 1.5f, 0.0f),
		glm::vec3(-2.5f, -1.5f, 0.0f),
	};
#else
	std::vector<glm::vec3> attractors = {
	    glm::vec3(5.0f, 0.0f, 0.0f),
	    glm::vec3(-5.0f, 0.0f, 0.0f),
	    glm::vec3(0.0f, 0.0f, 5.0f),
	    glm::vec3(0.0f, 0.0f, -5.0f),
	    glm::vec3(0.0f, 4.0f, 0.0f),
	    glm::vec3(0.0f, -8.0f, 0.0f),
	};
#endif

	num_particles = static_cast<uint32_t>(attractors.size()) * PARTICLES_PER_ATTRACTOR;

	// Initial particle positions
	std::vector<Particle> particle_buffer(num_particles);

	std::default_random_engine      rnd_engine(lock_simulation_speed ? 0 : static_cast<unsigned>(time(nullptr)));
	std::normal_distribution<float> rnd_distribution(0.0f, 1.0f);

	for (uint32_t i = 0; i < static_cast<uint32_t>(attractors.size()); i++)
	{
		for (uint32_t j = 0; j < PARTICLES_PER_ATTRACTOR; j++)
		{
			Particle &particle = particle_buffer[i * PARTICLES_PER_ATTRACTOR + j];

			// First particle in group as heavy center of gravity
			if (j == 0)
			{
				particle.pos = glm::vec4(attractors[i] * 1.5f, 90000.0f);
				particle.vel = glm::vec4(glm::vec4(0.0f));
			}
			else
			{
				// Position
				glm::vec3 position(attractors[i] + glm::vec3(rnd_distribution(rnd_engine), rnd_distribution(rnd_engine), rnd_distribution(rnd_engine)) * 0.75f);
				float     len = glm::length(glm::normalize(position - attractors[i]));
				position.y *= 2.0f - (len * len);

				// Velocity
				glm::vec3 angular  = glm::vec3(0.5f, 1.5f, 0.5f) * (((i % 2) == 0) ? 1.0f : -1.0f);
				glm::vec3 velocity = glm::cross((position - attractors[i]), angular) + glm::vec3(rnd_distribution(rnd_engine), rnd_distribution(rnd_engine), rnd_distribution(rnd_engine) * 0.025f);

				float mass   = (rnd_distribution(rnd_engine) * 0.5f + 0.5f) * 75.0f;
				particle.pos = glm::vec4(position, mass);
				particle.vel = glm::vec4(velocity, 0.0f);
			}

			// Color gradient offset
			particle.vel.w = static_cast<float>(i) * 1.0f / static_cast<uint32_t>(attractors.size());
		}
	}

	compute.ubo.particle_count = num_particles;

	VkDeviceSize storage_buffer_size = particle_buffer.size() * sizeof(Particle);

	// Staging
	// SSBO won't be changed on the host after upload so copy to device local memory
	vkb::core::Buffer staging_buffer{get_device(), storage_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY};
	staging_buffer.update(particle_buffer.data(), storage_buffer_size);

	compute.storage_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                             storage_buffer_size,
	                                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                             VMA_MEMORY_USAGE_GPU_ONLY);

	// Copy from staging buffer to storage buffer
	VkCommandBuffer copy_command = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkBufferCopy    copy_region  = {};
	copy_region.size             = storage_buffer_size;
	vkCmdCopyBuffer(copy_command, staging_buffer.get_handle(), compute.storage_buffer->get_handle(), 1, &copy_region);
	// Execute a transfer to the compute queue, if necessary
	if (graphics.queue_family_index != compute.queue_family_index)
	{
		VkBufferMemoryBarrier buffer_barrier =
		    {
		        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		        nullptr,
		        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
		        0,
		        graphics.queue_family_index,
		        compute.queue_family_index,
		        compute.storage_buffer->get_handle(),
		        0,
		        compute.storage_buffer->get_size()};

		vkCmdPipelineBarrier(
		    copy_command,
		    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		    0,
		    0, nullptr,
		    1, &buffer_barrier,
		    0, nullptr);
	}

	device->flush_command_buffer(copy_command, queue, true);
}

void ComputeNBody::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes =
	    {
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        2);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void ComputeNBody::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings;
	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 2),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout =
	    vkb::initializers::descriptor_set_layout_create_info(
	        set_layout_bindings.data(),
	        static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &graphics.descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &graphics.descriptor_set_layout,
	        1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &graphics.pipeline_layout));
}

void ComputeNBody::setup_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &graphics.descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &graphics.descriptor_set));

	VkDescriptorBufferInfo            buffer_descriptor         = create_descriptor(*graphics.uniform_buffer);
	VkDescriptorImageInfo             particle_image_descriptor = create_descriptor(textures.particle);
	VkDescriptorImageInfo             gradient_image_descriptor = create_descriptor(textures.gradient);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets;
	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(graphics.descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &particle_image_descriptor),
	    vkb::initializers::write_descriptor_set(graphics.descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &gradient_image_descriptor),
	    vkb::initializers::write_descriptor_set(graphics.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &buffer_descriptor),
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
}

void ComputeNBody::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
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

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_FALSE,
	        VK_FALSE,
	        VK_COMPARE_OP_ALWAYS);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicState =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	// Rendering pipeline
	// Load shaders
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

	shader_stages[0] = load_shader("compute_nbody/particle.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("compute_nbody/particle.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Particle), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, pos)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, vel))};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(
	        graphics.pipeline_layout,
	        render_pass,
	        0);

	pipeline_create_info.pVertexInputState   = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamicState;
	pipeline_create_info.stageCount          = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();
	pipeline_create_info.renderPass          = render_pass;

	// Additive blending
	blend_attachment_state.colorWriteMask      = 0xF;
	blend_attachment_state.blendEnable         = VK_TRUE;
	blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &graphics.pipeline));
}

void ComputeNBody::prepare_graphics()
{
	prepare_storage_buffers();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_set();

	// Semaphore for compute & graphics sync
	VkSemaphoreCreateInfo semaphore_create_info = vkb::initializers::semaphore_create_info();
	VK_CHECK(vkCreateSemaphore(get_device().get_handle(), &semaphore_create_info, nullptr, &graphics.semaphore));
}

void ComputeNBody::prepare_compute()
{
	// Get compute queue
	vkGetDeviceQueue(get_device().get_handle(), compute.queue_family_index, 0, &compute.queue);

	// Create compute pipeline
	// Compute pipelines are created separate from graphics pipelines even if they use the same queue (family index)

	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    // Binding 0 : Particle position storage buffer
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        VK_SHADER_STAGE_COMPUTE_BIT,
	        0),
	    // Binding 1 : Uniform buffer
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_COMPUTE_BIT,
	        1),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout =
	    vkb::initializers::descriptor_set_layout_create_info(
	        set_layout_bindings.data(),
	        static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &compute.descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &compute.descriptor_set_layout,
	        1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &compute.pipeline_layout));

	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &compute.descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &compute.descriptor_set));

	VkDescriptorBufferInfo            storage_buffer_descriptor = create_descriptor(*compute.storage_buffer);
	VkDescriptorBufferInfo            uniform_buffer_descriptor = create_descriptor(*compute.uniform_buffer);
	std::vector<VkWriteDescriptorSet> compute_write_descriptor_sets =
	    {
	        // Binding 0 : Particle position storage buffer
	        vkb::initializers::write_descriptor_set(
	            compute.descriptor_set,
	            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	            0,
	            &storage_buffer_descriptor),
	        // Binding 1 : Uniform buffer
	        vkb::initializers::write_descriptor_set(
	            compute.descriptor_set,
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            1,
	            &uniform_buffer_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(compute_write_descriptor_sets.size()), compute_write_descriptor_sets.data(), 0, NULL);

	// Create pipelines
	VkComputePipelineCreateInfo compute_pipeline_create_info = vkb::initializers::compute_pipeline_create_info(compute.pipeline_layout, 0);

	// 1st pass - Particle movement calculations
	compute_pipeline_create_info.stage = load_shader("compute_nbody/particle_calculate.comp", VK_SHADER_STAGE_COMPUTE_BIT);

	// Set some shader parameters via specialization constants
	struct SpecializationData
	{
		uint32_t workgroup_size;
		uint32_t shared_data_size;
		float    gravity;
		float    power;
		float    soften;
	} specialization_data;

	std::vector<VkSpecializationMapEntry> specialization_map_entries;
	specialization_map_entries.push_back(vkb::initializers::specialization_map_entry(0, offsetof(SpecializationData, workgroup_size), sizeof(uint32_t)));
	specialization_map_entries.push_back(vkb::initializers::specialization_map_entry(1, offsetof(SpecializationData, shared_data_size), sizeof(uint32_t)));
	specialization_map_entries.push_back(vkb::initializers::specialization_map_entry(2, offsetof(SpecializationData, gravity), sizeof(float)));
	specialization_map_entries.push_back(vkb::initializers::specialization_map_entry(3, offsetof(SpecializationData, power), sizeof(float)));
	specialization_map_entries.push_back(vkb::initializers::specialization_map_entry(4, offsetof(SpecializationData, soften), sizeof(float)));

	specialization_data.workgroup_size   = work_group_size;
	specialization_data.shared_data_size = shared_data_size;
	specialization_data.gravity          = 0.002f;
	specialization_data.power            = 0.75f;
	specialization_data.soften           = 0.05f;

	VkSpecializationInfo specialization_info =
	    vkb::initializers::specialization_info(static_cast<uint32_t>(specialization_map_entries.size()), specialization_map_entries.data(), sizeof(specialization_data), &specialization_data);
	compute_pipeline_create_info.stage.pSpecializationInfo = &specialization_info;

	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1, &compute_pipeline_create_info, nullptr, &compute.pipeline_calculate));

	// 2nd pass - Particle integration
	compute_pipeline_create_info.stage = load_shader("compute_nbody/particle_integrate.comp", VK_SHADER_STAGE_COMPUTE_BIT);

	specialization_map_entries.clear();
	specialization_map_entries.push_back(vkb::initializers::specialization_map_entry(0, 0, sizeof(uint32_t)));
	specialization_info =
	    vkb::initializers::specialization_info(1, specialization_map_entries.data(), sizeof(work_group_size), &work_group_size);

	compute_pipeline_create_info.stage.pSpecializationInfo = &specialization_info;
	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1, &compute_pipeline_create_info, nullptr, &compute.pipeline_integrate));

	// Separate command pool as queue family for compute may be different than graphics
	VkCommandPoolCreateInfo command_pool_create_info = {};
	command_pool_create_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.queueFamilyIndex        = get_device().get_queue_family_index(VK_QUEUE_COMPUTE_BIT);
	command_pool_create_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_CHECK(vkCreateCommandPool(get_device().get_handle(), &command_pool_create_info, nullptr, &compute.command_pool));

	// Create a command buffer for compute operations
	VkCommandBufferAllocateInfo command_buffer_allocate_info =
	    vkb::initializers::command_buffer_allocate_info(
	        compute.command_pool,
	        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	        1);

	VK_CHECK(vkAllocateCommandBuffers(get_device().get_handle(), &command_buffer_allocate_info, &compute.command_buffer));

	// Semaphore for compute & graphics sync
	VkSemaphoreCreateInfo semaphore_create_info = vkb::initializers::semaphore_create_info();
	VK_CHECK(vkCreateSemaphore(get_device().get_handle(), &semaphore_create_info, nullptr, &compute.semaphore));

	// Signal the semaphore
	VkSubmitInfo submit_info         = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = &compute.semaphore;
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	VK_CHECK(vkQueueWaitIdle(queue));

	// Build a single command buffer containing the compute dispatch commands
	build_compute_command_buffer();

	// If necessary, acquire and immediately release the storage buffer, so that the initial acquire
	// from the graphics command buffers are matched up properly.
	if (graphics.queue_family_index != compute.queue_family_index)
	{
		VkCommandBuffer transfer_command;

		// Create a transient command buffer for setting up the initial buffer transfer state
		VkCommandBufferAllocateInfo command_buffer_allocate_info =
		    vkb::initializers::command_buffer_allocate_info(
		        compute.command_pool,
		        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		        1);

		VK_CHECK(vkAllocateCommandBuffers(get_device().get_handle(), &command_buffer_allocate_info, &transfer_command));

		VkCommandBufferBeginInfo command_buffer_info{};
		command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		VK_CHECK(vkBeginCommandBuffer(transfer_command, &command_buffer_info));

		VkBufferMemoryBarrier acquire_buffer_barrier =
		    {
		        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		        nullptr,
		        0,
		        VK_ACCESS_SHADER_WRITE_BIT,
		        graphics.queue_family_index,
		        compute.queue_family_index,
		        compute.storage_buffer->get_handle(),
		        0,
		        compute.storage_buffer->get_size()};
		vkCmdPipelineBarrier(
		    transfer_command,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		    0,
		    0, nullptr,
		    1, &acquire_buffer_barrier,
		    0, nullptr);

		VkBufferMemoryBarrier release_buffer_barrier =
		    {
		        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		        nullptr,
		        VK_ACCESS_SHADER_WRITE_BIT,
		        0,
		        compute.queue_family_index,
		        graphics.queue_family_index,
		        compute.storage_buffer->get_handle(),
		        0,
		        compute.storage_buffer->get_size()};
		vkCmdPipelineBarrier(
		    transfer_command,
		    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    0,
		    0, nullptr,
		    1, &release_buffer_barrier,
		    0, nullptr);

		// Copied from Device::flush_command_buffer, which we can't use because it would be
		// working with the wrong command pool
		VK_CHECK(vkEndCommandBuffer(transfer_command));

		// Submit compute commands
		VkSubmitInfo submit_info{};
		submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers    = &transfer_command;

		// Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FLAGS_NONE;

		VkFence fence;
		VK_CHECK(vkCreateFence(device->get_handle(), &fence_info, nullptr, &fence));
		// Submit to the *compute* queue
		VkResult result = vkQueueSubmit(compute.queue, 1, &submit_info, fence);
		// Wait for the fence to signal that command buffer has finished executing
		VK_CHECK(vkWaitForFences(device->get_handle(), 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
		vkDestroyFence(device->get_handle(), fence, nullptr);

		vkFreeCommandBuffers(device->get_handle(), compute.command_pool, 1, &transfer_command);
	}
}

// Prepare and initialize uniform buffer containing shader uniforms
void ComputeNBody::prepare_uniform_buffers()
{
	// Compute shader uniform buffer block
	compute.uniform_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                             sizeof(compute.ubo),
	                                                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                             VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Vertex shader uniform buffer block
	graphics.uniform_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                              sizeof(graphics.ubo),
	                                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                              VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_compute_uniform_buffers(1.0f);
	update_graphics_uniform_buffers();
}

void ComputeNBody::update_compute_uniform_buffers(float delta_time)
{
	compute.ubo.delta_time = paused ? 0.0f : delta_time;
	compute.uniform_buffer->convert_and_update(compute.ubo);
}

void ComputeNBody::update_graphics_uniform_buffers()
{
	graphics.ubo.projection = camera.matrices.perspective;
	graphics.ubo.view       = camera.matrices.view;
	graphics.ubo.screenDim  = glm::vec2(static_cast<float>(width), static_cast<float>(height));
	graphics.uniform_buffer->convert_and_update(graphics.ubo);
}

void ComputeNBody::draw()
{
	ApiVulkanSample::prepare_frame();

	VkPipelineStageFlags graphics_wait_stage_masks[]  = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore          graphics_wait_semaphores[]   = {compute.semaphore, semaphores.acquired_image_ready};
	VkSemaphore          graphics_signal_semaphores[] = {graphics.semaphore, semaphores.render_complete};

	// Submit graphics commands
	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &draw_cmd_buffers[current_buffer];
	submit_info.waitSemaphoreCount   = 2;
	submit_info.pWaitSemaphores      = graphics_wait_semaphores;
	submit_info.pWaitDstStageMask    = graphics_wait_stage_masks;
	submit_info.signalSemaphoreCount = 2;
	submit_info.pSignalSemaphores    = graphics_signal_semaphores;
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();

	// Wait for rendering finished
	VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

	// Submit compute commands
	VkSubmitInfo compute_submit_info         = vkb::initializers::submit_info();
	compute_submit_info.commandBufferCount   = 1;
	compute_submit_info.pCommandBuffers      = &compute.command_buffer;
	compute_submit_info.waitSemaphoreCount   = 1;
	compute_submit_info.pWaitSemaphores      = &graphics.semaphore;
	compute_submit_info.pWaitDstStageMask    = &wait_stage_mask;
	compute_submit_info.signalSemaphoreCount = 1;
	compute_submit_info.pSignalSemaphores    = &compute.semaphore;
	VK_CHECK(vkQueueSubmit(compute.queue, 1, &compute_submit_info, VK_NULL_HANDLE));
}

bool ComputeNBody::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	graphics.queue_family_index = get_device().get_queue_family_index(VK_QUEUE_GRAPHICS_BIT);
	compute.queue_family_index  = get_device().get_queue_family_index(VK_QUEUE_COMPUTE_BIT);

	// Not all implementations support a work group size of 256, so we need to check with the device limits
	work_group_size = std::min(static_cast<uint32_t>(256), get_device().get_gpu().get_properties().limits.maxComputeWorkGroupSize[0]);
	// Same for shared data size for passing data between shader invocations
	shared_data_size = std::min(static_cast<uint32_t>(1024), static_cast<uint32_t>(get_device().get_gpu().get_properties().limits.maxComputeSharedMemorySize / sizeof(glm::vec4)));

	load_assets();
	setup_descriptor_pool();
	prepare_graphics();
	prepare_compute();
	build_command_buffers();
	prepared = true;
	return true;
}

void ComputeNBody::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	update_compute_uniform_buffers(delta_time);
	if (camera.updated)
	{
		update_graphics_uniform_buffers();
	}
}

bool ComputeNBody::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_graphics_uniform_buffers();
	return true;
}

std::unique_ptr<vkb::Application> create_compute_nbody()
{
	return std::make_unique<ComputeNBody>();
}
