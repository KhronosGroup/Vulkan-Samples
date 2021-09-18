/* Copyright (c) 2021, Sascha Willems
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
 * Separate samplers and image to draw a single image with different sampling options
 */

#include "separate_image_sampler.h"

SeparateImageSampler::SeparateImageSampler()
{
	zoom     = -0.5f;
	rotation = {45.0f, 0.0f, 0.0f};
	title    = "Separate sampler and image";
}

SeparateImageSampler::~SeparateImageSampler()
{
	if (device)
	{
		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class

		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);

		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), base_descriptor_set_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), sampler_descriptor_set_layout, nullptr);
		for (VkSampler sampler : samplers)
		{
			vkDestroySampler(get_device().get_handle(), sampler, nullptr);
		}
		// Delete the implicitly created sampler for the texture loaded via the framework
		vkDestroySampler(get_device().get_handle(), texture.sampler, nullptr);
	}

	vertex_buffer.reset();
	index_buffer.reset();
	uniform_buffer_vs.reset();
}

// Enable physical device features required for this example
void SeparateImageSampler::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

void SeparateImageSampler::build_command_buffers()
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

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		// Set target frame buffer
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		// Bind the uniform buffer and sampled image to set 0
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &base_descriptor_set, 0, nullptr);
		// Bind the selected sampler to set 1
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &sampler_descriptor_sets[selected_sampler], 0, nullptr);
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

void SeparateImageSampler::setup_samplers()
{
	// Create two samplers with different options
	VkSamplerCreateInfo samplerCI = vkb::initializers::sampler_create_info();
	samplerCI.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCI.addressModeU        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.addressModeV        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.addressModeW        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.mipLodBias          = 0.0f;
	samplerCI.compareOp           = VK_COMPARE_OP_NEVER;
	samplerCI.minLod              = 0.0f;
	samplerCI.maxLod              = (float) texture.image->get_mipmaps().size();
	if (get_device().get_gpu().get_features().samplerAnisotropy)
	{
		// Use max. level of anisotropy for this example
		samplerCI.maxAnisotropy    = get_device().get_gpu().get_properties().limits.maxSamplerAnisotropy;
		samplerCI.anisotropyEnable = VK_TRUE;
	}
	else
	{
		// The device does not support anisotropic filtering
		samplerCI.maxAnisotropy    = 1.0;
		samplerCI.anisotropyEnable = VK_FALSE;
	}
	samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	// First sampler with linear filtering
	samplerCI.magFilter = VK_FILTER_LINEAR;
	samplerCI.minFilter = VK_FILTER_LINEAR;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &samplerCI, nullptr, &samplers[0]));

	// Second sampler with nearest filtering
	samplerCI.magFilter = VK_FILTER_NEAREST;
	samplerCI.minFilter = VK_FILTER_NEAREST;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &samplerCI, nullptr, &samplers[1]));
}

void SeparateImageSampler::load_assets()
{
	texture = load_texture("textures/metalplate01_rgba.ktx");
}

void SeparateImageSampler::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void SeparateImageSampler::generate_quad()
{
	// Setup vertices for a single uv-mapped quad made from two triangles
	std::vector<VertexStructure> vertices =
	    {
	        {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	        {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

	// Setup indices
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

void SeparateImageSampler::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_SAMPLER, 2)};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        3);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void SeparateImageSampler::setup_descriptor_set_layout()
{
	// We separate the descriptor sets for the uniform buffer + image and samplers, so we don't need to duplicate the descriptors for the former
	VkDescriptorSetLayoutCreateInfo           descriptor_layout_create_info{};
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings{};

	// Set layout for the uniform buffer and the image
	set_layout_bindings = {
	    // Binding 0 : Vertex shader uniform buffer
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_VERTEX_BIT,
	        0),
	    // Binding 1 : Fragment shader sampled image
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	        VK_SHADER_STAGE_FRAGMENT_BIT,
	        1)};
	descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(
	        set_layout_bindings.data(),
	        static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &base_descriptor_set_layout));

	// Set layout for the samplers
	set_layout_bindings = {
	    // Binding 0: Fragment shader sampler
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_SAMPLER,
	        VK_SHADER_STAGE_FRAGMENT_BIT,
	        0)};
	descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(
	        set_layout_bindings.data(),
	        static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &sampler_descriptor_set_layout));

	// Pipeline layout
	// Set layout for the base descriptors in set 0 and set layout for the sampler descriptors in set 1
	std::vector<VkDescriptorSetLayout> set_layouts = {base_descriptor_set_layout, sampler_descriptor_set_layout};
	VkPipelineLayoutCreateInfo         pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        set_layouts.data(),
	        static_cast<uint32_t>(set_layouts.size()));
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void SeparateImageSampler::setup_descriptor_set()
{
	// @todo: comment, compare to combined setup (or better do this in the readme.md)

	// We separate the descriptor sets for the uniform buffer + image and samplers, so we don't need to duplicate the descriptors for the former
	VkDescriptorSetAllocateInfo descriptor_set_alloc_info{};

	// Descriptors set for the uniform buffer and the image
	descriptor_set_alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &base_descriptor_set_layout,
	        1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_alloc_info, &base_descriptor_set));

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniform_buffer_vs);

    // Image info only references the image
	VkDescriptorImageInfo image_info{};
	image_info.imageView   = texture.image->get_vk_image_view().get_handle();
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Sampled image descriptor
	VkWriteDescriptorSet image_write_descriptor_set{};
	image_write_descriptor_set.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	image_write_descriptor_set.dstSet          = base_descriptor_set;
	image_write_descriptor_set.dstBinding      = 1;
	image_write_descriptor_set.descriptorCount = 1;
	image_write_descriptor_set.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	image_write_descriptor_set.pImageInfo      = &image_info;

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    // Binding 0 : Vertex shader uniform buffer
	    vkb::initializers::write_descriptor_set(
	        base_descriptor_set,
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        0,
	        &buffer_descriptor),
	    // Binding 1 : Fragment shader sampled image
		image_write_descriptor_set};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

	// Sets for each of the sampler
	descriptor_set_alloc_info.pSetLayouts = &sampler_descriptor_set_layout;
	for (size_t i = 0; i < sampler_descriptor_sets.size(); i++)
	{
		VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_alloc_info, &sampler_descriptor_sets[i]));

        // Descriptor info only references the sampler
		VkDescriptorImageInfo sampler_info{};
		sampler_info.sampler = samplers[i];

		VkWriteDescriptorSet sampler_write_descriptor_set{};
		sampler_write_descriptor_set.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		sampler_write_descriptor_set.dstSet          = sampler_descriptor_sets[i];
		sampler_write_descriptor_set.dstBinding      = 0;
		sampler_write_descriptor_set.descriptorCount = 1;
		sampler_write_descriptor_set.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
		sampler_write_descriptor_set.pImageInfo      = &sampler_info;

		vkUpdateDescriptorSets(get_device().get_handle(), 1, &sampler_write_descriptor_set, 0, nullptr);
	}
}

void SeparateImageSampler::prepare_pipelines()
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
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	// Load shaders
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

	shader_stages[0] = load_shader("separate_image_sampler/separate_image_sampler.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("separate_image_sampler/separate_image_sampler.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(VertexStructure), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexStructure, pos)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexStructure, uv)),
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexStructure, normal)),
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
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
	pipeline_create_info.stageCount          = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

// Prepare and initialize uniform buffer containing shader uniforms
void SeparateImageSampler::prepare_uniform_buffers()
{
	// Vertex shader uniform buffer block
	uniform_buffer_vs = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                        sizeof(ubo_vs),
	                                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void SeparateImageSampler::update_uniform_buffers()
{
	// Vertex shader
	ubo_vs.projection     = glm::perspective(glm::radians(60.0f), (float) width / (float) height, 0.001f, 256.0f);
	glm::mat4 view_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));

	ubo_vs.model = view_matrix * glm::translate(glm::mat4(1.0f), camera_pos);
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo_vs.view_pos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);

	uniform_buffer_vs->convert_and_update(ubo_vs);
}

bool SeparateImageSampler::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}
	load_assets();
	generate_quad();
	prepare_uniform_buffers();
	setup_samplers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();
	prepared = true;
	return true;
}

void SeparateImageSampler::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
}

void SeparateImageSampler::view_changed()
{
	update_uniform_buffers();
}

void SeparateImageSampler::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		const std::vector<std::string> sampler_names = {"Linear filtering",
		                                                "Nearest filtering"};
		if (drawer.combo_box("Sampler", &selected_sampler, sampler_names))
		{
			update_uniform_buffers();
		}
	}
}

std::unique_ptr<vkb::Application> create_separate_image_sampler()
{
	return std::make_unique<SeparateImageSampler>();
}
