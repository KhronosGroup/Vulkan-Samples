/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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
  * Compute shader N-body simulation using two passes and shared compute shader memory, using vulkan.hpp
  */

#include <hpp_compute_nbody.h>

#include <benchmark_mode/benchmark_mode.h>
#include <random>

HPPComputeNBody::HPPComputeNBody()
{
	title       = "Compute shader N-body system";
	camera.type = vkb::CameraType::LookAt;

	// Note: Using Reversed depth-buffer for increased precision, so Z-Near and Z-Far are flipped
	camera.set_perspective(60.0f, (float) extent.width / (float) extent.height, 512.0f, 0.1f);
	camera.set_rotation(glm::vec3(-26.0f, 75.0f, 0.0f));
	camera.set_translation(glm::vec3(0.0f, 0.0f, -14.0f));
	camera.translation_speed = 2.5f;
}

HPPComputeNBody::~HPPComputeNBody()
{
	if (get_device() && get_device()->get_handle())
	{
		vk::Device device = get_device()->get_handle();

		// Graphics
		graphics.uniform_buffer.reset();
		device.destroyPipeline(graphics.pipeline);
		device.destroyPipelineLayout(graphics.pipeline_layout);
		device.destroyDescriptorSetLayout(graphics.descriptor_set_layout);
		device.destroySemaphore(graphics.semaphore);

		// Compute
		compute.storage_buffer.reset();
		compute.uniform_buffer.reset();
		device.destroyPipelineLayout(compute.pipeline_layout);
		device.destroyDescriptorSetLayout(compute.descriptor_set_layout);
		device.destroyPipeline(compute.pipeline_calculate);
		device.destroyPipeline(compute.pipeline_integrate);
		device.destroySemaphore(compute.semaphore);
		device.destroyCommandPool(compute.command_pool);

		device.destroySampler(textures.particle.sampler);
		device.destroySampler(textures.gradient.sampler);
	}
}

void HPPComputeNBody::request_gpu_features(vkb::core::HPPPhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

void HPPComputeNBody::load_assets()
{
	textures.particle = load_texture("textures/particle_rgba.ktx", vkb::sg::Image::Color);
	textures.gradient = load_texture("textures/particle_gradient_rgba.ktx", vkb::sg::Image::Color);
}

void HPPComputeNBody::build_command_buffers()
{
	// Destroy command buffers if already present
	if (!check_command_buffers())
	{
		destroy_command_buffers();
		create_command_buffers();
	}

	std::array<vk::ClearValue, 2> clear_values =
	    {{vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 1.0f}})),
	      vk::ClearDepthStencilValue(0.0f, 0)}};

	vk::RenderPassBeginInfo render_pass_begin_info(render_pass, {}, {{0, 0}, extent}, clear_values);

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		// Set target frame buffer
		render_pass_begin_info.framebuffer = framebuffers[i];

		draw_cmd_buffers[i].begin(vk::CommandBufferBeginInfo());

		// Acquire
		if (graphics.queue_family_index != compute.queue_family_index)
		{
			vk::BufferMemoryBarrier buffer_barrier({},
			                                       vk::AccessFlagBits::eVertexAttributeRead,
			                                       compute.queue_family_index,
			                                       graphics.queue_family_index,
			                                       compute.storage_buffer->get_handle(),
			                                       0,
			                                       compute.storage_buffer->get_size());

			draw_cmd_buffers[i].pipelineBarrier(
			    vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eVertexInput, {}, nullptr, buffer_barrier, nullptr);
		}

		// Draw the particle system using the update vertex buffer
		draw_cmd_buffers[i].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
		draw_cmd_buffers[i].setViewport(0, {{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f}});
		draw_cmd_buffers[i].setScissor(0, {{{0, 0}, extent}});
		draw_cmd_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphics.pipeline);
		draw_cmd_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphics.pipeline_layout, 0, graphics.descriptor_set, nullptr);
		draw_cmd_buffers[i].bindVertexBuffers(0, compute.storage_buffer->get_handle(), {0});
		draw_cmd_buffers[i].draw(num_particles, 1, 0, 0);
		draw_ui(draw_cmd_buffers[i]);
		draw_cmd_buffers[i].endRenderPass();

		// Release barrier
		if (graphics.queue_family_index != compute.queue_family_index)
		{
			vk::BufferMemoryBarrier buffer_barrier(vk::AccessFlagBits::eVertexAttributeRead,
			                                       {},
			                                       graphics.queue_family_index,
			                                       compute.queue_family_index,
			                                       compute.storage_buffer->get_handle(),
			                                       0,
			                                       compute.storage_buffer->get_size());

			draw_cmd_buffers[i].pipelineBarrier(
			    vk::PipelineStageFlagBits::eVertexInput, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, buffer_barrier, nullptr);
		}

		draw_cmd_buffers[i].end();
	}
}

void HPPComputeNBody::build_compute_command_buffer()
{
	compute.command_buffer.begin(vk::CommandBufferBeginInfo());

	// Acquire
	if (graphics.queue_family_index != compute.queue_family_index)
	{
		vk::BufferMemoryBarrier buffer_barrier({},
		                                       vk::AccessFlagBits::eShaderWrite,
		                                       graphics.queue_family_index,
		                                       compute.queue_family_index,
		                                       compute.storage_buffer->get_handle(),
		                                       0,
		                                       compute.storage_buffer->get_size());

		compute.command_buffer.pipelineBarrier(
		    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, buffer_barrier, nullptr);
	}

	// First pass: Calculate particle movement
	// -------------------------------------------------------------------------------------------------------
	compute.command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, compute.pipeline_calculate);
	compute.command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, compute.pipeline_layout, 0, compute.descriptor_set, nullptr);
	compute.command_buffer.dispatch(num_particles / work_group_size, 1, 1);

	// Add memory barrier to ensure that the computer shader has finished writing to the buffer
	vk::BufferMemoryBarrier memory_barrier(vk::AccessFlagBits::eShaderWrite,
	                                       vk::AccessFlagBits::eShaderRead,
	                                       VK_QUEUE_FAMILY_IGNORED,
	                                       VK_QUEUE_FAMILY_IGNORED,
	                                       compute.storage_buffer->get_handle(),
	                                       0,
	                                       compute.storage_buffer->get_size());

	compute.command_buffer.pipelineBarrier(
	    vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, memory_barrier, nullptr);

	// Second pass: Integrate particles
	// -------------------------------------------------------------------------------------------------------
	compute.command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, compute.pipeline_integrate);
	compute.command_buffer.dispatch(num_particles / work_group_size, 1, 1);

	// Release
	if (graphics.queue_family_index != compute.queue_family_index)
	{
		vk::BufferMemoryBarrier buffer_barrier(vk::AccessFlagBits::eShaderWrite,
		                                       {},
		                                       compute.queue_family_index,
		                                       graphics.queue_family_index,
		                                       compute.storage_buffer->get_handle(),
		                                       0,
		                                       compute.storage_buffer->get_size());

		compute.command_buffer.pipelineBarrier(
		    vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, buffer_barrier, nullptr);
	}

	compute.command_buffer.end();
}

// Setup and fill the compute shader storage buffers containing the particles
void HPPComputeNBody::prepare_storage_buffers()
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

	std::default_random_engine      rnd_engine(platform->using_plugin<::plugins::BenchmarkMode>() ? 0 : (unsigned) time(nullptr));
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
			particle.vel.w = (float) i * 1.0f / static_cast<uint32_t>(attractors.size());
		}
	}

	compute.ubo.particle_count = num_particles;

	vk::DeviceSize storage_buffer_size = particle_buffer.size() * sizeof(Particle);

	// Staging
	// SSBO won't be changed on the host after upload so copy to device local memory
	vkb::core::HPPBuffer staging_buffer(*get_device(), storage_buffer_size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
	staging_buffer.update(particle_buffer.data(), storage_buffer_size);

	compute.storage_buffer = std::make_unique<vkb::core::HPPBuffer>(*get_device(),
	                                                                storage_buffer_size,
	                                                                vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
	                                                                    vk::BufferUsageFlagBits::eTransferDst,
	                                                                VMA_MEMORY_USAGE_GPU_ONLY);

	// Copy from staging buffer to storage buffer
	vk::CommandBuffer copy_command = device->create_command_buffer(vk::CommandBufferLevel::ePrimary, true);
	copy_command.copyBuffer(staging_buffer.get_handle(), compute.storage_buffer->get_handle(), {{0, 0, storage_buffer_size}});
	// Execute a transfer to the compute queue, if necessary
	if (graphics.queue_family_index != compute.queue_family_index)
	{
		vk::BufferMemoryBarrier buffer_barrier(vk::AccessFlagBits::eVertexAttributeRead,
		                                       {},
		                                       graphics.queue_family_index,
		                                       compute.queue_family_index,
		                                       compute.storage_buffer->get_handle(),
		                                       0,
		                                       compute.storage_buffer->get_size());

		copy_command.pipelineBarrier(vk::PipelineStageFlagBits::eVertexInput, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, buffer_barrier, nullptr);
	}

	device->flush_command_buffer(copy_command, queue, true);
}

void HPPComputeNBody::setup_descriptor_pool()
{
	std::array<vk::DescriptorPoolSize, 3> pool_sizes = {
	    {{vk::DescriptorType::eUniformBuffer, 2}, {vk::DescriptorType::eStorageBuffer, 1}, {vk::DescriptorType::eCombinedImageSampler, 2}}};

	descriptor_pool = get_device()->get_handle().createDescriptorPool({{}, 2, pool_sizes});
}

void HPPComputeNBody::setup_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 3> set_layout_bindings = {
	    {{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	     {2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}}};

	graphics.descriptor_set_layout = get_device()->get_handle().createDescriptorSetLayout({{}, set_layout_bindings});

#if defined(ANDROID)
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, 1, &graphics.descriptor_set_layout);
#else
	vk::PipelineLayoutCreateInfo  pipeline_layout_create_info({}, graphics.descriptor_set_layout);
#endif

	graphics.pipeline_layout = get_device()->get_handle().createPipelineLayout(pipeline_layout_create_info);
}

void HPPComputeNBody::setup_descriptor_set()
{
#if defined(ANDROID)
	vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptor_pool, 1, &graphics.descriptor_set_layout);
#else
	vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptor_pool, graphics.descriptor_set_layout);
#endif
	graphics.descriptor_set = get_device()->get_handle().allocateDescriptorSets(descriptor_set_allocate_info).front();

	vk::DescriptorBufferInfo buffer_descriptor(graphics.uniform_buffer->get_handle(), 0, VK_WHOLE_SIZE);
	vk::DescriptorImageInfo  particle_image_descriptor(
        textures.particle.sampler,
        textures.particle.image->get_vk_image_view().get_handle(),
        descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, textures.particle.image->get_vk_image_view().get_format()));
	vk::DescriptorImageInfo gradient_image_descriptor(
	    textures.gradient.sampler,
	    textures.gradient.image->get_vk_image_view().get_handle(),
	    descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, textures.gradient.image->get_vk_image_view().get_format()));

	std::array<vk::WriteDescriptorSet, 3> write_descriptor_sets = {
	    {{graphics.descriptor_set, 0, 0, vk::DescriptorType::eCombinedImageSampler, particle_image_descriptor},
	     {graphics.descriptor_set, 1, 0, vk::DescriptorType::eCombinedImageSampler, gradient_image_descriptor},
	     {graphics.descriptor_set, 2, 0, vk::DescriptorType::eUniformBuffer, {}, buffer_descriptor}}};
	get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, nullptr);
}

void HPPComputeNBody::prepare_pipelines()
{
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state({}, vk::PrimitiveTopology::ePointList);

	vk::PipelineRasterizationStateCreateInfo rasterization_state;
	rasterization_state.polygonMode = vk::PolygonMode::eFill;
	rasterization_state.cullMode    = vk::CullModeFlagBits::eNone;
	rasterization_state.frontFace   = vk::FrontFace::eCounterClockwise;
	rasterization_state.lineWidth   = 1.0f;

	// Additive blending
	vk::PipelineColorBlendAttachmentState blend_attachment_state(true,
	                                                             vk::BlendFactor::eOne,
	                                                             vk::BlendFactor::eOne,
	                                                             vk::BlendOp::eAdd,
	                                                             vk::BlendFactor::eSrcAlpha,
	                                                             vk::BlendFactor::eDstAlpha,
	                                                             vk::BlendOp::eAdd,
	                                                             vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                                                 vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

	vk::PipelineColorBlendStateCreateInfo color_blend_state({}, {}, {}, blend_attachment_state);

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthTestEnable  = false;
	depth_stencil_state.depthWriteEnable = false;
	depth_stencil_state.depthCompareOp   = vk::CompareOp::eAlways;
	depth_stencil_state.back.compareOp   = vk::CompareOp::eAlways;

	vk::PipelineViewportStateCreateInfo viewport_state({}, 1, nullptr, 1, nullptr);

	vk::PipelineMultisampleStateCreateInfo multisample_state({}, vk::SampleCountFlagBits::e1);

	std::array<vk::DynamicState, 2>    dynamic_state_enables = {{vk::DynamicState::eViewport, vk::DynamicState::eScissor}};
	vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_state_enables);

	// Rendering pipeline
	// Load shaders
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages = {{load_shader("compute_nbody/particle.vert", vk::ShaderStageFlagBits::eVertex),
	                                                                   load_shader("compute_nbody/particle.frag", vk::ShaderStageFlagBits::eFragment)}};

	// Vertex bindings and attributes
	vk::VertexInputBindingDescription                  vertex_input_bindings(0, sizeof(Particle), vk::VertexInputRate::eVertex);
	std::array<vk::VertexInputAttributeDescription, 2> vertex_input_attributes = {
	    {{0, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Particle, pos)},          // Location 0 : Position
	     {1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Particle, vel)}}};        // Location 1 : Velocity
	vk::PipelineVertexInputStateCreateInfo vertex_input_state({}, vertex_input_bindings, vertex_input_attributes);

	vk::GraphicsPipelineCreateInfo pipeline_create_info({},
	                                                    shader_stages,
	                                                    &vertex_input_state,
	                                                    &input_assembly_state,
	                                                    {},
	                                                    &viewport_state,
	                                                    &rasterization_state,
	                                                    &multisample_state,
	                                                    &depth_stencil_state,
	                                                    &color_blend_state,
	                                                    &dynamic_state,
	                                                    graphics.pipeline_layout,
	                                                    render_pass,
	                                                    {},
	                                                    {},
	                                                    -1);

	vk::Result result;
	std::tie(result, graphics.pipeline) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);
}

void HPPComputeNBody::prepare_graphics()
{
	prepare_storage_buffers();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_set();

	// Semaphore for compute & graphics sync
	graphics.semaphore = get_device()->get_handle().createSemaphore({});
}

void HPPComputeNBody::prepare_compute()
{
	// Get compute queue
	compute.queue = get_device()->get_handle().getQueue(compute.queue_family_index, 0);

	// Create compute pipeline
	// Compute pipelines are created separate from graphics pipelines even if they use the same queue (family index)

	std::array<vk::DescriptorSetLayoutBinding, 2> set_layout_bindings = {{{0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
	                                                                      {1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute}}};

	compute.descriptor_set_layout = get_device()->get_handle().createDescriptorSetLayout({{}, set_layout_bindings});

#if defined(ANDROID)
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, 1, &compute.descriptor_set_layout);
#else
	vk::PipelineLayoutCreateInfo  pipeline_layout_create_info({}, compute.descriptor_set_layout);
#endif
	compute.pipeline_layout = get_device()->get_handle().createPipelineLayout(pipeline_layout_create_info);

#if defined(ANDROID)
	vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptor_pool, 1, &compute.descriptor_set_layout);
#else
	vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptor_pool, compute.descriptor_set_layout);
#endif
	compute.descriptor_set = get_device()->get_handle().allocateDescriptorSets(descriptor_set_allocate_info).front();

	vk::DescriptorBufferInfo              storage_buffer_descriptor(compute.storage_buffer->get_handle(), 0, VK_WHOLE_SIZE);
	vk::DescriptorBufferInfo              uniform_buffer_descriptor(compute.uniform_buffer->get_handle(), 0, VK_WHOLE_SIZE);
	std::array<vk::WriteDescriptorSet, 2> compute_write_descriptor_sets = {
	    {// Binding 0 : Particle position storage buffer
	     {compute.descriptor_set, 0, {}, vk::DescriptorType::eStorageBuffer, {}, storage_buffer_descriptor},
	     // Binding 1 : Uniform buffer
	     {compute.descriptor_set, 1, {}, vk::DescriptorType::eUniformBuffer, {}, uniform_buffer_descriptor}}};

	get_device()->get_handle().updateDescriptorSets(compute_write_descriptor_sets, nullptr);

	// Create pipelines
	vk::ComputePipelineCreateInfo compute_pipeline_create_info({}, {}, compute.pipeline_layout);

	// 1st pass - Particle movement calculations
	compute_pipeline_create_info.stage = load_shader("compute_nbody/particle_calculate.comp", vk::ShaderStageFlagBits::eCompute);

	// Set some shader parameters via specialization constants
	struct MovementSpecializationData
	{
		uint32_t workgroup_size;
		uint32_t shared_data_size;
		float    gravity;
		float    power;
		float    soften;
	} movement_specialization_data{work_group_size, shared_data_size, 0.002f, 0.75f, 0.05f};

	std::array<vk::SpecializationMapEntry, 5> movement_specialization_map_entries = {
	    {{0, offsetof(MovementSpecializationData, workgroup_size), sizeof(uint32_t)},
	     {1, offsetof(MovementSpecializationData, shared_data_size), sizeof(uint32_t)},
	     {2, offsetof(MovementSpecializationData, gravity), sizeof(float)},
	     {3, offsetof(MovementSpecializationData, power), sizeof(float)},
	     {4, offsetof(MovementSpecializationData, soften), sizeof(float)}}};
	vk::SpecializationInfo specialization_info(static_cast<uint32_t>(movement_specialization_map_entries.size()),
	                                           movement_specialization_map_entries.data(),
	                                           sizeof(movement_specialization_data),
	                                           &movement_specialization_data);

	compute_pipeline_create_info.stage.pSpecializationInfo = &specialization_info;

	compute.pipeline_calculate = get_device()->get_handle().createComputePipeline(pipeline_cache, compute_pipeline_create_info).value;

	// 2nd pass - Particle integration
	compute_pipeline_create_info.stage = load_shader("compute_nbody/particle_integrate.comp", vk::ShaderStageFlagBits::eCompute);

	vk::SpecializationMapEntry integration_specialization_entry(0, 0, sizeof(work_group_size));
	specialization_info = {1, &integration_specialization_entry, sizeof(work_group_size), &work_group_size};

	compute_pipeline_create_info.stage.pSpecializationInfo = &specialization_info;
	compute.pipeline_integrate                             = get_device()->get_handle().createComputePipeline(pipeline_cache, compute_pipeline_create_info).value;

	// Separate command pool as queue family for compute may be different than graphics
	vk::CommandPoolCreateInfo command_pool_create_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
	                                                   get_device()->get_queue_family_index(vk::QueueFlagBits::eCompute));
	compute.command_pool = get_device()->get_handle().createCommandPool(command_pool_create_info);

	// Create a command buffer for compute operations
	vk::CommandBufferAllocateInfo command_buffer_allocate_info(compute.command_pool, vk::CommandBufferLevel::ePrimary, 1);

	compute.command_buffer = get_device()->get_handle().allocateCommandBuffers(command_buffer_allocate_info).front();

	// Semaphore for compute & graphics sync
	compute.semaphore = get_device()->get_handle().createSemaphore({});

	// Signal the semaphore
	vk::SubmitInfo submit_info(nullptr, {}, {}, compute.semaphore);
	queue.submit(submit_info);
	queue.waitIdle();

	// Build a single command buffer containing the compute dispatch commands
	build_compute_command_buffer();

	// If necessary, acquire and immediately release the storage buffer, so that the initial acquire
	// from the graphics command buffers are matched up properly.
	if (graphics.queue_family_index != compute.queue_family_index)
	{
		// Create a transient command buffer for setting up the initial buffer transfer state
		vk::CommandBufferAllocateInfo command_buffer_allocate_info(compute.command_pool, vk::CommandBufferLevel::ePrimary, 1);

		vk::CommandBuffer transfer_command = get_device()->get_handle().allocateCommandBuffers(command_buffer_allocate_info).front();

		transfer_command.begin(vk::CommandBufferBeginInfo());

		vk::BufferMemoryBarrier acquire_buffer_barrier({},
		                                               vk::AccessFlagBits::eShaderWrite,
		                                               graphics.queue_family_index,
		                                               compute.queue_family_index,
		                                               compute.storage_buffer->get_handle(),
		                                               0,
		                                               compute.storage_buffer->get_size());
		transfer_command.pipelineBarrier(
		    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, acquire_buffer_barrier, nullptr);

		vk::BufferMemoryBarrier release_buffer_barrier(vk::AccessFlagBits::eShaderWrite,
		                                               {},
		                                               compute.queue_family_index,
		                                               graphics.queue_family_index,
		                                               compute.storage_buffer->get_handle(),
		                                               0,
		                                               compute.storage_buffer->get_size());
		transfer_command.pipelineBarrier(
		    vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, release_buffer_barrier, nullptr);

		// Copied from Device::flush_command_buffer, which we can't use because it would be
		// working with the wrong command pool
		transfer_command.end();

		// Submit compute commands
		vk::SubmitInfo submit_info(nullptr, {}, transfer_command);

		// Create fence to ensure that the command buffer has finished executing
		vk::Fence fence = get_device()->get_handle().createFence({});
		// Submit to the *compute* queue
		compute.queue.submit(submit_info, fence);
		// Wait for the fence to signal that command buffer has finished executing
		get_device()->get_handle().waitForFences(fence, true, DEFAULT_FENCE_TIMEOUT);
		get_device()->get_handle().destroyFence(fence);

		get_device()->get_handle().freeCommandBuffers(compute.command_pool, transfer_command);
	}
}

// Prepare and initialize uniform buffer containing shader uniforms
void HPPComputeNBody::prepare_uniform_buffers()
{
	// Compute shader uniform buffer block
	compute.uniform_buffer =
	    std::make_unique<vkb::core::HPPBuffer>(*get_device(), sizeof(compute.ubo), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Vertex shader uniform buffer block
	graphics.uniform_buffer =
	    std::make_unique<vkb::core::HPPBuffer>(*get_device(), sizeof(graphics.ubo), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_compute_uniform_buffers(1.0f);
	update_graphics_uniform_buffers();
}

void HPPComputeNBody::update_compute_uniform_buffers(float delta_time)
{
	compute.ubo.delta_time = paused ? 0.0f : delta_time;
	compute.uniform_buffer->convert_and_update(compute.ubo);
}

void HPPComputeNBody::update_graphics_uniform_buffers()
{
	graphics.ubo.projection = camera.matrices.perspective;
	graphics.ubo.view       = camera.matrices.view;
	graphics.ubo.screenDim  = glm::vec2(static_cast<float>(extent.width), static_cast<float>(extent.height));
	graphics.uniform_buffer->convert_and_update(graphics.ubo);
}

void HPPComputeNBody::draw()
{
	HPPApiVulkanSample::prepare_frame();

	std::array<vk::PipelineStageFlags, 2> graphics_wait_stage_masks  = {vk::PipelineStageFlagBits::eVertexInput,
                                                                       vk::PipelineStageFlagBits::eColorAttachmentOutput};
	std::array<vk::Semaphore, 2>          graphics_wait_semaphores   = {compute.semaphore, semaphores.acquired_image_ready};
	std::array<vk::Semaphore, 2>          graphics_signal_semaphores = {graphics.semaphore, semaphores.render_complete};

	// Submit graphics commands
	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);
	submit_info.setWaitSemaphores(graphics_wait_semaphores);
	submit_info.setWaitDstStageMask(graphics_wait_stage_masks);
	submit_info.setSignalSemaphores(graphics_signal_semaphores);
	queue.submit(submit_info);

	HPPApiVulkanSample::submit_frame();

	// Wait for rendering finished
	vk::PipelineStageFlags wait_stage_mask = vk::PipelineStageFlagBits::eComputeShader;

	// Submit compute commands
	vk::SubmitInfo compute_submit_info(graphics.semaphore, wait_stage_mask, compute.command_buffer, compute.semaphore);
	compute.queue.submit(compute_submit_info);
}

bool HPPComputeNBody::prepare(vkb::platform::HPPPlatform &platform)
{
	if (!HPPApiVulkanSample::prepare(platform))
	{
		return false;
	}

	graphics.queue_family_index = get_device()->get_queue_family_index(vk::QueueFlagBits::eGraphics);
	compute.queue_family_index  = get_device()->get_queue_family_index(vk::QueueFlagBits::eCompute);

	// Not all implementations support a work group size of 256, so we need to check with the device limits
	work_group_size = std::min<uint32_t>(256, get_device()->get_gpu().get_properties().limits.maxComputeWorkGroupSize[0]);
	// Same for shared data size for passing data between shader invocations
	shared_data_size = std::min<uint32_t>(1024, get_device()->get_gpu().get_properties().limits.maxComputeSharedMemorySize / sizeof(glm::vec4));

	load_assets();
	setup_descriptor_pool();
	prepare_graphics();
	prepare_compute();
	build_command_buffers();
	prepared = true;
	return true;
}

void HPPComputeNBody::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
	update_compute_uniform_buffers(delta_time);
	if (camera.updated)
	{
		update_graphics_uniform_buffers();
	}
}

bool HPPComputeNBody::resize(const uint32_t width, const uint32_t height)
{
	HPPApiVulkanSample::resize(width, height);
	update_graphics_uniform_buffers();
	return true;
}

std::unique_ptr<vkb::Application> create_hpp_compute_nbody()
{
	return std::make_unique<HPPComputeNBody>();
}
