/* Copyright (c) 2023-2024, Google
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

#include "oit_linked_lists.h"
#include <algorithm>

OITLinkedLists::OITLinkedLists()
{
}

OITLinkedLists::~OITLinkedLists()
{
	if (!has_device())
	{
		return;
	}

	vkDestroyPipeline(get_device().get_handle(), combine_pipeline, nullptr);
	vkDestroyPipeline(get_device().get_handle(), background_pipeline, nullptr);
	vkDestroyPipeline(get_device().get_handle(), gather_pipeline, nullptr);
	vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);

	vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, VK_NULL_HANDLE);
	vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);

	destroy_sized_objects();

	instance_data.reset();
	scene_constants.reset();

	vkDestroySampler(get_device().get_handle(), background_texture.sampler, nullptr);
	object.reset();
}

bool OITLinkedLists::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.0f, 0.0f, -4.0f});
	camera.set_rotation({0.0f, 0.0f, 0.0f});
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	load_assets();
	create_constant_buffers();
	create_descriptors();
	create_sized_objects(width, height);
	create_pipelines();

	update_scene_constants();
	fill_instance_data();
	update_descriptors();
	build_command_buffers();

	prepared = true;
	return true;
}

bool OITLinkedLists::resize(const uint32_t width, const uint32_t height)
{
	if ((width != this->width) || (height != this->height))
	{
		destroy_sized_objects();
		create_sized_objects(width, height);
		update_descriptors();
	}
	ApiVulkanSample::resize(width, height);
	return true;
}

void OITLinkedLists::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}

	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();

	if (camera_auto_rotation)
	{
		camera.rotate({delta_time * 5.0f, delta_time * 5.0f, 0.0f});
	}
	update_scene_constants();
}

void OITLinkedLists::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	if (gpu.get_features().fragmentStoresAndAtomics)
	{
		gpu.get_mutable_requested_features().fragmentStoresAndAtomics = VK_TRUE;
	}
	else
	{
		throw std::runtime_error("This sample requires support for buffers and images stores and atomic operations in the fragment shader stage");
	}
}

void OITLinkedLists::on_update_ui_overlay(vkb::Drawer &drawer)
{
	drawer.checkbox("Sort fragments", &sort_fragments);
	drawer.checkbox("Camera auto-rotation", &camera_auto_rotation);
	drawer.slider_int("Sorted fragments per pixel", &sorted_fragment_count, kSortedFragmentMinCount, kSortedFragmentMaxCount);
	drawer.slider_float("Background grayscale", &background_grayscale, kBackgroundGrayscaleMin, kBackgroundGrayscaleMax);
}

void OITLinkedLists::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));
		{
			// Gather pass
			{
				render_pass_begin_info.framebuffer     = gather_framebuffer;
				render_pass_begin_info.renderPass      = gather_render_pass;
				render_pass_begin_info.clearValueCount = 0;

				vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
				{
					VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
					vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

					VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
					vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

					vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);

					vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, gather_pipeline);
					draw_model(object, draw_cmd_buffers[i], kInstanceCount);
				}
				vkCmdEndRenderPass(draw_cmd_buffers[i]);
			}

			VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
			vkb::image_layout_transition(
			    draw_cmd_buffers[i], linked_list_head_image->get_handle(),
			    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			    VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			    VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
			    subresource_range);

			// Combine pass
			{
				VkClearValue clear_values[2];
				clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
				clear_values[1].depthStencil = {0.0f, 0};

				render_pass_begin_info.framebuffer     = framebuffers[i];
				render_pass_begin_info.renderPass      = render_pass;
				render_pass_begin_info.clearValueCount = 2;
				render_pass_begin_info.pClearValues    = clear_values;

				vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
				{
					VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
					vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

					VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
					vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

					vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);

					vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, background_pipeline);
					vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

					vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, combine_pipeline);
					vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

					draw_ui(draw_cmd_buffers[i]);
				}
				vkCmdEndRenderPass(draw_cmd_buffers[i]);
			}
		}
		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

////////////////////////////////////////////////////////////////////////////////

void OITLinkedLists::create_sized_objects(const uint32_t width, const uint32_t height)
{
	create_gather_pass_objects(width, height);
	create_fragment_resources(width, height);
	clear_sized_resources();
}

void OITLinkedLists::destroy_sized_objects()
{
	vkDestroyFramebuffer(get_device().get_handle(), gather_framebuffer, nullptr);
	vkDestroyRenderPass(get_device().get_handle(), gather_render_pass, nullptr);

	fragment_counter.reset();
	fragment_buffer.reset();
	fragment_max_count = 0;
	linked_list_head_image_view.reset();
	linked_list_head_image.reset();
}

void OITLinkedLists::create_gather_pass_objects(const uint32_t width, const uint32_t height)
{
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.subpassCount           = 1;
	render_pass_create_info.pSubpasses             = &subpass;

	VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &gather_render_pass));

	VkFramebufferCreateInfo framebuffer_create_info = vkb::initializers::framebuffer_create_info();
	framebuffer_create_info.renderPass              = gather_render_pass;
	framebuffer_create_info.width                   = width;
	framebuffer_create_info.height                  = height;
	framebuffer_create_info.layers                  = 1;

	VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_create_info, nullptr, &gather_framebuffer));
}

void OITLinkedLists::create_fragment_resources(const uint32_t width, const uint32_t height)
{
	{
		const VkExtent3D image_extent = {width, height, 1};
		linked_list_head_image        = std::make_unique<vkb::core::Image>(get_device(), image_extent, VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_SAMPLE_COUNT_1_BIT);
		linked_list_head_image_view   = std::make_unique<vkb::core::ImageView>(*linked_list_head_image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32_UINT);
	}

	{
		fragment_max_count                  = width * height * kFragmentsPerPixelAverage;
		const uint32_t fragment_buffer_size = sizeof(glm::uvec3) * fragment_max_count;
		fragment_buffer                     = std::make_unique<vkb::core::Buffer>(get_device(), fragment_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	}

	{
		fragment_counter = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(glm::uint), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	}
}

void OITLinkedLists::clear_sized_resources()
{
	VkCommandBuffer             command_buffer;
	VkCommandBufferAllocateInfo command_buffer_allocate_info = vkb::initializers::command_buffer_allocate_info(cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	VK_CHECK(vkAllocateCommandBuffers(get_device().get_handle(), &command_buffer_allocate_info, &command_buffer));

	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();
	VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));
	{
		vkCmdFillBuffer(command_buffer, fragment_counter->get_handle(), 0, sizeof(glm::uint), 0);

		VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		vkb::image_layout_transition(
		    command_buffer, linked_list_head_image->get_handle(),
		    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
		    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		    subresource_range);

		VkClearColorValue linked_lists_clear_value;
		linked_lists_clear_value.uint32[0] = kLinkedListEndSentinel;
		linked_lists_clear_value.uint32[1] = kLinkedListEndSentinel;
		linked_lists_clear_value.uint32[2] = kLinkedListEndSentinel;
		linked_lists_clear_value.uint32[3] = kLinkedListEndSentinel;
		vkCmdClearColorImage(command_buffer, linked_list_head_image->get_handle(), VK_IMAGE_LAYOUT_GENERAL, &linked_lists_clear_value, 1, &subresource_range);
	}
	VK_CHECK(vkEndCommandBuffer(command_buffer));

	{
		VkSubmitInfo submit_info       = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers    = &command_buffer;
		VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
		VK_CHECK(vkQueueWaitIdle(queue));
	}

	vkFreeCommandBuffers(get_device().get_handle(), cmd_pool, 1, &command_buffer);
}

////////////////////////////////////////////////////////////////////////////////

void OITLinkedLists::load_assets()
{
	object             = load_model("scenes/geosphere.gltf");
	background_texture = load_texture("textures/vulkan_logo_full.ktx", vkb::sg::Image::Color);
}

void OITLinkedLists::create_constant_buffers()
{
	scene_constants = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(SceneConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	instance_data   = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(Instance) * kInstanceCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void OITLinkedLists::create_descriptors()
{
	{
		std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
		};
		VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout));
	}

	{
		std::vector<VkDescriptorPoolSize> pool_sizes = {
		    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
		    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1),
		    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2),
		    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
		};
		const uint32_t             num_descriptor_sets         = 1;
		VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
		VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
	}

	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));
}

void OITLinkedLists::update_descriptors()
{
	VkDescriptorBufferInfo scene_constants_descriptor             = create_descriptor(*scene_constants);
	VkDescriptorBufferInfo instance_data_descriptor               = create_descriptor(*instance_data);
	VkDescriptorImageInfo  linked_list_head_image_view_descriptor = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, linked_list_head_image_view->get_handle(), VK_IMAGE_LAYOUT_GENERAL);
	VkDescriptorBufferInfo fragment_buffer_descriptor             = create_descriptor(*fragment_buffer);
	VkDescriptorBufferInfo fragment_counter_descriptor            = create_descriptor(*fragment_counter);
	VkDescriptorImageInfo  background_texture_descriptor          = create_descriptor(background_texture);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &scene_constants_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &instance_data_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2, &linked_list_head_image_view_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &fragment_buffer_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, &fragment_counter_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &background_texture_descriptor),
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
}

void OITLinkedLists::create_pipelines()
{
	{
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);
		VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
	}

	{
		VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();

		VkPipelineInputAssemblyStateCreateInfo input_assembly_state = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

		VkPipelineRasterizationStateCreateInfo rasterization_state = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

		VkPipelineColorBlendAttachmentState blend_attachment_state = vkb::initializers::pipeline_color_blend_attachment_state(0xF, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo color_blend_state      = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

		VkPipelineMultisampleStateCreateInfo multisample_state = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

		VkPipelineViewportStateCreateInfo viewport_state = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

		VkPipelineDepthStencilStateCreateInfo depth_stencil_state = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_GREATER);

		std::vector<VkDynamicState>      dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		VkPipelineDynamicStateCreateInfo dynamicState          = vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables.data(), static_cast<uint32_t>(dynamic_state_enables.size()), 0);

		std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

		VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout, render_pass, 0);
		pipeline_create_info.pVertexInputState            = &vertex_input_state;
		pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
		pipeline_create_info.pRasterizationState          = &rasterization_state;
		pipeline_create_info.pColorBlendState             = &color_blend_state;
		pipeline_create_info.pMultisampleState            = &multisample_state;
		pipeline_create_info.pViewportState               = &viewport_state;
		pipeline_create_info.pDepthStencilState           = &depth_stencil_state;
		pipeline_create_info.pDynamicState                = &dynamicState;
		pipeline_create_info.stageCount                   = static_cast<uint32_t>(shader_stages.size());
		pipeline_create_info.pStages                      = shader_stages.data();

		{
			const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
			    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
			};
			const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
			    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),
			};
			vertex_input_state.vertexBindingDescriptionCount   = static_cast<uint32_t>(vertex_input_bindings.size());
			vertex_input_state.pVertexBindingDescriptions      = vertex_input_bindings.data();
			vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes.size());
			vertex_input_state.pVertexAttributeDescriptions    = vertex_input_attributes.data();

			shader_stages[0] = load_shader("oit_linked_lists/gather.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shader_stages[1] = load_shader("oit_linked_lists/gather.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

			pipeline_create_info.renderPass = gather_render_pass;

			VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &gather_pipeline));
		}

		{
			vertex_input_state.vertexBindingDescriptionCount   = 0;
			vertex_input_state.pVertexBindingDescriptions      = nullptr;
			vertex_input_state.vertexAttributeDescriptionCount = 0;
			vertex_input_state.pVertexAttributeDescriptions    = nullptr;

			shader_stages[0] = load_shader("oit_linked_lists/fullscreen.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shader_stages[1] = load_shader("oit_linked_lists/background.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

			pipeline_create_info.renderPass = render_pass;

			VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &background_pipeline));
		}

		{
			vertex_input_state.vertexBindingDescriptionCount   = 0;
			vertex_input_state.pVertexBindingDescriptions      = nullptr;
			vertex_input_state.vertexAttributeDescriptionCount = 0;
			vertex_input_state.pVertexAttributeDescriptions    = nullptr;

			blend_attachment_state.blendEnable         = VK_TRUE;
			blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
			blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;

			shader_stages[0] = load_shader("oit_linked_lists/combine.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shader_stages[1] = load_shader("oit_linked_lists/combine.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

			pipeline_create_info.renderPass = render_pass;

			VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &combine_pipeline));
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void OITLinkedLists::update_scene_constants()
{
	SceneConstants constants        = {};
	constants.projection            = camera.matrices.perspective;
	constants.view                  = camera.matrices.view;
	constants.background_grayscale  = background_grayscale;
	constants.sort_fragments        = sort_fragments ? 1U : 0U;
	constants.fragment_max_count    = fragment_max_count;
	constants.sorted_fragment_count = sorted_fragment_count;
	scene_constants->convert_and_update(constants);
}

void OITLinkedLists::fill_instance_data()
{
	Instance instances[kInstanceCount] = {};

	auto get_random_float = []() {
		return static_cast<float>(rand()) / (RAND_MAX);
	};

	for (uint32_t l = 0, instance_index = 0; l < kInstanceLayerCount; ++l)
	{
		for (uint32_t c = 0; c < kInstanceColumnCount; ++c)
		{
			for (uint32_t r = 0; r < kInstanceRowCount; ++r, ++instance_index)
			{
				const float x     = static_cast<float>(r) - ((kInstanceRowCount - 1) * 0.5f);
				const float y     = static_cast<float>(c) - ((kInstanceColumnCount - 1) * 0.5f);
				const float z     = static_cast<float>(l) - ((kInstanceLayerCount - 1) * 0.5f);
				const float scale = 0.02f;
				instances[instance_index].model =
				    glm::scale(
				        glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)),
				        glm::vec3(scale));

				instances[instance_index].color.r = get_random_float();
				instances[instance_index].color.g = get_random_float();
				instances[instance_index].color.b = get_random_float();
				instances[instance_index].color.a = get_random_float() * 0.8f + 0.2f;
			}
		}
	}

	instance_data->convert_and_update(instances);
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_oit_linked_lists()
{
	return std::make_unique<OITLinkedLists>();
}
