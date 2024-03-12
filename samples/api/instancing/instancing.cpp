/* Copyright (c) 2019-2024, Sascha Willems
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
 * Instanced mesh rendering, uses a separate vertex buffer for instanced data
 */

#include "instancing.h"

#include "benchmark_mode/benchmark_mode.h"

Instancing::Instancing()
{
	title = "Instanced mesh rendering";
}

Instancing::~Instancing()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipelines.instanced_rocks, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.planet, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.starfield, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		vkDestroySampler(get_device().get_handle(), textures.rocks.sampler, nullptr);
		vkDestroySampler(get_device().get_handle(), textures.planet.sampler, nullptr);
	}
}

void Instancing::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	auto &requested_features = gpu.get_mutable_requested_features();

	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		requested_features.samplerAnisotropy = VK_TRUE;
	}
	// Enable texture compression
	if (gpu.get_features().textureCompressionBC)
	{
		requested_features.textureCompressionBC = VK_TRUE;
	}
	else if (gpu.get_features().textureCompressionASTC_LDR)
	{
		requested_features.textureCompressionASTC_LDR = VK_TRUE;
	}
	else if (gpu.get_features().textureCompressionETC2)
	{
		requested_features.textureCompressionETC2 = VK_TRUE;
	}
};

void Instancing::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.033f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		// Set target frame buffer
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		VkDeviceSize offsets[1] = {0};

		// Star field
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.planet, 0, NULL);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.starfield);
		vkCmdDraw(draw_cmd_buffers[i], 4, 1, 0, 0);

		// Planet
		auto &planet_vertex_buffer = models.planet->vertex_buffers.at("vertex_buffer");
		auto &planet_index_buffer  = models.planet->index_buffer;
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.planet, 0, NULL);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.planet);
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, planet_vertex_buffer.get(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], planet_index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(draw_cmd_buffers[i], models.planet->vertex_indices, 1, 0, 0, 0);

		// Instanced rocks
		auto &rock_vertex_buffer = models.rock->vertex_buffers.at("vertex_buffer");
		auto &rock_index_buffer  = models.rock->index_buffer;
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.instanced_rocks, 0, NULL);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.instanced_rocks);
		// Binding point 0 : Mesh vertex buffer
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, rock_vertex_buffer.get(), offsets);
		// Binding point 1 : Instance data buffer
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 1, 1, &instance_buffer.buffer->get_handle(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], rock_index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);
		// Render instances
		vkCmdDrawIndexed(draw_cmd_buffers[i], models.rock->vertex_indices, INSTANCE_COUNT, 0, 0, 0);

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void Instancing::load_assets()
{
	models.rock   = load_model("scenes/rock.gltf");
	models.planet = load_model("scenes/planet.gltf");

	// models.rock.loadFromFile(getAssetPath() + "scenes/rock.gltf", device.get(), queue);
	// models.planet.loadFromFile(getAssetPath() + "scenes/planet.gltf", device.get(), queue);

	textures.rocks  = load_texture_array("textures/texturearray_rocks_color_rgba.ktx", vkb::sg::Image::Color);
	textures.planet = load_texture("textures/lavaplanet_color_rgba.ktx", vkb::sg::Image::Color);

	// textures.rocks.loadFromFile(getAssetPath() + "textures/texturearray_rocks_color_rgba.ktx", device.get(), queue);
	// textures.planet.loadFromFile(getAssetPath() + "textures/lavaplanet_color_rgba.ktx", device.get(), queue);
}

void Instancing::setup_descriptor_pool()
{
	// Example uses one ubo
	std::vector<VkDescriptorPoolSize> pool_sizes =
	    {
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2),
	    };

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        vkb::to_u32(pool_sizes.size()),
	        pool_sizes.data(),
	        2);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void Instancing::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings =
	    {
	        // Binding 0 : Vertex shader uniform buffer
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            VK_SHADER_STAGE_VERTEX_BIT,
	            0),
	        // Binding 1 : Fragment shader combined sampler
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	            VK_SHADER_STAGE_FRAGMENT_BIT,
	            1),
	    };

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(
	        set_layout_bindings.data(),
	        vkb::to_u32(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void Instancing::setup_descriptor_set()
{
	VkDescriptorSetAllocateInfo       descriptor_set_alloc_info;
	std::vector<VkWriteDescriptorSet> write_descriptor_sets;

	descriptor_set_alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);

	// Instanced rocks
	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniform_buffers.scene);
	VkDescriptorImageInfo  image_descriptor  = create_descriptor(textures.rocks);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_alloc_info, &descriptor_sets.instanced_rocks));
	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_sets.instanced_rocks, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &buffer_descriptor),              // Binding 0 : Vertex shader uniform buffer
	    vkb::initializers::write_descriptor_set(descriptor_sets.instanced_rocks, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &image_descriptor)        // Binding 1 : Color map
	};
	vkUpdateDescriptorSets(get_device().get_handle(), vkb::to_u32(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);

	// Planet
	buffer_descriptor = create_descriptor(*uniform_buffers.scene);
	image_descriptor  = create_descriptor(textures.planet);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_alloc_info, &descriptor_sets.planet));
	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_sets.planet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &buffer_descriptor),              // Binding 0 : Vertex shader uniform buffer
	    vkb::initializers::write_descriptor_set(descriptor_sets.planet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &image_descriptor)        // Binding 1 : Color map
	};
	vkUpdateDescriptorSets(get_device().get_handle(), vkb::to_u32(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
}

void Instancing::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	        0,
	        VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_BACK_BIT,
	        VK_FRONT_FACE_CLOCKWISE,
	        0);

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        0xf,
	        VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
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
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(
	        pipeline_layout,
	        render_pass,
	        0);

	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.stageCount          = vkb::to_u32(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();

	// This example uses two different input states, one for the instanced part and one for non-instanced rendering
	VkPipelineVertexInputStateCreateInfo           input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	std::vector<VkVertexInputBindingDescription>   binding_descriptions;
	std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

	// Vertex input bindings
	// The instancing pipeline uses a vertex input state with two bindings
	binding_descriptions = {
	    // Binding point 0: Mesh vertex layout description at per-vertex rate
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	    // Binding point 1: Instanced data at per-instance rate
	    vkb::initializers::vertex_input_binding_description(1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)};

	// Vertex attribute bindings
	// Note that the shader declaration for per-vertex and per-instance attributes is the same, the different input rates are only stored in the bindings:
	// instanced.vert:
	//	layout (location = 0) in vec3 inPos;		Per-Vertex
	//	...
	//	layout (location = 4) in vec3 instancePos;	Per-Instance
	attribute_descriptions = {
	    // Per-vertex attributes
	    // These are advanced for each vertex fetched by the vertex shader
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                        // Location 0: Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),        // Location 1: Normal
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6),           // Location 2: Texture coordinates
	    // Per-Instance attributes
	    // These are fetched for each instance rendered
	    vkb::initializers::vertex_input_attribute_description(1, 3, VK_FORMAT_R32G32B32_SFLOAT, 0),                        // Location 3: Position
	    vkb::initializers::vertex_input_attribute_description(1, 4, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),        // Location 4: Rotation
	    vkb::initializers::vertex_input_attribute_description(1, 5, VK_FORMAT_R32_SFLOAT, sizeof(float) * 6),              // Location 5: Scale
	    vkb::initializers::vertex_input_attribute_description(1, 6, VK_FORMAT_R32_SINT, sizeof(float) * 7),                // Location 6: Texture array layer index
	};
	input_state.pVertexBindingDescriptions   = binding_descriptions.data();
	input_state.pVertexAttributeDescriptions = attribute_descriptions.data();

	pipeline_create_info.pVertexInputState = &input_state;

	// Instancing pipeline
	shader_stages[0] = load_shader("instancing/instancing.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("instancing/instancing.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	// Use all input bindings and attribute descriptions
	input_state.vertexBindingDescriptionCount   = static_cast<uint32_t>(binding_descriptions.size());
	input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.instanced_rocks));

	// Planet rendering pipeline
	shader_stages[0] = load_shader("instancing/planet.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("instancing/planet.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	// Only use the non-instanced input bindings and attribute descriptions
	input_state.vertexBindingDescriptionCount   = 1;
	input_state.vertexAttributeDescriptionCount = 3;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.planet));

	// Star field pipeline
	rasterization_state.cullMode         = VK_CULL_MODE_NONE;
	depth_stencil_state.depthWriteEnable = VK_FALSE;
	depth_stencil_state.depthTestEnable  = VK_FALSE;
	shader_stages[0]                     = load_shader("instancing/starfield.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]                     = load_shader("instancing/starfield.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	// Vertices are generated in the vertex shader
	input_state.vertexBindingDescriptionCount   = 0;
	input_state.vertexAttributeDescriptionCount = 0;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.starfield));
}

void Instancing::prepare_instance_data()
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

	vkb::core::Buffer staging_buffer = vkb::core::Buffer::create_staging_buffer(get_device(), instance_data);

	instance_buffer.buffer = std::make_unique<vkb::core::Buffer>(get_device(), instance_buffer.size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	// Copy to staging buffer
	VkCommandBuffer copy_command = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copy_region = {};
	copy_region.size         = instance_buffer.size;
	vkCmdCopyBuffer(
	    copy_command,
	    staging_buffer.get_handle(),
	    instance_buffer.buffer->get_handle(),
	    1,
	    &copy_region);

	get_device().flush_command_buffer(copy_command, queue, true);

	instance_buffer.descriptor.range  = instance_buffer.size;
	instance_buffer.descriptor.buffer = instance_buffer.buffer->get_handle();
	instance_buffer.descriptor.offset = 0;
}

void Instancing::prepare_uniform_buffers()
{
	uniform_buffers.scene = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                            sizeof(ubo_vs),
	                                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                            VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffer(0.0f);
}

void Instancing::update_uniform_buffer(float delta_time)
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

void Instancing::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

bool Instancing::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.type = vkb::CameraType::LookAt;
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);
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

void Instancing::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (!paused || camera.updated)
	{
		update_uniform_buffer(delta_time);
	}
}

void Instancing::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Statistics"))
	{
		drawer.text("Instances: %d", INSTANCE_COUNT);
	}
}

bool Instancing::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	rebuild_command_buffers();
	return true;
}

std::unique_ptr<vkb::Application> create_instancing()
{
	return std::make_unique<Instancing>();
}
