/* Copyright (c) 2026, Arm Limited and Contributors
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

#include "rasterization_order_attachment_access.h"

// ============================================================================
// Initialization
// ============================================================================

// Sets up sample title and required Vulkan extensions
RasterizationOrderAttachmentAccess::RasterizationOrderAttachmentAccess()
{
	title = "Rasterization Order Attachment Access";

	// Request extensions
	add_device_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
	add_device_extension(VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME);
	add_device_extension(VK_EXT_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_EXTENSION_NAME, /*optional=*/true);
	add_device_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

	// Dynamic rendering doesn't use render passes.
	render_pass = VK_NULL_HANDLE;
}

// Enables specific features within the requested extensions
void RasterizationOrderAttachmentAccess::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceDynamicRenderingFeaturesKHR, dynamicRendering);
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR, dynamicRenderingLocalRead);
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceSynchronization2FeaturesKHR, synchronization2);

	// ROAA is optional. Only the barrier fallback path runs on devices that don't support it
	if (gpu.is_extension_supported(VK_EXT_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_EXTENSION_NAME))
	{
		roaa_supported = REQUEST_OPTIONAL_FEATURE(gpu, VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT, rasterizationOrderColorAttachmentAccess);
		roaa_enabled   = roaa_supported;
	}
}

// Initializes camera, loads assets, creates Vulkan resources, and records command buffers
bool RasterizationOrderAttachmentAccess::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	// Add INPUT_ATTACHMENT_BIT so fragment shaders can read the color attachment with subpassLoad()
	update_swapchain_image_usage_flags({VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT});

	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.0f, 0.0f, -4.0f});
	camera.set_rotation({0.0f, 0.0f, 0.0f});
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	load_assets();
	create_buffers();
	create_descriptors();
	create_pipelines();
	create_timestamp_query_pool();

	update_scene_uniforms();
	update_instance_buffer();

	build_command_buffers();

	prepared = true;
	return true;
}

// Loads sphere geometry and background texture from disk
void RasterizationOrderAttachmentAccess::load_assets()
{
	sphere_mesh        = load_model("scenes/geosphere.gltf");
	background_texture = load_texture("textures/vulkan_logo_full.ktx", vkb::sg::Image::Color);
}

// Creates uniform buffer for scene data and instance buffer with randomized sphere transforms/colors
void RasterizationOrderAttachmentAccess::create_buffers()
{
	scene_uniform_buffer = std::make_unique<vkb::core::BufferC>(
	    get_device(), sizeof(SceneUniforms),
	    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	instance_buffer = std::make_unique<vkb::core::BufferC>(
	    get_device(), sizeof(InstanceData) * kInstanceCount,
	    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Initialize instance data with random colors
	instances.resize(kInstanceCount);
	srand(42);

	const auto random_float = []() {
		return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	};

	uint32_t idx = 0;
	for (uint32_t z = 0; z < kInstanceLayerCount; ++z)
	{
		for (uint32_t y = 0; y < kInstanceColumnCount; ++y)
		{
			for (uint32_t x = 0; x < kInstanceRowCount; ++x, ++idx)
			{
				const glm::vec3 pos = {
				    static_cast<float>(x) - (kInstanceRowCount - 1) * 0.5f,
				    static_cast<float>(y) - (kInstanceColumnCount - 1) * 0.5f,
				    static_cast<float>(z) - (kInstanceLayerCount - 1) * 0.5f};

				instances[idx].model = glm::scale(
				    glm::translate(glm::mat4(1.0f), pos),
				    glm::vec3(0.03f));

				instances[idx].color = glm::vec4(
				    random_float(),
				    random_float(),
				    random_float(),
				    random_float() * 0.8f + 0.2f);
			}
		}
	}
}

// Creates descriptor set layout, pool, and writes descriptors for uniforms, texture, and input attachment
void RasterizationOrderAttachmentAccess::create_descriptors()
{
	if (!has_device())
	{
		return;
	}

	const VkDevice device = get_device().get_handle();

	// Layout
	if (descriptor_set_layout == VK_NULL_HANDLE)
	{
		const std::vector<VkDescriptorSetLayoutBinding> bindings = {
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
		};
		const VkDescriptorSetLayoutCreateInfo layout_info = vkb::initializers::descriptor_set_layout_create_info(bindings.data(), static_cast<uint32_t>(bindings.size()));
		VK_CHECK(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout));
	}

	// Destroy old pool if recreating
	if (descriptor_pool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
		descriptor_pool = VK_NULL_HANDLE;
		descriptor_sets.clear();
	}

	// One descriptor set per swapchain image
	const uint32_t set_count = static_cast<uint32_t>(swapchain_buffers.size());

	// Pool
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, set_count),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, set_count),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, set_count),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, set_count),
	};
	VkDescriptorPoolCreateInfo pool_info = vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), set_count);
	VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool));

	// Allocate
	const std::vector<VkDescriptorSetLayout> layouts(set_count, descriptor_set_layout);
	descriptor_sets.resize(set_count);
	const VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, layouts.data(), set_count);
	VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()));

	// Write descriptors (shared resources are the same across all sets, but the input attachment differs per image)
	VkDescriptorBufferInfo scene_buffer_info    = create_descriptor(*scene_uniform_buffer);
	VkDescriptorBufferInfo instance_buffer_info = create_descriptor(*instance_buffer);
	VkDescriptorImageInfo  texture_info         = create_descriptor(background_texture);

	for (uint32_t i = 0; i < set_count; ++i)
	{
		VkDescriptorImageInfo input_attachment_info = {};
		input_attachment_info.imageView             = swapchain_buffers[i].view;
		input_attachment_info.imageLayout           = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR;

		const std::vector<VkWriteDescriptorSet> writes = {
		    vkb::initializers::write_descriptor_set(descriptor_sets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &scene_buffer_info),
		    vkb::initializers::write_descriptor_set(descriptor_sets[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &instance_buffer_info),
		    vkb::initializers::write_descriptor_set(descriptor_sets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &texture_info),
		    vkb::initializers::write_descriptor_set(descriptor_sets[i], VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 3, &input_attachment_info),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}
}

// Creates background pipeline and two blend pipelines, one with and one without ROAA flag.
void RasterizationOrderAttachmentAccess::create_pipelines()
{
	if (!has_device())
	{
		return;
	}

	const VkDevice device = get_device().get_handle();

	// Pipeline layout
	const VkPipelineLayoutCreateInfo layout_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);
	VK_CHECK(vkCreatePipelineLayout(device, &layout_info, nullptr, &pipeline_layout));

	const VkFormat color_format = get_render_context().get_swapchain().get_format();

	// Common pipeline state
	VkPipelineVertexInputStateCreateInfo         vertex_input_state   = vkb::initializers::pipeline_vertex_input_state_create_info();
	const VkPipelineInputAssemblyStateCreateInfo input_assembly_state = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	const VkPipelineRasterizationStateCreateInfo rasterization_state  = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	const VkPipelineColorBlendAttachmentState    blend_attachment     = vkb::initializers::pipeline_color_blend_attachment_state(0xF, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo          color_blend_state    = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment);
	const VkPipelineMultisampleStateCreateInfo   multisample_state    = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);
	const VkPipelineViewportStateCreateInfo      viewport_state       = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);
	const VkPipelineDepthStencilStateCreateInfo  depth_stencil_state  = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_GREATER);

	const std::vector<VkDynamicState>      dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	const VkPipelineDynamicStateCreateInfo dynamic_state  = vkb::initializers::pipeline_dynamic_state_create_info(dynamic_states.data(), static_cast<uint32_t>(dynamic_states.size()), 0);

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

	VkPipelineRenderingCreateInfoKHR rendering_info = {};
	rendering_info.sType                            = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	rendering_info.colorAttachmentCount             = 1;
	rendering_info.pColorAttachmentFormats          = &color_format;

	VkGraphicsPipelineCreateInfo pipeline_info = vkb::initializers::pipeline_create_info(pipeline_layout, VK_NULL_HANDLE, 0);
	pipeline_info.pNext                        = &rendering_info;
	pipeline_info.pVertexInputState            = &vertex_input_state;
	pipeline_info.pInputAssemblyState          = &input_assembly_state;
	pipeline_info.pRasterizationState          = &rasterization_state;
	pipeline_info.pColorBlendState             = &color_blend_state;
	pipeline_info.pMultisampleState            = &multisample_state;
	pipeline_info.pViewportState               = &viewport_state;
	pipeline_info.pDepthStencilState           = &depth_stencil_state;
	pipeline_info.pDynamicState                = &dynamic_state;
	pipeline_info.stageCount                   = static_cast<uint32_t>(shader_stages.size());
	pipeline_info.pStages                      = shader_stages.data();

	// Background pipeline (fullscreen quad, no vertex input)
	shader_stages[0] = load_shader("rasterization_order_attachment_access", "fullscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("rasterization_order_attachment_access", "background.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(device, pipeline_cache, 1, &pipeline_info, nullptr, &background_pipeline));

	// Blend pipelines (sphere geometry with local read)
	const VkVertexInputBindingDescription   vertex_binding   = vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
	const VkVertexInputAttributeDescription vertex_attribute = vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);

	vertex_input_state.vertexBindingDescriptionCount   = 1;
	vertex_input_state.pVertexBindingDescriptions      = &vertex_binding;
	vertex_input_state.vertexAttributeDescriptionCount = 1;
	vertex_input_state.pVertexAttributeDescriptions    = &vertex_attribute;

	shader_stages[0] = load_shader("rasterization_order_attachment_access", "blend.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("rasterization_order_attachment_access", "blend.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Chain input attachment mapping so fragment shaders can read color attachment at index 0
	rendering_info.pNext = &INPUT_ATTACHMENT_INDEX_INFO;

	// Both blend pipelines use kBlendCullMode with VK_CULL_MODE_BACK_BIT set as the default
	VkPipelineRasterizationStateCreateInfo rasterization_state_blend = rasterization_state;
	rasterization_state_blend.cullMode                               = kBlendCullMode;
	pipeline_info.pRasterizationState                                = &rasterization_state_blend;

	// With ROAA
	if (roaa_supported)
	{
		color_blend_state.flags = VK_PIPELINE_COLOR_BLEND_STATE_CREATE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_BIT_EXT;
		VK_CHECK(vkCreateGraphicsPipelines(device, pipeline_cache, 1, &pipeline_info, nullptr, &blend_pipeline_roaa));
		color_blend_state.flags = 0;
	}

	// Without ROAA
	VK_CHECK(vkCreateGraphicsPipelines(device, pipeline_cache, 1, &pipeline_info, nullptr, &blend_pipeline_no_roaa));
}

// Tries to create a query pool for GPU timestamp measurements
void RasterizationOrderAttachmentAccess::create_timestamp_query_pool()
{
	if (!has_device())
	{
		return;
	}

	const VkDevice                    device     = get_device().get_handle();
	const VkPhysicalDeviceProperties &properties = get_device().get_gpu().get_properties();

	// Check if GPU supports timestamp queries (timestampPeriod of 0 means unsupported)
	if (properties.limits.timestampPeriod == 0)
	{
		LOGW("Timestamp queries not supported - GPU timing disabled");
		return;
	}

	timestamp_results.resize(2);

	VkQueryPoolCreateInfo info = {};
	info.sType                 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	info.queryType             = VK_QUERY_TYPE_TIMESTAMP;
	info.queryCount            = 2;
	VK_CHECK(vkCreateQueryPool(device, &info, nullptr, &timestamp_query_pool));
}

// ============================================================================
// Runtime
// ============================================================================

// Called each frame to update and submit command buffers
void RasterizationOrderAttachmentAccess::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}

	update_scene_uniforms();

	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();

	retrieve_timestamp_results();
}

// Called on window resize
bool RasterizationOrderAttachmentAccess::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	create_descriptors();
	rebuild_command_buffers();
	return true;
}

// Called each frame to render the UI overlay
void RasterizationOrderAttachmentAccess::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (roaa_supported)
	{
		if (drawer.checkbox("Enable ROAA", &roaa_enabled))
		{
			rebuild_command_buffers();
		}
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "ROAA not supported on this device");
	}

	drawer.text("Draw calls: %u", draw_call_count);
	drawer.text("Barriers: %u", barrier_count);
	if (supports_timestamp_queries())
	{
		drawer.text("GPU time: %.3f ms", gpu_draw_time_ms);
	}
}

// Called to record command buffers
void RasterizationOrderAttachmentAccess::build_command_buffers()
{
	if (roaa_enabled)
	{
		draw_call_count = 2;        // 1 background + 1 instanced spheres
		barrier_count   = 0;
	}
	else
	{
		draw_call_count = kInstanceCount + 1;        // 1 background + N spheres
		barrier_count   = kInstanceCount - 1;
	}

	const VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	for (uint32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));
		{
			// Reset timestamp queries before use
			if (supports_timestamp_queries())
			{
				vkCmdResetQueryPool(draw_cmd_buffers[i], timestamp_query_pool, 0, static_cast<uint32_t>(timestamp_results.size()));
			}

			// Transition: UNDEFINED -> RENDERING_LOCAL_READ_KHR (enables framebuffer fetch)
			vkb::image_layout_transition(
			    draw_cmd_buffers[i], swapchain_buffers[i].image,
			    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			    0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR,
			    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

			// Set up color attachment for dynamic rendering with local read support
			VkRenderingAttachmentInfoKHR color_attachment = {};
			color_attachment.sType                        = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
			color_attachment.imageView                    = swapchain_buffers[i].view;
			color_attachment.imageLayout                  = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR;
			color_attachment.loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
			color_attachment.storeOp                      = VK_ATTACHMENT_STORE_OP_STORE;
			color_attachment.clearValue.color             = {{0.0f, 0.0f, 0.0f, 0.0f}};

			VkRenderingInfoKHR rendering_info   = {};
			rendering_info.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
			rendering_info.renderArea.extent    = {width, height};
			rendering_info.layerCount           = 1;
			rendering_info.colorAttachmentCount = 1;
			rendering_info.pColorAttachments    = &color_attachment;

			// GPU timestamp: start (TOP_OF_PIPE captures time before any work begins)
			if (supports_timestamp_queries())
			{
				vkCmdWriteTimestamp(draw_cmd_buffers[i], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestamp_query_pool, 0);
			}

			// Begin dynamic rendering with local read layout for framebuffer fetch
			vkCmdBeginRenderingKHR(draw_cmd_buffers[i], &rendering_info);
			{
				// Set viewport and scissor to match swapchain dimensions
				const VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
				vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

				const VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
				vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

				// Set input attachment mapping for dynamic rendering local read
				vkCmdSetRenderingInputAttachmentIndicesKHR(draw_cmd_buffers[i], &INPUT_ATTACHMENT_INDEX_INFO);
				vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[i], 0, nullptr);

				// Draw background (fullscreen triangle - vertex shader generates positions)
				vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, background_pipeline);
				vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

				// Barrier: background writes -> sphere reads
				VkMemoryBarrier2KHR memory_barrier = {};
				memory_barrier.sType               = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR;
				memory_barrier.srcStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
				memory_barrier.dstStageMask        = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
				memory_barrier.srcAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
				memory_barrier.dstAccessMask       = VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT;

				VkDependencyInfoKHR dependency_info = {};
				dependency_info.sType               = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
				dependency_info.dependencyFlags     = VK_DEPENDENCY_BY_REGION_BIT;        // BY_REGION makes the barrier tile-local, which is more efficient on tiled GPUs
				dependency_info.memoryBarrierCount  = 1;
				dependency_info.pMemoryBarriers     = &memory_barrier;
				vkCmdPipelineBarrier2KHR(draw_cmd_buffers[i], &dependency_info);

				// ROAA ON: Single instanced draw. Extension guarantees fragment ordering.
				if (roaa_supported && roaa_enabled)
				{
					vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, blend_pipeline_roaa);
					draw_model(sphere_mesh, draw_cmd_buffers[i], kInstanceCount);
				}
				// ROAA OFF: Individual draws with barriers for correct ordering.
				else
				{
					vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, blend_pipeline_no_roaa);

					const VkDeviceSize offsets[1] = {0};
					vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, sphere_mesh->vertex_buffers.at("vertex_buffer").get(), offsets);
					vkCmdBindIndexBuffer(draw_cmd_buffers[i], sphere_mesh->index_buffer->get_handle(), 0, sphere_mesh->index_type);

					for (uint32_t inst = 0; inst < kInstanceCount; ++inst)
					{
						vkCmdDrawIndexed(draw_cmd_buffers[i], sphere_mesh->vertex_indices, 1, 0, 0, inst);

						if (inst < kInstanceCount - 1)
						{
							vkCmdPipelineBarrier2KHR(draw_cmd_buffers[i], &dependency_info);
						}
					}
				}
			}
			vkCmdEndRenderingKHR(draw_cmd_buffers[i]);

			// GPU timestamp: end (BOTTOM_OF_PIPE captures time after all work completes)
			if (supports_timestamp_queries())
			{
				vkCmdWriteTimestamp(draw_cmd_buffers[i], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestamp_query_pool, 1);
			}

			// Transition: RENDERING_LOCAL_READ_KHR -> COLOR_ATTACHMENT_OPTIMAL (required for UI rendering)
			vkb::image_layout_transition(
			    draw_cmd_buffers[i], swapchain_buffers[i].image,
			    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			    VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

			draw_ui(draw_cmd_buffers[i], i);

			// Transition: COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR
			vkb::image_layout_transition(
			    draw_cmd_buffers[i], swapchain_buffers[i].image,
			    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
			    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
		}
		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

// Uploads camera matrices and background settings to the uniform buffer
void RasterizationOrderAttachmentAccess::update_scene_uniforms()
{
	SceneUniforms uniforms        = {};
	uniforms.projection           = camera.matrices.perspective;
	uniforms.view                 = camera.matrices.view;
	uniforms.background_grayscale = 0.3f;
	scene_uniform_buffer->convert_and_update(uniforms);
}

// Uploads per-instance transform and color data to the GPU buffer
void RasterizationOrderAttachmentAccess::update_instance_buffer()
{
	instance_buffer->update(reinterpret_cast<const uint8_t *>(instances.data()), sizeof(InstanceData) * kInstanceCount);
}

// Reads GPU timestamp query results and calculates draw time in milliseconds
void RasterizationOrderAttachmentAccess::retrieve_timestamp_results()
{
	if (!has_device())
	{
		return;
	}

	if (timestamp_query_pool == VK_NULL_HANDLE)
	{
		return;
	}

	// WAIT_BIT blocks until results are available. 64_BIT for full precision timestamps
	const VkDevice device = get_device().get_handle();
	vkGetQueryPoolResults(
	    device,
	    timestamp_query_pool,
	    0, 2,
	    timestamp_results.size() * sizeof(uint64_t),
	    timestamp_results.data(),
	    sizeof(uint64_t),
	    VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

	// timestampPeriod is nanoseconds per tick, convert delta to milliseconds
	const VkPhysicalDeviceProperties &properties = get_device().get_gpu().get_properties();
	const float                       period     = properties.limits.timestampPeriod;
	gpu_draw_time_ms                             = static_cast<float>(timestamp_results[1] - timestamp_results[0]) * period / 1000000.0f;
}

// ============================================================================
// Teardown
// ============================================================================

// Destroys all Vulkan resources created by the sample
RasterizationOrderAttachmentAccess::~RasterizationOrderAttachmentAccess()
{
	if (!has_device())
	{
		return;
	}

	const VkDevice device = get_device().get_handle();

	vkDestroyQueryPool(device, timestamp_query_pool, nullptr);

	vkDestroyPipeline(device, blend_pipeline_roaa, nullptr);
	vkDestroyPipeline(device, blend_pipeline_no_roaa, nullptr);
	vkDestroyPipeline(device, background_pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipeline_layout, nullptr);

	vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);

	instance_buffer.reset();
	scene_uniform_buffer.reset();

	vkDestroySampler(device, background_texture.sampler, nullptr);
	background_texture.image.reset();
	sphere_mesh.reset();
}

// ============================================================================
// Factory
// ============================================================================

// Factory function to create a new sample instance
std::unique_ptr<vkb::VulkanSampleC> create_rasterization_order_attachment_access()
{
	return std::make_unique<RasterizationOrderAttachmentAccess>();
}
