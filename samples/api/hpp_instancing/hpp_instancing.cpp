/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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
 * Instanced mesh rendering, uses a separate vertex buffer for instanced data, using vulkan.hpp
 */

#include "hpp_instancing.h"

#include <benchmark_mode/benchmark_mode.h>
#include <random>

HPPInstancing::HPPInstancing()
{
	title = "HPP instanced mesh rendering";
}

HPPInstancing::~HPPInstancing()
{
	if (get_device() && get_device()->get_handle())
	{
		vk::Device device = get_device()->get_handle();

		device.destroyPipeline(pipelines.instanced_rocks);
		device.destroyPipeline(pipelines.planet);
		device.destroyPipeline(pipelines.starfield);
		device.destroyPipelineLayout(pipeline_layout);
		device.destroyDescriptorSetLayout(descriptor_set_layout);
		device.destroyBuffer(instance_buffer.buffer);
		device.freeMemory(instance_buffer.memory);
		device.destroySampler(textures.rocks.sampler);
		device.destroySampler(textures.planet.sampler);
	}
}

void HPPInstancing::request_gpu_features(vkb::core::HPPPhysicalDevice &gpu)
{
	auto       &requested_features = gpu.get_mutable_requested_features();
	auto const &features           = gpu.get_features();

	// Enable anisotropic filtering if supported
	if (features.samplerAnisotropy)
	{
		requested_features.samplerAnisotropy = true;
	}
	// Enable texture compression
	if (features.textureCompressionBC)
	{
		requested_features.textureCompressionBC = true;
	}
	else if (features.textureCompressionASTC_LDR)
	{
		requested_features.textureCompressionASTC_LDR = true;
	}
	else if (features.textureCompressionETC2)
	{
		requested_features.textureCompressionETC2 = true;
	}
};

void HPPInstancing::build_command_buffers()
{
	vk::CommandBufferBeginInfo command_buffer_begin_info;

	std::array<vk::ClearValue, 2> clear_values =
	    {{vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.033f, 0.0f}})),
	      vk::ClearDepthStencilValue(0.0f, 0)}};

	vk::RenderPassBeginInfo render_pass_begin_info(render_pass, {}, {{0, 0}, extent}, clear_values);

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		// Set target frame buffer
		render_pass_begin_info.framebuffer = framebuffers[i];

		auto command_buffer = draw_cmd_buffers[i];
		command_buffer.begin(command_buffer_begin_info);

		command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

		vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
		command_buffer.setViewport(0, viewport);

		vk::Rect2D scissor({0, 0}, extent);
		command_buffer.setScissor(0, scissor);

		vk::DeviceSize offset = 0;

		// Star field
		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_sets.planet, {});
		command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.starfield);
		command_buffer.draw(4, 1, 0, 0);

		// Planet
		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_sets.planet, {});
		command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.planet);
		command_buffer.bindVertexBuffers(0, models.planet->get_vertex_buffer("vertex_buffer").get_handle(), offset);
		command_buffer.bindIndexBuffer(models.planet->get_index_buffer().get_handle(), 0, vk::IndexType::eUint32);
		command_buffer.drawIndexed(models.planet->vertex_indices, 1, 0, 0, 0);

		// Instanced rocks
		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_sets.instanced_rocks, {});
		command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.instanced_rocks);
		// Binding point 0 : Mesh vertex buffer
		command_buffer.bindVertexBuffers(0, models.rock->get_vertex_buffer("vertex_buffer").get_handle(), offset);
		// Binding point 1 : Instance data buffer
		command_buffer.bindVertexBuffers(1, instance_buffer.buffer, offset);
		command_buffer.bindIndexBuffer(models.rock->get_index_buffer().get_handle(), 0, vk::IndexType::eUint32);
		// Render instances
		command_buffer.drawIndexed(models.rock->vertex_indices, INSTANCE_COUNT, 0, 0, 0);

		draw_ui(command_buffer);

		command_buffer.endRenderPass();

		command_buffer.end();
	}
}

void HPPInstancing::load_assets()
{
	models.rock   = load_model("scenes/rock.gltf");
	models.planet = load_model("scenes/planet.gltf");

	textures.rocks  = load_texture_array("textures/texturearray_rocks_color_rgba.ktx", vkb::sg::Image::Color);
	textures.planet = load_texture("textures/lavaplanet_color_rgba.ktx", vkb::sg::Image::Color);
}

void HPPInstancing::setup_descriptor_pool()
{
	// Example uses one ubo
	std::array<vk::DescriptorPoolSize, 2> pool_sizes = {{{vk::DescriptorType::eUniformBuffer, 2}, {vk::DescriptorType::eCombinedImageSampler, 2}}};

	vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 2, pool_sizes);
	descriptor_pool = get_device()->get_handle().createDescriptorPool(descriptor_pool_create_info);
}

void HPPInstancing::setup_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 2> set_layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},                   // Binding 0 : Vertex shader uniform buffer
	     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};        // Binding 1 : Fragment shader combined sampler

	vk::DescriptorSetLayoutCreateInfo descriptor_layout_create_info({}, set_layout_bindings);

	descriptor_set_layout = get_device()->get_handle().createDescriptorSetLayout(descriptor_layout_create_info);

#if defined(ANDROID)
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, 1, &descriptor_set_layout);
#else
	vk::PipelineLayoutCreateInfo  pipeline_layout_create_info({}, descriptor_set_layout);
#endif

	pipeline_layout = get_device()->get_handle().createPipelineLayout(pipeline_layout_create_info);
}

void HPPInstancing::setup_descriptor_set()
{
#if defined(ANDROID)
	vk::DescriptorSetAllocateInfo descriptor_set_alloc_info(descriptor_pool, 1, &descriptor_set_layout);
#else
	vk::DescriptorSetAllocateInfo descriptor_set_alloc_info(descriptor_pool, descriptor_set_layout);
#endif

	// Instanced rocks
	descriptor_sets.instanced_rocks = get_device()->get_handle().allocateDescriptorSets(descriptor_set_alloc_info).front();
	vk::DescriptorBufferInfo buffer_descriptor(uniform_buffers.scene->get_handle(), 0, VK_WHOLE_SIZE);
	vk::DescriptorImageInfo  image_descriptor(
        textures.rocks.sampler,
        textures.rocks.image->get_vk_image_view().get_handle(),
        descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, textures.rocks.image->get_vk_image_view().get_format()));
	std::array<vk::WriteDescriptorSet, 2> write_descriptor_sets = {
	    {{descriptor_sets.instanced_rocks, 0, 0, vk::DescriptorType::eUniformBuffer, {}, buffer_descriptor},            // Binding 0 : Vertex shader uniform buffer
	     {descriptor_sets.instanced_rocks, 1, 0, vk::DescriptorType::eCombinedImageSampler, image_descriptor}}};        // Binding 1 : Color map
	get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});

	// Planet
	descriptor_sets.planet = get_device()->get_handle().allocateDescriptorSets(descriptor_set_alloc_info).front();
	image_descriptor       = {textures.planet.sampler,
	                          textures.planet.image->get_vk_image_view().get_handle(),
	                          descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, textures.planet.image->get_vk_image_view().get_format())};
	write_descriptor_sets  = {
        {{descriptor_sets.planet, 0, 0, vk::DescriptorType::eUniformBuffer, {}, buffer_descriptor},            // Binding 0 : Vertex shader uniform buffer
	      {descriptor_sets.planet, 1, 0, vk::DescriptorType::eCombinedImageSampler, image_descriptor}}};        // Binding 1 : Color map
	get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});
}

void HPPInstancing::prepare_pipelines()
{
	// Load shaders: Instancing pipeline
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages{
	    load_shader("instancing/instancing.vert", vk::ShaderStageFlagBits::eVertex),
	    load_shader("instancing/instancing.frag", vk::ShaderStageFlagBits::eFragment)};

	// This example uses two different input states, one for the instanced part and one for non-instanced rendering

	// Vertex input bindings
	// The instancing pipeline uses a vertex input state with two bindings
	std::array<vk::VertexInputBindingDescription, 2> binding_descriptions = {
	    {{0, sizeof(HPPVertex), vk::VertexInputRate::eVertex},               // Binding point 0: Mesh vertex layout description at per-vertex rate
	     {1, sizeof(InstanceData), vk::VertexInputRate::eInstance}}};        // Binding point 1: Instanced data at per-instance rate

	// Vertex attribute bindings
	// Note that the shader declaration for per-vertex and per-instance attributes is the same, the different input rates are only stored in the bindings:
	// instanced.vert:
	//	layout (location = 0) in vec3 inPos;		Per-Vertex
	//	...
	//	layout (location = 4) in vec3 instancePos;	Per-Instance
	std::array<vk::VertexInputAttributeDescription, 7> attribute_descriptions = {
	    {                                                                // Per-vertex attributes
	                                                                     // These are advanced for each vertex fetched by the vertex shader
	     {0, 0, vk::Format::eR32G32B32Sfloat, 0},                        // Location 0: Position
	     {1, 0, vk::Format::eR32G32B32Sfloat, 3 * sizeof(float)},        // Location 1: Normal
	     {2, 0, vk::Format::eR32G32Sfloat, 6 * sizeof(float)},           // Location 2: Texture coordinates
	                                                                     // Per-Instance attributes
	                                                                     // These are fetched for each instance rendered
	     {3, 1, vk::Format::eR32G32B32Sfloat, 0},                        // Location 3: Position
	     {4, 1, vk::Format::eR32G32B32Sfloat, 3 * sizeof(float)},        // Location 4: Rotation
	     {5, 1, vk::Format::eR32Sfloat, 6 * sizeof(float)},              // Location 5: Scale
	     {6, 1, vk::Format::eR32Sint, 7 * sizeof(float)}}};              // Location 6: Texture array layer index

	// Use all input bindings and attribute descriptions
	vk::PipelineVertexInputStateCreateInfo input_state({}, binding_descriptions, attribute_descriptions);

	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state({}, vk::PrimitiveTopology::eTriangleList, false);

	vk::PipelineViewportStateCreateInfo viewport_state({}, 1, nullptr, 1, nullptr);

	vk::PipelineRasterizationStateCreateInfo rasterization_state;
	rasterization_state.polygonMode = vk::PolygonMode::eFill;
	rasterization_state.cullMode    = vk::CullModeFlagBits::eBack;
	rasterization_state.frontFace   = vk::FrontFace::eClockwise;
	rasterization_state.lineWidth   = 1.0f;

	vk::PipelineMultisampleStateCreateInfo multisample_state({}, vk::SampleCountFlagBits::e1);

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthCompareOp   = vk::CompareOp::eGreater;
	depth_stencil_state.depthTestEnable  = true;
	depth_stencil_state.depthWriteEnable = true;
	depth_stencil_state.back.compareOp   = vk::CompareOp::eAlways;
	depth_stencil_state.front            = depth_stencil_state.back;

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo color_blend_state({}, false, {}, blend_attachment_state);

	std::array<vk::DynamicState, 2> dynamic_state_enables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

	vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_state_enables);

	vk::GraphicsPipelineCreateInfo pipeline_create_info({},
	                                                    shader_stages,
	                                                    &input_state,
	                                                    &input_assembly_state,
	                                                    {},
	                                                    &viewport_state,
	                                                    &rasterization_state,
	                                                    &multisample_state,
	                                                    &depth_stencil_state,
	                                                    &color_blend_state,
	                                                    &dynamic_state,
	                                                    pipeline_layout,
	                                                    render_pass,
	                                                    {},
	                                                    {},
	                                                    -1);

	vk::Result result;
	std::tie(result, pipelines.instanced_rocks) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);

	// Planet rendering pipeline
	shader_stages = {load_shader("instancing/planet.vert", vk::ShaderStageFlagBits::eVertex),
	                 load_shader("instancing/planet.frag", vk::ShaderStageFlagBits::eFragment)};
	// Only use the non-instanced input bindings and attribute descriptions
	input_state.vertexBindingDescriptionCount   = 1;
	input_state.vertexAttributeDescriptionCount = 3;

	std::tie(result, pipelines.planet) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);

	// Star field pipeline
	rasterization_state.cullMode         = vk::CullModeFlagBits::eNone;
	depth_stencil_state.depthWriteEnable = false;
	depth_stencil_state.depthTestEnable  = false;

	shader_stages = {load_shader("instancing/starfield.vert", vk::ShaderStageFlagBits::eVertex),
	                 load_shader("instancing/starfield.frag", vk::ShaderStageFlagBits::eFragment)};
	// Vertices are generated in the vertex shader
	input_state.vertexBindingDescriptionCount   = 0;
	input_state.vertexAttributeDescriptionCount = 0;

	std::tie(result, pipelines.starfield) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);
}

void HPPInstancing::prepare_instance_data()
{
	std::vector<InstanceData> instance_data;
	instance_data.resize(INSTANCE_COUNT);

	std::default_random_engine              rnd_generator(lock_simulation_speed ? 0 : static_cast<unsigned>(time(nullptr)));
	std::uniform_real_distribution<float>   uniform_dist(0.0, 1.0);
	std::uniform_int_distribution<uint32_t> rnd_texture_index(0, textures.rocks.image->get_vk_image().get_array_layer_count());

	// Distribute rocks randomly on two different rings
	for (auto i = 0; i < INSTANCE_COUNT / 2; i++)
	{
		glm::vec2 ring0{7.0f, 11.0f};
		glm::vec2 ring1{14.0f, 18.0f};

		float rho, theta;

		// Inner ring
		rho                       = sqrt((pow(ring0[1], 2.0f) - pow(ring0[0], 2.0f)) * uniform_dist(rnd_generator) + pow(ring0[0], 2.0f));
		theta                     = 2.0f * glm::pi<float>() * uniform_dist(rnd_generator);
		instance_data[i].pos      = glm::vec3(rho * cos(theta), uniform_dist(rnd_generator) * 0.5f - 0.25f, rho * sin(theta));
		instance_data[i].rot      = glm::vec3(glm::pi<float>() * uniform_dist(rnd_generator), glm::pi<float>() * uniform_dist(rnd_generator), glm::pi<float>() * uniform_dist(rnd_generator));
		instance_data[i].scale    = 1.5f + uniform_dist(rnd_generator) - uniform_dist(rnd_generator);
		instance_data[i].texIndex = rnd_texture_index(rnd_generator);
		instance_data[i].scale *= 0.75f;

		// Outer ring
		rho                                                                 = sqrt((pow(ring1[1], 2.0f) - pow(ring1[0], 2.0f)) * uniform_dist(rnd_generator) + pow(ring1[0], 2.0f));
		theta                                                               = 2.0f * glm::pi<float>() * uniform_dist(rnd_generator);
		instance_data[static_cast<size_t>(i + INSTANCE_COUNT / 2)].pos      = glm::vec3(rho * cos(theta), uniform_dist(rnd_generator) * 0.5f - 0.25f, rho * sin(theta));
		instance_data[static_cast<size_t>(i + INSTANCE_COUNT / 2)].rot      = glm::vec3(glm::pi<float>() * uniform_dist(rnd_generator), glm::pi<float>() * uniform_dist(rnd_generator), glm::pi<float>() * uniform_dist(rnd_generator));
		instance_data[static_cast<size_t>(i + INSTANCE_COUNT / 2)].scale    = 1.5f + uniform_dist(rnd_generator) - uniform_dist(rnd_generator);
		instance_data[static_cast<size_t>(i + INSTANCE_COUNT / 2)].texIndex = rnd_texture_index(rnd_generator);
		instance_data[static_cast<size_t>(i + INSTANCE_COUNT / 2)].scale *= 0.75f;
	}

	instance_buffer.size = instance_data.size() * sizeof(InstanceData);

	// Staging
	// Instanced data is static, copy to device local memory
	// On devices with separate memory types for host visible and device local memory this will result in better performance
	// On devices with unified memory types (DEVICE_LOCAL_BIT and HOST_VISIBLE_BIT supported at once) this isn't necessary and you could skip the staging

	struct
	{
		vk::DeviceMemory memory;
		vk::Buffer       buffer;
	} staging_buffer;

	std::tie(staging_buffer.buffer, staging_buffer.memory) =
	    get_device()->create_buffer(vk::BufferUsageFlagBits::eTransferSrc,
	                                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
	                                instance_buffer.size,
	                                instance_data.data());

	std::tie(instance_buffer.buffer, instance_buffer.memory) =
	    get_device()->create_buffer(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
	                                vk::MemoryPropertyFlagBits::eDeviceLocal,
	                                instance_buffer.size);

	// Copy to staging buffer
	vk::CommandBuffer copy_command = device->create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

	vk::BufferCopy copy_region(0, 0, instance_buffer.size);
	copy_command.copyBuffer(staging_buffer.buffer, instance_buffer.buffer, copy_region);

	device->flush_command_buffer(copy_command, queue, true);

	instance_buffer.descriptor.range  = instance_buffer.size;
	instance_buffer.descriptor.buffer = instance_buffer.buffer;
	instance_buffer.descriptor.offset = 0;

	// Destroy staging resources
	get_device()->get_handle().destroyBuffer(staging_buffer.buffer);
	get_device()->get_handle().freeMemory(staging_buffer.memory);
}

void HPPInstancing::prepare_uniform_buffers()
{
	uniform_buffers.scene =
	    std::make_unique<vkb::core::HPPBuffer>(*get_device(), sizeof(ubo_vs), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffer(0.0f);
}

void HPPInstancing::update_uniform_buffer(float delta_time)
{
	ubo_vs.projection = camera.matrices.perspective;
	ubo_vs.view       = camera.matrices.view;

	if (!paused)
	{
		ubo_vs.loc_speed += delta_time * 0.35f;
		ubo_vs.glob_speed += delta_time * 0.01f;
	}

	uniform_buffers.scene->convert_and_update(ubo_vs);
}

void HPPInstancing::draw()
{
	HPPApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);

	// Submit to queue
	queue.submit(submit_info);

	HPPApiVulkanSample::submit_frame();
}

bool HPPInstancing::prepare(const vkb::ApplicationOptions &options)
{
	if (!HPPApiVulkanSample::prepare(options))
	{
		return false;
	}

	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.type = vkb::CameraType::LookAt;
	camera.set_perspective(60.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 256.0f, 0.1f);
	camera.set_rotation(glm::vec3(-17.2f, -4.7f, 0.0f));
	camera.set_translation(glm::vec3(5.5f, -1.85f, -18.5f));

	load_assets();
	prepare_instance_data();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();
	prepared = true;
	return true;
}

void HPPInstancing::render(float delta_time)
{
	if (prepared)
	{
		draw();
		if (!paused || camera.updated)
		{
			update_uniform_buffer(delta_time);
		}
	}
}

void HPPInstancing::on_update_ui_overlay(vkb::HPPDrawer &drawer)
{
	if (drawer.header("Statistics"))
	{
		drawer.text("Instances: %d", INSTANCE_COUNT);
	}
}

bool HPPInstancing::resize(const uint32_t width, const uint32_t height)
{
	HPPApiVulkanSample::resize(width, height);
	build_command_buffers();
	return true;
}

std::unique_ptr<vkb::Application> create_hpp_instancing()
{
	return std::make_unique<HPPInstancing>();
}
