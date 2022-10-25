/* Copyright (c) 2019-2021, Sascha Willems
 * Modifications Copyright (c) 2022, Holochip Corporation
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
 * Instanced mesh rendering, uses a separate vertex buffer for instanced data.
 * Adjust density of instanced meshes, displays hardware memory availability/consumption.
 */

#include "memory_budget.h"
#include "benchmark_mode/benchmark_mode.h"

MemoryBudget::MemoryBudget()
{
	title = "Memory Budget on Instanced Mesh Renderer";

	// Enable instance and device extensions required to use VK_EXT_memory_budget
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

	// Initialize physical device memory properties variables
	MemoryBudget::initialize_device_memory_properties();
}

MemoryBudget::~MemoryBudget()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), pipelines.instanced_rocks, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.planet, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.starfield, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		vkDestroyBuffer(get_device().get_handle(), instance_buffer.buffer, nullptr);
		vkFreeMemory(get_device().get_handle(), instance_buffer.memory, nullptr);
		vkDestroySampler(get_device().get_handle(), textures.rocks.sampler, nullptr);
		vkDestroySampler(get_device().get_handle(), textures.planet.sampler, nullptr);
	}
}

void MemoryBudget::request_gpu_features(vkb::PhysicalDevice &gpu)
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
}

void MemoryBudget::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.2f, 0.0f}};
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

		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int>(width), static_cast<int>(height), 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		VkDeviceSize offsets[1] = {0};

		// Star field
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.planet, 0, nullptr);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.starfield);
		vkCmdDraw(draw_cmd_buffers[i], 4, 1, 0, 0);

		// Planet
		auto &planet_vertex_buffer = models.planet->vertex_buffers.at("vertex_buffer");
		auto &planet_index_buffer  = models.planet->index_buffer;
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.planet, 0, nullptr);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.planet);
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, planet_vertex_buffer.get(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], planet_index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(draw_cmd_buffers[i], models.planet->vertex_indices, 1, 0, 0, 0);

		// Instanced rocks
		auto &rock_vertex_buffer = models.rock->vertex_buffers.at("vertex_buffer");
		auto &rock_index_buffer  = models.rock->index_buffer;
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.instanced_rocks, 0, nullptr);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.instanced_rocks);
		// Binding point 0 : Mesh vertex buffer
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, rock_vertex_buffer.get(), offsets);
		// Binding point 1 : Instance data buffer
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 1, 1, &instance_buffer.buffer, offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], rock_index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);
		// Render instances
		vkCmdDrawIndexed(draw_cmd_buffers[i], models.rock->vertex_indices, mesh_density, 0, 0, 0);

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void MemoryBudget::initialize_device_memory_properties()
{
	// Initialize physical device memory budget properties structures variables
	physical_device_memory_budget_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
	physical_device_memory_budget_properties.pNext = nullptr;
	// Initialize physical device memory properties structure variables
	device_memory_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
	device_memory_properties.pNext = &physical_device_memory_budget_properties;
}

MemoryBudget::ConvertedMemory MemoryBudget::update_converted_memory(uint64_t input_memory)
{
	MemoryBudget::ConvertedMemory returnMe{};

	if ( input_memory < kilobyte_coefficient )
	{
		returnMe.data  = input_memory;
		returnMe.units = "B";
	}
	else if ( input_memory>= kilobyte_coefficient && input_memory < megabyte_coefficient)
	{
		returnMe.data  = input_memory / kilobyte_coefficient;
		returnMe.units = "KB";
	}
	else if ( input_memory >= megabyte_coefficient && input_memory < gigabyte_coefficient )
	{
		returnMe.data  = input_memory / megabyte_coefficient;
		returnMe.units = "MB";
	}
	else
	{
		returnMe.data  = input_memory / gigabyte_coefficient;
		returnMe.units = "GB";
	}

	return returnMe;
}

void MemoryBudget::update_device_memory_properties()
{
	vkGetPhysicalDeviceMemoryProperties2(get_device().get_gpu().get_handle(), &device_memory_properties);
	device_memory_heap_count = device_memory_properties.memoryProperties.memoryHeapCount;

	device_memory_total_usage  = 0;
	device_memory_total_budget = 0;

	for (uint32_t i = 0; i < device_memory_heap_count; i++)
	{
		device_memory_total_usage += physical_device_memory_budget_properties.heapUsage[i];
		device_memory_total_budget += physical_device_memory_budget_properties.heapBudget[i];
	}
}

void MemoryBudget::load_assets()
{
	models.rock   = load_model("scenes/rock.gltf");
	models.planet = load_model("scenes/planet.gltf");

	textures.rocks  = load_texture_array("textures/texturearray_rocks_color_rgba.ktx");
	textures.planet = load_texture("textures/lavaplanet_color_rgba.ktx");
}

void MemoryBudget::setup_descriptor_pool()
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

void MemoryBudget::setup_descriptor_set_layout()
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

void MemoryBudget::setup_descriptor_set()
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
	vkUpdateDescriptorSets(get_device().get_handle(), vkb::to_u32(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

	// Planet
	buffer_descriptor = create_descriptor(*uniform_buffers.scene);
	image_descriptor  = create_descriptor(textures.planet);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_alloc_info, &descriptor_sets.planet));
	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_sets.planet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &buffer_descriptor),              // Binding 0 : Vertex shader uniform buffer
	    vkb::initializers::write_descriptor_set(descriptor_sets.planet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &image_descriptor)        // Binding 1 : Color map
	};
	vkUpdateDescriptorSets(get_device().get_handle(), vkb::to_u32(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void MemoryBudget::prepare_pipelines()
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
	    vkb::initializers::vertex_input_attribute_description(0, 3, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 8),        // Location 3: Color
	    // Per-Instance attributes
	    // These are fetched for each instance rendered
	    vkb::initializers::vertex_input_attribute_description(1, 4, VK_FORMAT_R32G32B32_SFLOAT, 0),                        // Location 4: Position
	    vkb::initializers::vertex_input_attribute_description(1, 5, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),        // Location 5: Rotation
	    vkb::initializers::vertex_input_attribute_description(1, 6, VK_FORMAT_R32_SFLOAT, sizeof(float) * 6),              // Location 6: Scale
	    vkb::initializers::vertex_input_attribute_description(1, 7, VK_FORMAT_R32_SINT, sizeof(float) * 7),                // Location 7: Texture array layer index
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
	input_state.vertexAttributeDescriptionCount = 4;
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

void MemoryBudget::prepare_instance_data()
{
	std::vector<InstanceData> instance_data;
	instance_data.resize(mesh_density);

	std::default_random_engine              rnd_generator(platform->using_plugin<::plugins::BenchmarkMode>() ? 0 : (unsigned) time(nullptr));
	std::uniform_real_distribution<float>   uniform_dist(0.0, 1.0);
	std::uniform_int_distribution<uint32_t> rnd_texture_index(0, textures.rocks.image->get_vk_image().get_array_layer_count());

	// Distribute rocks randomly on two different rings
	for (auto i = 0; i < mesh_density / 2; i++)
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
		instance_data[static_cast<uint32_t>(i + mesh_density / 2)].pos      = glm::vec3(rho * cos(theta), uniform_dist(rnd_generator) * 0.5f - 0.25f, rho * sin(theta));
		instance_data[static_cast<uint32_t>(i + mesh_density / 2)].rot      = glm::vec3(glm::pi<float>() * uniform_dist(rnd_generator), glm::pi<float>() * uniform_dist(rnd_generator), glm::pi<float>() * uniform_dist(rnd_generator));
		instance_data[static_cast<uint32_t>(i + mesh_density / 2)].scale    = 1.5f + uniform_dist(rnd_generator) - uniform_dist(rnd_generator);
		instance_data[static_cast<uint32_t>(i + mesh_density / 2)].texIndex = rnd_texture_index(rnd_generator);
		instance_data[static_cast<uint32_t>(i + mesh_density / 2)].scale *= 0.75f;
	}

	instance_buffer.size = instance_data.size() * sizeof(InstanceData);

	// Staging
	// Instanced data is static, copy to device local memory
	// On devices with separate memory types for host visible and device local memory this will result in better performance
	// On devices with unified memory types (DEVICE_LOCAL_BIT and HOST_VISIBLE_BIT supported at once) this isn't necessary and you could skip the staging

	struct
	{
		VkDeviceMemory memory;
		VkBuffer       buffer;
	} staging_buffer{};

	staging_buffer.buffer = get_device().create_buffer(
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	    instance_buffer.size,
	    &staging_buffer.memory,
	    instance_data.data());

	instance_buffer.buffer = get_device().create_buffer(
	    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	    instance_buffer.size,
	    &instance_buffer.memory);

	// Copy to staging buffer
	VkCommandBuffer copy_command = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copy_region = {};
	copy_region.size         = instance_buffer.size;
	vkCmdCopyBuffer(
	    copy_command,
	    staging_buffer.buffer,
	    instance_buffer.buffer,
	    1,
	    &copy_region);

	device->flush_command_buffer(copy_command, queue, true);

	instance_buffer.descriptor.range  = instance_buffer.size;
	instance_buffer.descriptor.buffer = instance_buffer.buffer;
	instance_buffer.descriptor.offset = 0;

	// Destroy staging resources
	vkDestroyBuffer(get_device().get_handle(), staging_buffer.buffer, nullptr);
	vkFreeMemory(get_device().get_handle(), staging_buffer.memory, nullptr);
}

void MemoryBudget::prepare_uniform_buffers()
{
	uniform_buffers.scene = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                            sizeof(ubo_vs),
	                                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                            VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffer(0.0f);
}

void MemoryBudget::update_uniform_buffer(float delta_time)
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

void MemoryBudget::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

bool MemoryBudget::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	// Note: Using Reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.type = vkb::CameraType::LookAt;
	camera.set_perspective(60.0f, (float) width / (float) height, 256.0f, 0.1f);
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

	// Update the device memory properties and calculate the total heap memory usage and budget:
	// If no changes happen to the total number of instanced meshes, then device should now have allocated total memory expected to be used.
	// While the memory_budget_ext is performant enough to be called every frame, this sample only has one allocation happen if all preparation remain the same.
	// Thus, no update to the memory totals beyond the first allocation is necessary.
	update_device_memory_properties();

	prepared = true;
	return true;
}

void MemoryBudget::render(float delta_time)
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

void MemoryBudget::on_update_ui_overlay(vkb::Drawer &drawer)
{
	converted_memory = update_converted_memory(device_memory_total_usage);
	drawer.text("Total Memory Usage: %llu %s", converted_memory.data, converted_memory.units.c_str());
	converted_memory = update_converted_memory(device_memory_total_budget);
	drawer.text("Total Memory Budget: %llu %s", converted_memory.data, converted_memory.units.c_str());

	if (drawer.header("Memory Heap Details:"))
	{
		for (uint32_t i = 0; i < device_memory_heap_count; i++)
		{
			converted_memory = update_converted_memory(physical_device_memory_budget_properties.heapUsage[i]);
			drawer.text("Memory Heap %lu: Usage: %llu %s", i, converted_memory.data, converted_memory.units.c_str());

			converted_memory = update_converted_memory(physical_device_memory_budget_properties.heapBudget[i]);
			drawer.text("Memory Heap %lu: Budget: %llu %s", i, converted_memory.data, converted_memory.units.c_str());
		}
	}
}

void MemoryBudget::resize(uint32_t width, uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	build_command_buffers();
}

std::unique_ptr<vkb::Application> create_memory_budget()
{
	return std::make_unique<MemoryBudget>();
}
