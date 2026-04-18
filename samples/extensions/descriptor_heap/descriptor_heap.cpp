/*
 * Copyright (c) 2026 Sascha Willems
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

#include "descriptor_heap.h"

DescriptorHeap::DescriptorHeap()
{
	title = "Descriptor heap";

	add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	add_device_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
	add_device_extension(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);
	add_device_extension(VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME);
}

DescriptorHeap::~DescriptorHeap()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		for (auto &cube : cubes)
		{
			cube.texture.image.reset();
			vkDestroySampler(get_device().get_handle(), cube.texture.sampler, nullptr);
		}
		cube.reset();
		descriptor_heap_resources.reset();
		descriptor_heap_samplers.reset();
	}
}

bool DescriptorHeap::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.f, 0.f, -4.f});
	camera.set_rotation({0.f, 180.f, 0.f});
	camera.set_perspective(60.f, static_cast<float>(width) / static_cast<float>(height), 256.f, 0.1f);

	load_assets();
	prepare_uniform_buffers();
	create_descriptor_heaps();
	create_pipeline();
	build_command_buffers();
	prepared = true;

	return true;
}

void DescriptorHeap::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	// Enable features required for this example
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceBufferDeviceAddressFeatures, bufferDeviceAddress);
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceDynamicRenderingFeaturesKHR, dynamicRendering);

	// We need to enable the descriptor heap feature to make use of them
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceDescriptorHeapFeaturesEXT, descriptorHeap);

	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = true;
	}
}

void DescriptorHeap::setup_render_pass()
{
	// We use dynamic rendering, so we skip render pass setup
}

void DescriptorHeap::setup_framebuffer()
{
	// We use dynamic rendering, so we skip framebuffer setup
}

void DescriptorHeap::on_update_ui_overlay(vkb::Drawer &drawer)
{
	drawer.combo_box("Sampler type", &selected_sampler, sampler_names);
}

uint32_t DescriptorHeap::get_api_version() const
{
	return VK_API_VERSION_1_2;
}

void DescriptorHeap::load_assets()
{
	cube             = load_model("scenes/textured_unit_cube.gltf");
	cubes[0].texture = load_texture("textures/crate01_color_height_rgba.ktx", vkb::sg::Image::Color);
	cubes[1].texture = load_texture("textures/crate02_color_height_rgba.ktx", vkb::sg::Image::Color);
}

inline static VkDeviceSize aligned_size(VkDeviceSize value, VkDeviceSize alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

void DescriptorHeap::prepare_uniform_buffers()
{
	uniform_buffers.resize(draw_cmd_buffers.size());
	for (auto &uniform_buffer : uniform_buffers)
	{
		uniform_buffer = std::make_unique<vkb::core::BufferC>(get_device(), sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}
}

void DescriptorHeap::update_uniform_buffers(float delta_time)
{
	if (animate)
	{
		cubes[0].rotation.x += 2.5f * delta_time;
		if (cubes[0].rotation.x > 360.0f)
		{
			cubes[0].rotation.x -= 360.0f;
		}
		cubes[1].rotation.y += 2.0f * delta_time;
		if (cubes[1].rotation.y > 360.0f)
		{
			cubes[1].rotation.y -= 360.0f;
		}
	}

	uniform_data.projection_matrix = camera.matrices.perspective;
	uniform_data.view_matrix       = camera.matrices.view;

	std::array<glm::vec3, 2> positions = {glm::vec3(-2.0f, 0.0f, 0.0f), glm::vec3(1.5f, 0.5f, 0.0f)};
	for (auto i = 0; i < cubes.size(); i++)
	{
		glm::mat4 cubeMat            = glm::translate(glm::mat4(1.0f), positions[i]);
		cubeMat                      = glm::rotate(cubeMat, glm::radians(cubes[i].rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		cubeMat                      = glm::rotate(cubeMat, glm::radians(cubes[i].rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		cubeMat                      = glm::rotate(cubeMat, glm::radians(cubes[i].rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		uniform_data.model_matrix[i] = cubeMat;
	}

	uniform_buffers[current_buffer]->convert_and_update(uniform_data);
}

void DescriptorHeap::create_descriptor_heaps()
{
	// Descriptor heaps have varying offset, size and alignment requirements, so we store it's properties for later user
	descriptor_heap_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;
	VkPhysicalDeviceProperties2 device_props_2{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
	    .pNext = &descriptor_heap_properties};
	vkGetPhysicalDeviceProperties2(get_device().get_gpu().get_handle(), &device_props_2);

	// There are two descriptor heap types: One that can store resources (buffers, images) and one that can store samplers
	// We create heaps with a fixed size that's guaranteed to fit in the few descriptors we use
	const VkDeviceSize heap_buffer_size = aligned_size(2048 + descriptor_heap_properties.minResourceHeapReservedRange, descriptor_heap_properties.resourceHeapAlignment);

	descriptor_heap_resources = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                                 heap_buffer_size,
	                                                                 VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	                                                                 VMA_MEMORY_USAGE_CPU_TO_GPU);

	const VkDeviceSize heap_sampler_size = aligned_size(2048 + descriptor_heap_properties.minSamplerHeapReservedRange, descriptor_heap_properties.samplerHeapAlignment);

	descriptor_heap_samplers = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                                heap_sampler_size,
	                                                                VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	                                                                VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Sampler heap
	sampler_descriptor_size = aligned_size(descriptor_heap_properties.samplerDescriptorSize, descriptor_heap_properties.samplerDescriptorAlignment);

	std::array<VkHostAddressRangeEXT, 2> host_address_ranges_samplers{};

	// No need to create an actual VkSampler, we can simply pass the create info that describes the sampler
	std::array<VkSamplerCreateInfo, 2> sampler_create_infos{
	    VkSamplerCreateInfo{
	        .sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	        .magFilter     = VK_FILTER_LINEAR,
	        .minFilter     = VK_FILTER_LINEAR,
	        .mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR,
	        .addressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	        .addressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	        .addressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	        .maxAnisotropy = 16.0f,
	        .maxLod        = (float) cubes[0].texture.image.get()->get_mipmaps().size(),
	    },
	    VkSamplerCreateInfo{
	        .sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	        .magFilter     = VK_FILTER_NEAREST,
	        .minFilter     = VK_FILTER_NEAREST,
	        .mipmapMode    = VK_SAMPLER_MIPMAP_MODE_NEAREST,
	        .addressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	        .addressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	        .addressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	        .maxAnisotropy = 16.0f,
	        .maxLod        = (float) cubes[1].texture.image.get()->get_mipmaps().size(),
	    }};

	for (auto i = 0; i < static_cast<uint32_t>(sampler_create_infos.size()); i++)
	{
		host_address_ranges_samplers[i] = {
		    .address = (uint8_t *) (descriptor_heap_samplers->get_data()) + sampler_heap_offset + sampler_descriptor_size * i,
		    .size    = sampler_descriptor_size};
	}

	VK_CHECK(vkWriteSamplerDescriptorsEXT(get_device().get_handle(), static_cast<uint32_t>(host_address_ranges_samplers.size()), sampler_create_infos.data(), host_address_ranges_samplers.data()));

	// Resource heap (buffers and images)
	buffer_descriptor_size = aligned_size(descriptor_heap_properties.bufferDescriptorSize, descriptor_heap_properties.bufferDescriptorAlignment);
	// Images are stored after the last buffer (aligned)
	image_heap_offset     = aligned_size(buffer_descriptor_size * uniform_buffers.size(), descriptor_heap_properties.imageDescriptorAlignment);
	image_descriptor_size = aligned_size(descriptor_heap_properties.imageDescriptorSize, descriptor_heap_properties.imageDescriptorAlignment);

	auto                                     vector_size{cubes.size() + uniform_buffers.size()};
	std::vector<VkHostAddressRangeEXT>       host_address_ranges_resources(vector_size);
	std::vector<VkResourceDescriptorInfoEXT> resource_descriptor_infos(vector_size);

	size_t resource_heap_index{0};

	// Buffers
	std::vector<VkDeviceAddressRangeEXT> device_address_ranges_uniform_buffer(uniform_buffers.size());
	for (auto i = 0; i < uniform_buffers.size(); i++)
	{
		device_address_ranges_uniform_buffer[i]        = {.address = uniform_buffers[i]->get_device_address(), .size = uniform_buffers[i]->get_size()};
		resource_descriptor_infos[resource_heap_index] = {
		    .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
		    .type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		    .data  = {
		         .pAddressRange = &device_address_ranges_uniform_buffer[i]}};
		host_address_ranges_resources[resource_heap_index] = {
		    .address = (uint8_t *) (descriptor_heap_resources->get_data()) + buffer_descriptor_size * i,
		    .size    = buffer_descriptor_size};

		resource_heap_index++;
	}

	// Images
	std::array<VkImageViewCreateInfo, 2>    image_view_create_infos{};
	std::array<VkImageDescriptorInfoEXT, 2> image_descriptor_infos{};

	for (auto i = 0; i < cubes.size(); i++)
	{
		image_view_create_infos[i] = {
		    .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		    .image            = cubes[i].texture.image->get_vk_image().get_handle(),
		    .viewType         = VK_IMAGE_VIEW_TYPE_2D,
		    .format           = cubes[i].texture.image->get_format(),
		    .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = static_cast<uint32_t>(cubes[i].texture.image->get_mipmaps().size()), .baseArrayLayer = 0, .layerCount = 1},
		};

		image_descriptor_infos[i] = {
		    .sType  = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
		    .pView  = &image_view_create_infos[i],
		    .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		resource_descriptor_infos[resource_heap_index] = {
		    .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
		    .type  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		    .data  = {
		         .pImage = &image_descriptor_infos[i]}};

		host_address_ranges_resources[resource_heap_index] = {
		    .address = (uint8_t *) (descriptor_heap_resources->get_data()) + image_heap_offset + image_descriptor_size * i,
		    .size    = image_descriptor_size};

		resource_heap_index++;
	}

	VK_CHECK(vkWriteResourceDescriptorsEXT(get_device().get_handle(), static_cast<uint32_t>(resource_descriptor_infos.size()), resource_descriptor_infos.data(), host_address_ranges_resources.data()));
}

void DescriptorHeap::create_pipeline()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state   = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterization_state    = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState    blend_attachment_state = vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo    color_blend_state      = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);
	VkPipelineDepthStencilStateCreateInfo  depth_stencil_state    = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER);
	VkPipelineViewportStateCreateInfo      viewport_state         = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo   multisample_state      = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState>            dynamic_state_enables  = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo       dynamic_state          = vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)),
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
	shader_stages[0] = load_shader("descriptor_heap", "cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("descriptor_heap", "cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Descriptor heaps can be used without having to explicitly change the shaders
	// This is done by specifiying the bindings and their types at the shader stage level
	// As samplers require a different heap (than images), we can't use combined images

	std::array<VkDescriptorSetAndBindingMappingEXT, 3> set_binding_mappings{};

	// Buffer binding
	set_binding_mappings[0] = {
	    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
	    .descriptorSet = 0,
	    .firstBinding  = 0,
	    .bindingCount  = 1,
	    .resourceMask  = VK_SPIRV_RESOURCE_TYPE_UNIFORM_BUFFER_BIT_EXT,
	    .source        = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_CONSTANT_OFFSET_EXT,
	    .sourceData    = {
	           .constantOffset = {
	               .heapArrayStride = static_cast<uint32_t>(buffer_descriptor_size)}}};

	// We are using multiple images, which requires us to set heapArrayStride to let the implementation know where image n+1 starts
	set_binding_mappings[1] = {
	    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
	    .descriptorSet = 1,
	    .firstBinding  = 0,
	    .bindingCount  = 1,
	    .resourceMask  = VK_SPIRV_RESOURCE_TYPE_SAMPLED_IMAGE_BIT_EXT,
	    .source        = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_CONSTANT_OFFSET_EXT,
	    .sourceData    = {
	           .constantOffset = {
	               .heapOffset = static_cast<uint32_t>(image_heap_offset), .heapArrayStride = static_cast<uint32_t>(image_descriptor_size)}}};

	// As samplers require a different heap (than images), we can't use combined images but split image and sampler
	set_binding_mappings[2] = {
	    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
	    .descriptorSet = 2,
	    .firstBinding  = 0,
	    .bindingCount  = 1,
	    .resourceMask  = VK_SPIRV_RESOURCE_TYPE_SAMPLER_BIT_EXT,
	    .source        = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_CONSTANT_OFFSET_EXT,
	    .sourceData    = {
	           .constantOffset = {
	               .heapOffset = static_cast<uint32_t>(sampler_heap_offset), .heapArrayStride = static_cast<uint32_t>(sampler_descriptor_size)}}};

	VkShaderDescriptorSetAndBindingMappingInfoEXT descriptor_set_binding_mapping_info{
	    .sType        = VK_STRUCTURE_TYPE_SHADER_DESCRIPTOR_SET_AND_BINDING_MAPPING_INFO_EXT,
	    .mappingCount = static_cast<uint32_t>(set_binding_mappings.size()),
	    .pMappings    = set_binding_mappings.data()};

	shader_stages[0].pNext = &descriptor_set_binding_mapping_info;
	shader_stages[1].pNext = &descriptor_set_binding_mapping_info;

	// Create graphics pipeline for dynamic rendering
	VkFormat color_rendering_format = get_render_context().get_format();

	// Provide information for dynamic rendering
	VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{
	    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
	    .colorAttachmentCount    = 1,
	    .pColorAttachmentFormats = &color_rendering_format,
	    .depthAttachmentFormat   = depth_format};
	if (!vkb::is_depth_only_format(depth_format))
	{
		pipeline_rendering_create_info.stencilAttachmentFormat = depth_format;
	}

	// Use the pNext to point to the rendering create struct
	VkGraphicsPipelineCreateInfo pipeline_create_info{
	    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	    .stageCount          = static_cast<uint32_t>(shader_stages.size()),
	    .pStages             = shader_stages.data(),
	    .pVertexInputState   = &vertex_input_state,
	    .pInputAssemblyState = &input_assembly_state,
	    .pViewportState      = &viewport_state,
	    .pRasterizationState = &rasterization_state,
	    .pMultisampleState   = &multisample_state,
	    .pDepthStencilState  = &depth_stencil_state,
	    .pColorBlendState    = &color_blend_state,
	    .pDynamicState       = &dynamic_state,
	};

	// With descriptor heaps we no longer need a pipeline layout
	// This struct must be chained into pipeline creation to enable the use of heaps (allowing us to leave pipelineLayout empty)
	VkPipelineCreateFlags2CreateInfo pipeline_create_flags_2{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO,
	    .pNext = &pipeline_rendering_create_info,
	    .flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT};
	pipeline_create_info.pNext = &pipeline_create_flags_2;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &pipeline_create_info, VK_NULL_HANDLE, &pipeline));
}

void DescriptorHeap::draw(float delta_time)
{
	ApiVulkanSample::prepare_frame();
	update_uniform_buffers(delta_time);
	build_command_buffer();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void DescriptorHeap::build_command_buffers()
{
	// This sample doesn't use prebuilt command buffers
}

void DescriptorHeap::build_command_buffer()
{
	VkCommandBuffer draw_cmd_buffer = draw_cmd_buffers[current_buffer];
	vkResetCommandBuffer(draw_cmd_buffer, 0);

	std::array<VkClearValue, 2> clear_values{};
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	auto command_begin = vkb::initializers::command_buffer_begin_info();
	VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffer, &command_begin));

	VkImageSubresourceRange range{
	    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
	    .baseMipLevel   = 0,
	    .levelCount     = VK_REMAINING_MIP_LEVELS,
	    .baseArrayLayer = 0,
	    .layerCount     = VK_REMAINING_ARRAY_LAYERS};

	VkImageSubresourceRange depth_range{range};
	depth_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

	vkb::image_layout_transition(draw_cmd_buffer,
	                             swapchain_buffers[current_buffer].image,
	                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                             0,
	                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	                             VK_IMAGE_LAYOUT_UNDEFINED,
	                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                             range);

	vkb::image_layout_transition(draw_cmd_buffer,
	                             depth_stencil.image,
	                             VK_IMAGE_LAYOUT_UNDEFINED,
	                             VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
	                             depth_range);

	VkRenderingAttachmentInfoKHR color_attachment_info{
	    .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
	    .imageView   = swapchain_buffers[current_buffer].view,
	    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	    .resolveMode = VK_RESOLVE_MODE_NONE,
	    .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
	    .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
	    .clearValue  = clear_values[0]};

	VkRenderingAttachmentInfoKHR depth_attachment_info{
	    .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
	    .imageView   = depth_stencil.view,
	    .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
	    .resolveMode = VK_RESOLVE_MODE_NONE,
	    .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
	    .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	    .clearValue  = clear_values[1]};

	auto render_area             = VkRect2D{VkOffset2D{}, VkExtent2D{width, height}};
	auto render_info             = vkb::initializers::rendering_info(render_area, 1, &color_attachment_info);
	render_info.layerCount       = 1;
	render_info.pDepthAttachment = &depth_attachment_info;
	if (!vkb::is_depth_only_format(depth_format))
	{
		render_info.pStencilAttachment = &depth_attachment_info;
	}

	vkCmdBeginRenderingKHR(draw_cmd_buffer, &render_info);

	VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
	vkCmdSetViewport(draw_cmd_buffer, 0, 1, &viewport);

	VkRect2D scissor = vkb::initializers::rect2D(static_cast<int>(width), static_cast<int>(height), 0, 0);
	vkCmdSetScissor(draw_cmd_buffer, 0, 1, &scissor);

	vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	// Pass options as push data
	struct PushData
	{
		int32_t samplerIndex;
		int32_t frameIndex;
	} push_data = {
	    .samplerIndex = selected_sampler,
	    // Samples do not support frames-in-flight yet, so frameIndex never changes
	    .frameIndex = 0};
	VkPushDataInfoEXT push_data_info{
	    .sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
	    .data  = {.address = &push_data, .size = sizeof(PushData)}};
	vkCmdPushDataEXT(draw_cmd_buffer, &push_data_info);

	// Bind the heap containing resources (buffers and images)
	VkBindHeapInfoEXT bind_heap_info_res{
	    .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
	    .heapRange{
	        .address = descriptor_heap_resources->get_device_address(),
	        .size    = descriptor_heap_resources->get_size()},
	    .reservedRangeOffset = descriptor_heap_resources->get_size() - descriptor_heap_properties.minResourceHeapReservedRange,
	    .reservedRangeSize   = descriptor_heap_properties.minResourceHeapReservedRange,
	};
	vkCmdBindResourceHeapEXT(draw_cmd_buffer, &bind_heap_info_res);

	// Bind the heap containing samplers
	VkBindHeapInfoEXT bind_heap_info_samplers{
	    .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
	    .heapRange{
	        .address = descriptor_heap_samplers->get_device_address(),
	        .size    = descriptor_heap_samplers->get_size()},
	    .reservedRangeOffset = descriptor_heap_samplers->get_size() - descriptor_heap_properties.minSamplerHeapReservedRange,
	    .reservedRangeSize   = descriptor_heap_properties.minSamplerHeapReservedRange};
	vkCmdBindSamplerHeapEXT(draw_cmd_buffer, &bind_heap_info_samplers);

	VkDeviceSize offsets[1] = {0};

	const auto &vertex_buffer = cube->vertex_buffers.at("vertex_buffer");
	auto       &index_buffer  = cube->index_buffer;

	vkCmdBindVertexBuffers(draw_cmd_buffer, 0, 1, vertex_buffer.get(), offsets);
	vkCmdBindIndexBuffer(draw_cmd_buffer, index_buffer->get_handle(), 0, cube->index_type);
	vkCmdDrawIndexed(draw_cmd_buffer, cube->vertex_indices, 2, 0, 0, 0);

	vkCmdEndRenderingKHR(draw_cmd_buffer);

	draw_ui(draw_cmd_buffer, current_buffer);

	vkb::image_layout_transition(draw_cmd_buffer,
	                             swapchain_buffers[current_buffer].image,
	                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	                             range);

	VK_CHECK(vkEndCommandBuffer(draw_cmd_buffer));
}

void DescriptorHeap::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw(delta_time);
}

void DescriptorHeap::create_command_pool()
{
	VkCommandPoolCreateInfo command_pool_info{
	    .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	    .queueFamilyIndex = get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0).get_family_index()};
	VK_CHECK(vkCreateCommandPool(get_device().get_handle(), &command_pool_info, nullptr, &cmd_pool));
}

std::unique_ptr<vkb::VulkanSampleC> create_descriptor_heap()
{
	return std::make_unique<DescriptorHeap>();
}
