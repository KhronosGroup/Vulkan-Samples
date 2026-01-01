/* Copyright (c) 2025, Arm Limited and Contributors
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

#include "fragment_density_map.h"
#include "api_vulkan_sample.h"
#include <string>
#include <vulkan/vulkan_core.h>

template <typename THandle>
static uint64_t get_object_handle(THandle object)
{
	using UintHandle = typename std::conditional<sizeof(THandle) == sizeof(uint32_t), uint32_t, uint64_t>::type;
	return static_cast<uint64_t>(reinterpret_cast<UintHandle>(object));
}

void FragmentDensityMap::destroy_image(ApiVulkanSample::ImageData &image_data)
{
	VkDevice device_handle = get_device().get_handle();
	vkDestroyImageView(device_handle, image_data.view, nullptr);
	vkDestroyImage(device_handle, image_data.image, nullptr);
	vkFreeMemory(device_handle, image_data.mem, nullptr);
	image_data.view  = VK_NULL_HANDLE;
	image_data.image = VK_NULL_HANDLE;
	image_data.mem   = VK_NULL_HANDLE;
}

void FragmentDensityMap::destroy_pipeline(FragmentDensityMap::PipelineData &pipeline_data)
{
	VkDevice device_handle = get_device().get_handle();
	vkDestroyPipeline(device_handle, pipeline_data.pipeline, nullptr);
	vkDestroyPipelineLayout(device_handle, pipeline_data.pipeline_layout, nullptr);
	vkDestroyDescriptorSetLayout(device_handle, pipeline_data.set_layout, nullptr);
	pipeline_data.pipeline        = VK_NULL_HANDLE;
	pipeline_data.pipeline_layout = VK_NULL_HANDLE;
	pipeline_data.set_layout      = VK_NULL_HANDLE;
}

FragmentDensityMap::FragmentDensityMap()
{
	title = "Fragment Density Map";
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
	add_device_extension(VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME);
}

FragmentDensityMap::~FragmentDensityMap()
{
	if (has_device())
	{
		VkDevice device_handle = get_device().get_handle();
		vkDestroyDescriptorPool(device_handle, main_pass.descriptor_pool, nullptr);

		destroy_pipeline(present.pipeline);
		destroy_pipeline(main_pass.sky_pipeline);
		destroy_pipeline(main_pass.meshes.pipeline);
		destroy_pipeline(fdm.generate.pipeline);

		vkDestroyRenderPass(device_handle, present.render_pass, nullptr);
		vkDestroyRenderPass(device_handle, fdm.generate.render_pass, nullptr);

		destroy_image(main_pass.image);
		destroy_image(fdm.image);

		vkDestroySampler(device_handle, samplers.nearest, nullptr);
		vkDestroySampler(device_handle, samplers.subsampled_nearest, nullptr);

		vkDestroyFramebuffer(device_handle, main_pass.framebuffer, nullptr);
		vkDestroyFramebuffer(device_handle, fdm.generate.framebuffer, nullptr);
	}
}

bool FragmentDensityMap::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	last_options = current_options;

	setup_samplers();
	load_assets();

	setup_descriptor_pool_main_pass();
	prepare_uniform_buffers_main_pass();
	setup_descriptor_set_layout_main_pass();
	setup_descriptor_set_main_pass();

	reset_fdm_gpu_data();

	prepared = true;
	return true;
}

void FragmentDensityMap::setup_samplers()
{
	// Samplers are not affected by configuration settings.
	// They are created once and reused across all configurations.
	assert(samplers.subsampled_nearest == VK_NULL_HANDLE);
	assert(samplers.nearest == VK_NULL_HANDLE);

	// The sample needs to create a sampler using the subsampled flag to interact with the FDM attachments.
	VkSamplerCreateInfo sampler_create_info{
	    .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	    .pNext                   = nullptr,
	    .flags                   = 0u,
	    .magFilter               = VK_FILTER_NEAREST,
	    .minFilter               = VK_FILTER_NEAREST,
	    .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST,
	    .addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	    .addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	    .addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	    .mipLodBias              = 0.0f,
	    .anisotropyEnable        = VK_FALSE,
	    .compareEnable           = VK_FALSE,
	    .minLod                  = 0.0f,
	    .maxLod                  = 0.0f,
	    .borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
	    .unnormalizedCoordinates = VK_FALSE,
	};

	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_create_info, nullptr, &samplers.nearest));

	if (is_fdm_supported())
	{
		sampler_create_info.flags = VK_SAMPLER_CREATE_SUBSAMPLED_BIT_EXT;
		VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_create_info, nullptr, &samplers.subsampled_nearest));
	}
}

void FragmentDensityMap::prepare_pipelines()
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
	        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	        VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

	// Note: A reversed depth buffer is used for increased precision, so larger depth values are retained.
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

	// Specify that these states will be dynamic, i.e. not part of the pipeline state object.
	std::array<VkDynamicState, 2> dynamic_state_enables{
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        vkb::to_u32(dynamic_state_enables.size()),
	        0);

	VkDevice device_handle = get_device().get_handle();
	// Load our SPIR-V shaders.
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(
	        main_pass.meshes.pipeline.pipeline_layout,
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
	pipeline_create_info.pVertexInputState   = &vertex_input_state;

	// Generic forward render pipeline for the glTF-submeshes.
	{
		std::array<VkVertexInputBindingDescription, 3> binding_descriptions = {
		    // Binding point 0: Mesh vertex layout description at per-vertex rate.
		    vkb::initializers::vertex_input_binding_description(0, 3 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX),
		    vkb::initializers::vertex_input_binding_description(1, 3 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX),
		    vkb::initializers::vertex_input_binding_description(2, 2 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX)};

		std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions = {
		    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Location 0: Position.
		    vkb::initializers::vertex_input_attribute_description(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Location 1: Normal.
		    vkb::initializers::vertex_input_attribute_description(2, 2, VK_FORMAT_R32G32_SFLOAT, 0),           // Location 2: Texture coordinates.
		};
		vertex_input_state.pVertexBindingDescriptions      = binding_descriptions.data();
		vertex_input_state.pVertexAttributeDescriptions    = attribute_descriptions.data();
		vertex_input_state.vertexBindingDescriptionCount   = binding_descriptions.size();
		vertex_input_state.vertexAttributeDescriptionCount = attribute_descriptions.size();

		shader_stages[0] = load_shader("fragment_density_map/forward.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(
		    is_debug_fdm_enabled() ? "fragment_density_map/forward_debug.frag.spv" : "fragment_density_map/forward.frag.spv",
		    VK_SHADER_STAGE_FRAGMENT_BIT);

		pipeline_create_info.layout     = main_pass.meshes.pipeline.pipeline_layout;
		pipeline_create_info.renderPass = render_pass;
		vkDestroyPipeline(device_handle, main_pass.meshes.pipeline.pipeline, nullptr);
		VK_CHECK(vkCreateGraphicsPipelines(device_handle, pipeline_cache, 1, &pipeline_create_info, nullptr, &main_pass.meshes.pipeline.pipeline));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_PIPELINE, get_object_handle(main_pass.meshes.pipeline.pipeline), "Submeshes Pipeline");
	}

	VkPipelineShaderStageCreateInfo quad_uvw_shader_stage = load_shader("fragment_density_map/quad_uvw.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	VkPipelineShaderStageCreateInfo quad_uv_shader_stage  = load_shader("fragment_density_map/quad_uv.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);

	// Sky pipeline.
	{
		pipeline_create_info.renderPass      = render_pass;
		pipeline_create_info.layout          = main_pass.sky_pipeline.pipeline_layout;
		rasterization_state.cullMode         = VK_CULL_MODE_NONE;
		depth_stencil_state.depthWriteEnable = VK_FALSE;
		depth_stencil_state.depthTestEnable  = VK_FALSE;
		shader_stages[0]                     = quad_uvw_shader_stage;
		shader_stages[1]                     = load_shader(
            is_debug_fdm_enabled() ? "fragment_density_map/sky_debug.frag.spv" : "fragment_density_map/sky.frag.spv",
		    VK_SHADER_STAGE_FRAGMENT_BIT);

		// The vertex shader generates a full-screen quad procedurally.
		// No vertex buffers are required because the vertex positions are computed in the shader itself.
		vertex_input_state.vertexBindingDescriptionCount   = 0;
		vertex_input_state.vertexAttributeDescriptionCount = 0;
		vertex_input_state.pVertexBindingDescriptions      = nullptr;
		vertex_input_state.pVertexAttributeDescriptions    = nullptr;

		vkDestroyPipeline(device_handle, main_pass.sky_pipeline.pipeline, nullptr);
		VK_CHECK(vkCreateGraphicsPipelines(device_handle, pipeline_cache, 1, &pipeline_create_info, nullptr, &main_pass.sky_pipeline.pipeline));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_PIPELINE, get_object_handle(main_pass.sky_pipeline.pipeline), "Starfield Sky Pipeline");
	}

	// Present and UI pipeline.
	{
		// Vertex stage of the pipeline.
		shader_stages[0] = quad_uv_shader_stage;
		shader_stages[1] = load_shader("fragment_density_map/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		depth_stencil_state.depthWriteEnable = VK_FALSE;
		depth_stencil_state.depthTestEnable  = VK_FALSE;
		rasterization_state.cullMode         = VK_CULL_MODE_NONE;

		pipeline_create_info.layout     = present.pipeline.pipeline_layout;
		pipeline_create_info.renderPass = present.render_pass;

		vkDestroyPipeline(device_handle, present.pipeline.pipeline, nullptr);
		VK_CHECK(vkCreateGraphicsPipelines(device_handle, pipeline_cache, 1, &pipeline_create_info, nullptr, &present.pipeline.pipeline));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_PIPELINE, get_object_handle(present.pipeline.pipeline), "Present Pipeline");
	}

	// Generate FDM.
	{
		vkDestroyPipeline(device_handle, fdm.generate.pipeline.pipeline, nullptr);
		if (is_generate_fdm_compute())
		{
			// Generate FDM (compute).

			const VkPipelineShaderStageCreateInfo fdm_comp = load_shader("fragment_density_map/generate_density_map.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
			VkComputePipelineCreateInfo           pipeline_create_info{
			              .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			              .stage  = fdm_comp,
			              .layout = fdm.generate.pipeline.pipeline_layout,
            };
			VK_CHECK(vkCreateComputePipelines(device_handle, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &fdm.generate.pipeline.pipeline));
			debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_PIPELINE, get_object_handle(fdm.generate.pipeline.pipeline), "Generate FDM Pipeline (compute)");
		}
		else
		{
			// Generate FDM (fragment).

			pipeline_create_info.layout = fdm.generate.pipeline.pipeline_layout;

			pipeline_create_info.renderPass      = fdm.generate.render_pass;
			rasterization_state.cullMode         = VK_CULL_MODE_NONE;
			depth_stencil_state.depthWriteEnable = VK_FALSE;
			depth_stencil_state.depthTestEnable  = VK_FALSE;
			shader_stages[0]                     = quad_uv_shader_stage;
			shader_stages[1]                     = load_shader("fragment_density_map/generate_density_map.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

			VK_CHECK(vkCreateGraphicsPipelines(device_handle, pipeline_cache, 1, &pipeline_create_info, nullptr, &fdm.generate.pipeline.pipeline));
			debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_PIPELINE, get_object_handle(fdm.generate.pipeline.pipeline), "Generate FDM Pipeline (fragment)");
		}
	}
}

void FragmentDensityMap::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	std::array<VkClearValue, 3> clear_values{
	    VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 0.0f}}},        // Color output.
	    VkClearValue{.depthStencil = {0.0f, 0}},                  // Depth stencil output.
	    VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 0.0f}}},        // FDM input (LoadOP - clear value ignored).
	};

	// Begin the render pass.
	VkRenderPassBeginInfo render_pass_begin_info = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.clearValueCount       = is_fdm_enabled() ? clear_values.size() : (clear_values.size() - 1);
	render_pass_begin_info.pClearValues          = clear_values.data();

	assert((main_pass.extend.height > 0) && (main_pass.extend.width > 0));
	VkRect2D   main_scissor  = vkb::initializers::rect2D(main_pass.extend.width, main_pass.extend.height, 0, 0);
	VkViewport main_viewport = vkb::initializers::viewport(static_cast<float>(main_scissor.extent.width), static_cast<float>(main_scissor.extent.height), 0.0f, 1.0f);

	VkRect2D   present_scissor  = vkb::initializers::rect2D(get_render_context().get_surface_extent().width, get_render_context().get_surface_extent().height, 0, 0);
	VkViewport present_viewport = vkb::initializers::viewport(static_cast<float>(present_scissor.extent.width), static_cast<float>(present_scissor.extent.height), 0.0f, 1.0f);

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VkCommandBuffer cmd_buffer = draw_cmd_buffers[i];
		std::string     debug_name = fmt::format("Draw command buffer {}", i);
		debug_utils.set_debug_name(get_device().get_handle(), VK_OBJECT_TYPE_COMMAND_BUFFER, get_object_handle(cmd_buffer), debug_name.c_str());

		VK_CHECK(vkBeginCommandBuffer(cmd_buffer, &command_buffer_begin_info));

		if (is_update_fdm_enabled())
		{
			write_density_map(cmd_buffer);
		}

		// Main pass (forward).
		{
			debug_utils.cmd_begin_label(cmd_buffer, "Main pass (forward)", glm::vec4());

			render_pass_begin_info.clearValueCount   = is_fdm_enabled() ? clear_values.size() : (clear_values.size() - 1);
			render_pass_begin_info.renderPass        = render_pass;
			render_pass_begin_info.framebuffer       = main_pass.framebuffer;
			render_pass_begin_info.renderArea.extent = main_scissor.extent;

			vkCmdBeginRenderPass(cmd_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdSetViewport(cmd_buffer, 0, 1, &main_viewport);

			vkCmdSetScissor(cmd_buffer, 0, 1, &main_scissor);

			// Sky
			{
				debug_utils.cmd_begin_label(cmd_buffer, "Sky", glm::vec4());
				vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, main_pass.sky_pipeline.pipeline);
				vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
				debug_utils.cmd_end_label(cmd_buffer);
			}

			// Main pass glTF-submeshes.
			{
				assert(scene_data.size() == main_pass.meshes.descriptor_sets.size());
				vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, main_pass.meshes.pipeline.pipeline);
				for (uint32_t mesh_idx = 0; mesh_idx < scene_data.size(); ++mesh_idx)
				{
					auto &mesh_data = scene_data[mesh_idx];

					debug_utils.cmd_begin_label(cmd_buffer, mesh_data.submesh->get_name().c_str(), glm::vec4());

					vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, main_pass.meshes.pipeline.pipeline_layout, 0, 1, &main_pass.meshes.descriptor_sets[mesh_idx], 0, NULL);

					auto                   &vertex_buffer  = mesh_data.submesh->vertex_buffers.at("position");
					auto                   &normal_buffer  = mesh_data.submesh->vertex_buffers.at("normal");
					auto                   &uv_buffer      = mesh_data.submesh->vertex_buffers.at("texcoord_0");
					std::array<VkBuffer, 3> vertex_buffers = {
					    vertex_buffer.get_handle(),
					    normal_buffer.get_handle(),
					    uv_buffer.get_handle()};
					std::array<VkDeviceSize, 3> vertex_offsets = {0, 0, 0};

					vkCmdBindVertexBuffers(cmd_buffer, 0,
					                       vertex_buffers.size(),
					                       vertex_buffers.data(),
					                       vertex_offsets.data());
					vkCmdBindIndexBuffer(cmd_buffer,
					                     mesh_data.submesh->index_buffer->get_handle(),
					                     mesh_data.submesh->index_offset,
					                     mesh_data.submesh->index_type);
					vkCmdDrawIndexed(cmd_buffer,
					                 mesh_data.submesh->vertex_indices,
					                 1, 0, 0, 0);

					debug_utils.cmd_end_label(cmd_buffer);
				}
			}
			vkCmdEndRenderPass(cmd_buffer);
			debug_utils.cmd_end_label(cmd_buffer);
		}

		// Present + UI.
		{
			debug_utils.cmd_begin_label(cmd_buffer, "Present+UI", glm::vec4());

			if (is_show_fdm_enabled())
			{
				VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

				vkb::image_layout_transition(cmd_buffer,
				                             fdm.image.image,
				                             VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT,
				                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				                             VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT,
				                             VK_ACCESS_SHADER_READ_BIT,
				                             VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT,
				                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				                             subresource_range);
			}
			render_pass_begin_info.renderArea.extent = present_scissor.extent;
			render_pass_begin_info.renderPass        = present.render_pass;
			render_pass_begin_info.framebuffer       = framebuffers[i];
			render_pass_begin_info.clearValueCount   = 1;
			// Copy to swap chain.
			{
				debug_utils.cmd_begin_label(cmd_buffer, "Copy", glm::vec4());
				vkCmdBeginRenderPass(cmd_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, present.pipeline.pipeline);
				vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, present.pipeline.pipeline_layout, 0, 1, &present.set, 0, nullptr);

				vkCmdSetViewport(cmd_buffer, 0, 1, &present_viewport);
				vkCmdSetScissor(cmd_buffer, 0, 1, &present_scissor);
				vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
				debug_utils.cmd_end_label(cmd_buffer);
			}
			// UI
			if (has_gui())
			{
				debug_utils.cmd_begin_label(cmd_buffer, "UI", glm::vec4());
				get_gui().draw(cmd_buffer);
				debug_utils.cmd_end_label(cmd_buffer);
			}

			vkCmdEndRenderPass(cmd_buffer);
			if (is_show_fdm_enabled() && !is_update_fdm_enabled())
			{
				VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
				vkb::image_layout_transition(cmd_buffer,
				                             fdm.image.image,
				                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				                             VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT,
				                             VK_ACCESS_SHADER_READ_BIT,
				                             VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT,
				                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				                             VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT,
				                             subresource_range);
			}
			debug_utils.cmd_end_label(cmd_buffer);
		}
		VK_CHECK(vkEndCommandBuffer(cmd_buffer));
	}
}

bool FragmentDensityMap::resize(const uint32_t _width, const uint32_t _height)
{
	// This sample needs to modify ApiVulkanSample::resize to handle the FDM.
	if (!prepared)
	{
		return false;
	}

	get_render_context().handle_surface_changes();

	// Don't recreate the swapchain if the dimensions haven't changed.
	if (width == get_render_context().get_surface_extent().width && height == get_render_context().get_surface_extent().height)
	{
		return false;
	}

	width  = get_render_context().get_surface_extent().width;
	height = get_render_context().get_surface_extent().height;

	prepared = false;

	// Ensure all operations on the device have been finished before destroying resources.
	get_device().wait_idle();

	create_swapchain_buffers();

	reset_fdm_gpu_data();

	if ((width > 0u) && (height > 0u))
	{
		if (has_gui())
		{
			get_gui().resize(width, height);
		}
	}

	rebuild_command_buffers();

	get_device().wait_idle();

	// Notify derived class.
	view_changed();

	prepared = true;
	return true;
}

void FragmentDensityMap::reset_fdm_gpu_data()
{
	vkResetCommandPool(get_device().get_handle(), cmd_pool, 0);

	last_options = current_options;
	setup_additional_descriptor_pool();
	prepare_uniform_buffers_fdm();

	setup_depth_stencil();

	setup_render_pass();
	setup_framebuffer();

	setup_descriptor_set_layout_fdm();
	setup_descriptor_set_fdm();

	setup_descriptor_set_layout_present();
	setup_descriptor_set_present();
	prepare_pipelines();

	if (!is_update_fdm_enabled() && is_fdm_enabled())
	{
		auto cmd_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		debug_utils.set_debug_name(get_device().get_handle(), VK_OBJECT_TYPE_COMMAND_BUFFER, get_object_handle(cmd_buffer), "Generate FDM command buffer");
		write_density_map(cmd_buffer);
		get_device().flush_command_buffer(cmd_buffer, queue, true, VK_NULL_HANDLE);
	}

	build_command_buffers();
}

void FragmentDensityMap::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	// Recreate resources if options changed.
	if (last_options != current_options)
	{
		prepared = false;
		get_device().wait_idle();
		reset_fdm_gpu_data();
		get_device().wait_idle();
		prepared = true;
	}

	// Submit current command buffer.
	{
		ApiVulkanSample::prepare_frame();
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
		VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
		ApiVulkanSample::submit_frame();
	}

	if (!paused || camera.updated)
	{
		update_uniform_buffer(delta_time);
	}
	if (has_gui() && is_show_stats())
	{
		update_stats(delta_time);
	}
}

void FragmentDensityMap::load_assets()
{
	vkb::GLTFLoader loader{get_device()};
	sg_scene = loader.read_scene_from_file("scenes/bonza/Bonza.gltf");
	assert(sg_scene);

	camera.type              = vkb::CameraType::FirstPerson;
	const float aspect_ratio = 1.0f;        // dummy value. Reset by update_extents.
	camera.set_perspective(50.0f, aspect_ratio, 4000.0f, 1.0f);
	camera.set_rotation(glm::vec3(230.0f, 101.0f, -5.0f));
	camera.set_translation(glm::vec3(115.0f, -390.0f, 18.0f));
	camera.translation_speed = 100.0f;

	// Store all data from glTF scene nodes in a vector.
	for (auto &mesh : sg_scene->get_components<vkb::sg::Mesh>())
	{
		for (auto &node : mesh->get_nodes())
		{
			for (auto &submesh : mesh->get_submeshes())
			{
				const vkb::sg::Material *mesh_material = submesh->get_material();
				if (mesh_material)
				{
					bool        negative_scale   = glm::any(glm::lessThanEqual(node->get_transform().get_scale(), glm::vec3(0.0f)));
					const auto &color_texture_it = mesh_material->textures.find("base_color_texture");
					// Cull double-sided/transparent/negatively-scaled/non-textured meshes.
					if (!negative_scale &&
					    !mesh_material->double_sided &&
					    mesh_material->alpha_mode == vkb::sg::AlphaMode::Opaque &&
					    color_texture_it != mesh_material->textures.end())
					{
						SubmeshData &mesh_data = scene_data.emplace_back(std::move(SubmeshData{
						    .submesh            = submesh,
						    .world_matrix       = node->get_transform().get_world_matrix(),
						    .base_color_texture = color_texture_it->second}));
					}
					else
					{
						LOGI("Ignoring glTF mesh <{}>", submesh->get_name());
					}
				}
			}
		}
	}
	assert(!scene_data.empty());
}

void FragmentDensityMap::setup_descriptor_pool_main_pass()
{
	assert(main_pass.descriptor_pool == VK_NULL_HANDLE);
	const uint32_t max_sets = vkb::to_u32(scene_data.size());

	std::array<VkDescriptorPoolSize, 2> pool_sizes =
	    {
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, vkb::to_u32(scene_data.size())),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, vkb::to_u32(scene_data.size())),
	    };

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        vkb::to_u32(pool_sizes.size()),
	        pool_sizes.data(), max_sets);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &main_pass.descriptor_pool));
	debug_utils.set_debug_name(get_device().get_handle(), VK_OBJECT_TYPE_DESCRIPTOR_POOL, get_object_handle(main_pass.descriptor_pool), "Main pass descriptor pool");
}

void FragmentDensityMap::setup_additional_descriptor_pool()
{
	vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, nullptr);
	const uint32_t max_sets = 2;        // generate_fdm + present.

	std::array<VkDescriptorPoolSize, 3> pool_sizes =
	    {
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1),
	    };

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        vkb::to_u32(pool_sizes.size()),
	        pool_sizes.data(), max_sets);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
	debug_utils.set_debug_name(get_device().get_handle(), VK_OBJECT_TYPE_DESCRIPTOR_POOL, get_object_handle(descriptor_pool), "Additional Descriptor Pool");
}

void FragmentDensityMap::setup_descriptor_set_layout_main_pass()
{
	VkDevice                        device_handle = get_device().get_handle();
	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info;
	VkPipelineLayoutCreateInfo      pipeline_layout_create_info;

	// Main pass glTF-submesh.
	{
		std::array<VkDescriptorSetLayoutBinding, 2> set_layout_bindings =
		    {
		        // Binding 0 : Vertex shader uniform buffer.
		        vkb::initializers::descriptor_set_layout_binding(
		            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		            VK_SHADER_STAGE_VERTEX_BIT,
		            0),
		        // Binding 1 : Fragment shader combined sampler.
		        vkb::initializers::descriptor_set_layout_binding(
		            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		            VK_SHADER_STAGE_FRAGMENT_BIT,
		            1),
		    };

		descriptor_layout_create_info =
		    vkb::initializers::descriptor_set_layout_create_info(
		        set_layout_bindings.data(),
		        vkb::to_u32(set_layout_bindings.size()));

		pipeline_layout_create_info =
		    vkb::initializers::pipeline_layout_create_info(
		        &main_pass.meshes.pipeline.set_layout, 1);

		assert(main_pass.meshes.pipeline.set_layout == VK_NULL_HANDLE);
		VK_CHECK(vkCreateDescriptorSetLayout(device_handle, &descriptor_layout_create_info, nullptr, &main_pass.meshes.pipeline.set_layout));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
		                           get_object_handle(main_pass.meshes.pipeline.set_layout), "Submeshes Descriptor Set Layout");

		assert(main_pass.meshes.pipeline.pipeline_layout == VK_NULL_HANDLE);
		VK_CHECK(vkCreatePipelineLayout(device_handle, &pipeline_layout_create_info, nullptr, &main_pass.meshes.pipeline.pipeline_layout));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_PIPELINE_LAYOUT,
		                           get_object_handle(main_pass.meshes.pipeline.pipeline_layout), "Submeshes Pipeline Layout");
	}

	// Sky
	{
		descriptor_layout_create_info =
		    vkb::initializers::descriptor_set_layout_create_info(nullptr, 0);

		pipeline_layout_create_info =
		    vkb::initializers::pipeline_layout_create_info(
		        &main_pass.sky_pipeline.set_layout, 1);

		assert(main_pass.sky_pipeline.set_layout == VK_NULL_HANDLE);
		VK_CHECK(vkCreateDescriptorSetLayout(device_handle, &descriptor_layout_create_info, nullptr, &main_pass.sky_pipeline.set_layout));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
		                           get_object_handle(main_pass.sky_pipeline.set_layout), "Sky Descriptor Set Layout");

		assert(main_pass.sky_pipeline.pipeline_layout == VK_NULL_HANDLE);
		VK_CHECK(vkCreatePipelineLayout(device_handle, &pipeline_layout_create_info, nullptr, &main_pass.sky_pipeline.pipeline_layout));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_PIPELINE_LAYOUT,
		                           get_object_handle(main_pass.sky_pipeline.pipeline_layout), "Sky Pipeline Layout");
	}
}

void FragmentDensityMap::setup_descriptor_set_layout_fdm()
{
	VkDescriptorSetLayoutCreateInfo             descriptor_layout_create_info;
	VkPipelineLayoutCreateInfo                  pipeline_layout_create_info;
	VkDevice                                    device_handle = get_device().get_handle();
	std::array<VkDescriptorSetLayoutBinding, 2> compute_set_layout_bindings =
	    {
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            VK_SHADER_STAGE_COMPUTE_BIT,
	            0),
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	            VK_SHADER_STAGE_COMPUTE_BIT,
	            1),
	    };
	std::array<VkDescriptorSetLayoutBinding, 1> fragment_set_layout_bindings =
	    {
	        // Binding 0: Fragment shader uniform buffer.
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            VK_SHADER_STAGE_FRAGMENT_BIT,
	            0)};

	if (is_generate_fdm_compute())
	{
		descriptor_layout_create_info =
		    vkb::initializers::descriptor_set_layout_create_info(
		        compute_set_layout_bindings.data(),
		        vkb::to_u32(compute_set_layout_bindings.size()));
	}
	else
	{
		descriptor_layout_create_info =
		    vkb::initializers::descriptor_set_layout_create_info(
		        fragment_set_layout_bindings.data(),
		        vkb::to_u32(fragment_set_layout_bindings.size()));
	}

	pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &fdm.generate.pipeline.set_layout, 1);

	vkDestroyDescriptorSetLayout(device_handle, fdm.generate.pipeline.set_layout, nullptr);
	VK_CHECK(vkCreateDescriptorSetLayout(device_handle, &descriptor_layout_create_info, nullptr, &fdm.generate.pipeline.set_layout));
	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
	                           get_object_handle(fdm.generate.pipeline.set_layout),
	                           is_generate_fdm_compute() ? "Generate FDM (Compute) Descriptor Set Layout" : "Generate FDM (Fragment) Descriptor Set Layout");

	vkDestroyPipelineLayout(device_handle, fdm.generate.pipeline.pipeline_layout, nullptr);
	VK_CHECK(vkCreatePipelineLayout(device_handle, &pipeline_layout_create_info, nullptr, &fdm.generate.pipeline.pipeline_layout));
	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_PIPELINE_LAYOUT,
	                           get_object_handle(fdm.generate.pipeline.pipeline_layout),
	                           is_generate_fdm_compute() ? "Generate FDM (Compute) Pipeline Layout" : "Generate FDM (Fragment) Pipeline Layout");
}

void FragmentDensityMap::setup_descriptor_set_layout_present()
{
	VkDescriptorSetLayoutCreateInfo             descriptor_layout_create_info;
	VkPipelineLayoutCreateInfo                  pipeline_layout_create_info;
	VkDevice                                    device_handle = get_device().get_handle();
	std::array<VkDescriptorSetLayoutBinding, 1> set_layout_bindings =
	    {
	        // Binding 0 : Fragment shader combined sampler.
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	            VK_SHADER_STAGE_FRAGMENT_BIT,
	            0),
	    };

	descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(
	        set_layout_bindings.data(),
	        vkb::to_u32(set_layout_bindings.size()));

	pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &present.pipeline.set_layout, 1);

	set_layout_bindings[0].pImmutableSamplers = (is_fdm_enabled() && !is_show_fdm_enabled()) ? &samplers.subsampled_nearest : &samplers.nearest;

	vkDestroyDescriptorSetLayout(device_handle, present.pipeline.set_layout, nullptr);
	VK_CHECK(vkCreateDescriptorSetLayout(device_handle, &descriptor_layout_create_info, nullptr, &present.pipeline.set_layout));
	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, get_object_handle(present.pipeline.set_layout), "Present Descriptor Set Layout");

	vkDestroyPipelineLayout(device_handle, present.pipeline.pipeline_layout, nullptr);
	VK_CHECK(vkCreatePipelineLayout(device_handle, &pipeline_layout_create_info, nullptr, &present.pipeline.pipeline_layout));
	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_PIPELINE_LAYOUT, get_object_handle(present.pipeline.pipeline_layout), "Present Pipeline Layout");
}

void FragmentDensityMap::setup_descriptor_set_main_pass()
{
	VkDevice device_handle = get_device().get_handle();
	assert(main_pass.meshes.descriptor_sets.empty());
	main_pass.meshes.descriptor_sets.resize(scene_data.size(), VK_NULL_HANDLE);
	for (uint32_t i = 0; i < scene_data.size(); ++i)
	{
		auto &mesh_data       = scene_data[i];
		auto &mesh_descriptor = main_pass.meshes.descriptor_sets[i];

		VkDescriptorSetAllocateInfo descriptor_set_alloc_info = vkb::initializers::descriptor_set_allocate_info(main_pass.descriptor_pool, &main_pass.meshes.pipeline.set_layout, 1);
		VK_CHECK(vkAllocateDescriptorSets(device_handle, &descriptor_set_alloc_info, &mesh_descriptor));

		std::string debug_name = fmt::format("Descriptor Set glTF submesh-{} <{}>", i, mesh_data.submesh->get_name());
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_DESCRIPTOR_SET, get_object_handle(mesh_descriptor), debug_name.c_str());

		VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*mesh_data.vertex_ubo);
		VkDescriptorImageInfo  image_descriptor  = vkb::initializers::descriptor_image_info(
            mesh_data.base_color_texture->get_sampler()->get_core_sampler().get_handle(),
            mesh_data.base_color_texture->get_image()->get_vk_image_view().get_handle(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		std::array<VkWriteDescriptorSet, 2> write_descriptor_sets =
		    {
		        vkb::initializers::write_descriptor_set(mesh_descriptor, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &buffer_descriptor),              // Binding 0 : Vertex shader uniform buffer.
		        vkb::initializers::write_descriptor_set(mesh_descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &image_descriptor)        // Binding 1 : Color map.
		    };
		vkUpdateDescriptorSets(device_handle, vkb::to_u32(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
	}
}

void FragmentDensityMap::setup_descriptor_set_fdm()
{
	VkDescriptorSetAllocateInfo descriptor_set_alloc_info;
	VkDevice                    device_handle = get_device().get_handle();
	descriptor_set_alloc_info                 = vkb::initializers::descriptor_set_allocate_info(
        descriptor_pool, &fdm.generate.pipeline.set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(device_handle, &descriptor_set_alloc_info, &fdm.generate.set));

	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_DESCRIPTOR_SET,
	                           get_object_handle(fdm.generate.set),
	                           is_generate_fdm_compute() ? "Descriptor set Generate FDM (Compute)" :
	                                                       "Descriptor set Generate FDM (Fragment)");

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*fdm.ubo);
	VkDescriptorImageInfo  image_descriptor  = vkb::initializers::descriptor_image_info(
        samplers.nearest,
        fdm.image.view,
        VK_IMAGE_LAYOUT_GENERAL);
	std::array<VkWriteDescriptorSet, 2> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(fdm.generate.set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &buffer_descriptor),
	    vkb::initializers::write_descriptor_set(fdm.generate.set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &image_descriptor)};
	vkUpdateDescriptorSets(device_handle,
	                       is_generate_fdm_compute() ? vkb::to_u32(write_descriptor_sets.size()) : 1u,
	                       write_descriptor_sets.data(), 0, NULL);
}

void FragmentDensityMap::setup_descriptor_set_present()
{
	VkDescriptorSetAllocateInfo descriptor_set_alloc_info;
	VkDevice                    device_handle = get_device().get_handle();
	descriptor_set_alloc_info                 = vkb::initializers::descriptor_set_allocate_info(
        descriptor_pool, &present.pipeline.set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(device_handle, &descriptor_set_alloc_info, &present.set));
	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_DESCRIPTOR_SET, get_object_handle(present.set), "Descriptor set Present");

	VkDescriptorImageInfo image_descriptor = vkb::initializers::descriptor_image_info(
	    (is_fdm_enabled() && !is_show_fdm_enabled()) ? samplers.subsampled_nearest : samplers.nearest,
	    is_show_fdm_enabled() ? fdm.image.view : main_pass.image.view,
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	std::array<VkWriteDescriptorSet, 1> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(
	        present.set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &image_descriptor)};
	vkUpdateDescriptorSets(device_handle, vkb::to_u32(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void FragmentDensityMap::prepare_uniform_buffers_main_pass()
{
	// Create uniform buffers for each glTF-submesh.
	for (auto &mesh_data : scene_data)
	{
		mesh_data.vertex_ubo = std::make_unique<vkb::core::BufferC>(
		    get_device(), sizeof(UBOVS), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}
}

void FragmentDensityMap::prepare_uniform_buffers_fdm()
{
	fdm.ubo = std::make_unique<vkb::core::BufferC>(
	    get_device(), sizeof(FDMUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	fdm.ubo_data = {};        // Reset so that the GPU UBO is updated.
	update_uniform_buffer(0.0f);
}

void FragmentDensityMap::update_uniform_buffer(float delta_time)
{
	// Main pass glTF-submeshes UBO.
	{
		UBOVS ubo_vs{
		    .projection = camera.matrices.perspective};

		for (auto &mesh_data : scene_data)
		{
			ubo_vs.modelview = camera.matrices.view * mesh_data.world_matrix;
			mesh_data.vertex_ubo->convert_and_update(ubo_vs);
		}
	}

	// Generate FDM UBO.
	{
		const float     min_dimension     = static_cast<float>(std::min(fdm.extend.width, fdm.extend.height));
		constexpr float radius_factor_1x1 = 0.20f;
		constexpr float radius_factor_1x2 = 0.25f;
		constexpr float radius_factor_2x2 = 0.30f;
		constexpr float radius_factor_2x4 = 0.35f;

		constexpr uint32_t frame_period = 512;

		if (is_update_fdm_enabled())
		{
			frame_idx = (frame_idx + 1) % frame_period;
		}

		// Small animation rotating the eye center around a circle.
		constexpr float frame_factor           = 2.0f * glm::pi<float>() / static_cast<float>(frame_period);
		const float     frame_angle            = static_cast<float>(frame_idx) * frame_factor;
		const float     rotating_center_radius = 0.12f * min_dimension;

		FDMUBO new_fdm_data{
		    .eye_center = {
		        static_cast<float>(fdm.extend.width) * 0.5f + rotating_center_radius * sin(frame_angle),
		        static_cast<float>(fdm.extend.height) * 0.5f + rotating_center_radius * cos(frame_angle),
		        0.0f, 0.0f},
		    .circle_radius = {min_dimension * radius_factor_1x1, min_dimension * radius_factor_1x2, min_dimension * radius_factor_2x2, min_dimension * radius_factor_2x4},
		};

		if (fdm.ubo_data != new_fdm_data)
		{
			fdm.ubo_data = new_fdm_data;
			fdm.ubo->convert_and_update(new_fdm_data);
		}
	}
}

void FragmentDensityMap::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	VulkanSample::request_gpu_features(gpu);
	auto &requested_features = gpu.get_mutable_requested_features();

	// Enable anisotropic filtering if supported.
	if (gpu.get_features().samplerAnisotropy)
	{
		requested_features.samplerAnisotropy = VK_TRUE;
	}

	// Enable texture compression.
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

	// Check for FDM support and configure options.
	available_options.supports_fdm = false;
	if (gpu.is_extension_supported(VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME))
	{
		const auto &supported_extension_features =
		    gpu.get_extension_features<VkPhysicalDeviceFragmentDensityMapFeaturesEXT>();

		if (!supported_extension_features.fragmentDensityMap)
		{
			LOGW("FDM extension supported but fragmentDensityMap feature is not supported.");
		}
		else
		{
			available_options.supports_fdm = true;

			available_options.supports_dynamic_fdm &= static_cast<bool>(supported_extension_features.fragmentDensityMapDynamic);
			if (!available_options.supports_dynamic_fdm)
			{
				LOGW("Dynamic FDM is not supported. The FDM cannot be updated.");
				current_options.update_fdm = false;
			}

			auto &requested_extension_features                     = gpu.add_extension_features<VkPhysicalDeviceFragmentDensityMapFeaturesEXT>();
			requested_extension_features.fragmentDensityMap        = VK_TRUE;
			requested_extension_features.fragmentDensityMapDynamic = available_options.supports_dynamic_fdm;
			// fragmentDensityMapNonSubsampledImages is not supported on all GPUs.
			// It is not necessary in this sample since we create resources with the flag VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT.
			// If supported, we could skip the present pass and render directly to the swapchain.
			// However, this is not recommended, since UI and composition are usually done at full resolution.
			requested_extension_features.fragmentDensityMapNonSubsampledImages = VK_FALSE;
		}
	}
	if (!available_options.supports_fdm)
	{
		current_options.enable_fdm = false;
		LOGE("Fragment density map is not supported");
	}
	else
	{
		VkPhysicalDeviceProperties2KHR                  device_properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
		VkPhysicalDeviceFragmentDensityMapPropertiesEXT physical_device_FRAGMENT_DENSITY_MAP_properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT};
		device_properties.pNext = &physical_device_FRAGMENT_DENSITY_MAP_properties;

		vkGetPhysicalDeviceProperties2KHR(gpu.get_handle(), &device_properties);

		LOGI("FDM enable: FDM min texel size={}x{} FDM max texel size={}x{}",
		     physical_device_FRAGMENT_DENSITY_MAP_properties.minFragmentDensityTexelSize.width,
		     physical_device_FRAGMENT_DENSITY_MAP_properties.minFragmentDensityTexelSize.height,
		     physical_device_FRAGMENT_DENSITY_MAP_properties.maxFragmentDensityTexelSize.width,
		     physical_device_FRAGMENT_DENSITY_MAP_properties.maxFragmentDensityTexelSize.height);

		fdm.texel_size.width  = std::clamp(fdm.texel_size.width,
		                                   physical_device_FRAGMENT_DENSITY_MAP_properties.minFragmentDensityTexelSize.width,
		                                   physical_device_FRAGMENT_DENSITY_MAP_properties.maxFragmentDensityTexelSize.width);
		fdm.texel_size.height = std::clamp(fdm.texel_size.height,
		                                   physical_device_FRAGMENT_DENSITY_MAP_properties.minFragmentDensityTexelSize.height,
		                                   physical_device_FRAGMENT_DENSITY_MAP_properties.maxFragmentDensityTexelSize.height);
	}
	last_options = current_options;
}

void FragmentDensityMap::write_density_map(VkCommandBuffer cmd_buffer)
{
	if (is_generate_fdm_compute())
	{
		debug_utils.cmd_begin_label(cmd_buffer, "Write FDM (compute)", glm::vec4());
		VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

		// Clear the density map buffer by transitioning it from UNDEFINED.
		vkb::image_layout_transition(cmd_buffer,
		                             fdm.image.image,
		                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		                             {},
		                             VK_ACCESS_SHADER_WRITE_BIT,
		                             VK_IMAGE_LAYOUT_UNDEFINED,
		                             VK_IMAGE_LAYOUT_GENERAL,
		                             subresource_range);

		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fdm.generate.pipeline.pipeline);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fdm.generate.pipeline.pipeline_layout, 0, 1, &fdm.generate.set, 0, nullptr);

		const glm::vec2 shader_local_size = glm::vec2(4, 8);        // Keep up to date with shader source code.
		glm::vec2       dispatch_size     = glm::ceil(glm::vec2(fdm.extend.width, fdm.extend.height) / shader_local_size);

		vkCmdDispatch(cmd_buffer, dispatch_size.x, dispatch_size.y, 1);

		if (is_fdm_enabled())
		{
			vkb::image_layout_transition(cmd_buffer,
			                             fdm.image.image,
			                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			                             VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT,
			                             VK_ACCESS_SHADER_WRITE_BIT,
			                             VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT,
			                             VK_IMAGE_LAYOUT_GENERAL,
			                             VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT,
			                             subresource_range);
		}
		debug_utils.cmd_end_label(cmd_buffer);
	}
	else
	{
		debug_utils.cmd_begin_label(cmd_buffer, "Write FDM (fragment)", glm::vec4());

		VkClearValue clear_value{
		    .color = {{0.0f, 0.0f, 0.0f, 0.0f}}};
		VkRect2D   scissor  = vkb::initializers::rect2D(fdm.extend.width, fdm.extend.height, 0, 0);
		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(scissor.extent.width), static_cast<float>(scissor.extent.height), 0.0f, 1.0f);

		VkRenderPassBeginInfo render_pass_begin_info = vkb::initializers::render_pass_begin_info();
		render_pass_begin_info.renderPass            = fdm.generate.render_pass;
		render_pass_begin_info.renderArea.extent     = scissor.extent;
		render_pass_begin_info.clearValueCount       = 1;
		render_pass_begin_info.pClearValues          = &clear_value;

		render_pass_begin_info.framebuffer = fdm.generate.framebuffer;

		vkCmdBeginRenderPass(cmd_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fdm.generate.pipeline.pipeline);

		vkCmdBindDescriptorSets(cmd_buffer,
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        fdm.generate.pipeline.pipeline_layout,
		                        0, 1,
		                        &fdm.generate.set,
		                        0, nullptr);

		vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
		vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);
		vkCmdDraw(cmd_buffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(cmd_buffer);
		debug_utils.cmd_end_label(cmd_buffer);
	}
}

void FragmentDensityMap::setup_render_pass()
{
	setup_color();
	setup_fragment_density_map();

	VkImageLayout density_map_initial_layout = VK_IMAGE_LAYOUT_GENERAL;
	if (is_fdm_enabled())
	{
		density_map_initial_layout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
	}
	VkDevice device_handle = get_device().get_handle();
	// Main render pass (forward render).
	{
		std::array<VkAttachmentDescription2, 3> attachments{
		    // Color attachment.
		    VkAttachmentDescription2{
		        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
		        .pNext          = nullptr,
		        .flags          = 0,
		        .format         = get_render_context().get_format(),
		        .samples        = VK_SAMPLE_COUNT_1_BIT,
		        .loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
		        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		        .finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
		    // Depth attachment.
		    VkAttachmentDescription2{
		        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
		        .pNext          = nullptr,
		        .flags          = 0,
		        .format         = depth_format,
		        .samples        = VK_SAMPLE_COUNT_1_BIT,
		        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
		        .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
		        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		        .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL},

		    // Density map attachment.
		    VkAttachmentDescription2{
		        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
		        .pNext          = nullptr,
		        .flags          = 0,
		        .format         = VK_FORMAT_R8G8_UNORM,
		        .samples        = VK_SAMPLE_COUNT_1_BIT,
		        .loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD,
		        .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		        .initialLayout  = density_map_initial_layout,
		        .finalLayout    = density_map_initial_layout}};

		VkAttachmentReference2 color_attachment_ref{
		    .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
		    .pNext      = nullptr,
		    .attachment = 0,
		    .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT};

		VkAttachmentReference2 depth_reference = {
		    .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
		    .pNext      = nullptr,
		    .attachment = 1,
		    .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT};

		VkSubpassDescription2 subpass{
		    .sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
		    .pNext                   = nullptr,
		    .flags                   = 0,
		    .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
		    .viewMask                = 0u,
		    .inputAttachmentCount    = 0,
		    .pInputAttachments       = nullptr,
		    .colorAttachmentCount    = 1,
		    .pColorAttachments       = &color_attachment_ref,
		    .pResolveAttachments     = nullptr,
		    .pDepthStencilAttachment = &depth_reference,
		    .preserveAttachmentCount = 0,
		    .pPreserveAttachments    = nullptr};

		VkSubpassDependency2 dependency{
		    .sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
		    .pNext           = nullptr,
		    .srcSubpass      = 0,
		    .dstSubpass      = VK_SUBPASS_EXTERNAL,
		    .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		    .dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		    .srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		    .dstAccessMask   = VK_ACCESS_SHADER_READ_BIT,
		    .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		    .viewOffset      = 0};

		VkRenderPassFragmentDensityMapCreateInfoEXT density_map_info{
		    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT,
		    .pNext = nullptr,
		    .fragmentDensityMapAttachment{
		        .attachment = 2,
		        .layout     = density_map_initial_layout},
		};

		VkRenderPassCreateInfo2 render_pass_info{
		    .sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
		    .pNext                   = is_fdm_enabled() ? &density_map_info : nullptr,
		    .flags                   = 0,
		    .attachmentCount         = is_fdm_enabled() ? vkb::to_u32(attachments.size()) : (vkb::to_u32(attachments.size()) - 1),
		    .pAttachments            = attachments.data(),
		    .subpassCount            = 1,
		    .pSubpasses              = &subpass,
		    .dependencyCount         = 1,
		    .pDependencies           = &dependency,
		    .correlatedViewMaskCount = 0,
		    .pCorrelatedViewMasks    = nullptr,
		};

		vkDestroyRenderPass(device_handle, render_pass, nullptr);
		VK_CHECK(vkCreateRenderPass2KHR(device_handle, &render_pass_info, nullptr, &render_pass));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_RENDER_PASS, get_object_handle(render_pass), "Main Renderpass (Forward rendering)");
	}

	// Write FDM (fragment).
	if (is_fdm_enabled() && !is_generate_fdm_compute())
	{
		std::array<VkAttachmentDescription2, 1> attachments{
		    VkAttachmentDescription2{
		        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
		        .pNext          = nullptr,
		        .flags          = 0,
		        .format         = VK_FORMAT_R8G8_UNORM,
		        .samples        = VK_SAMPLE_COUNT_1_BIT,
		        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
		        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
		        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		        .finalLayout    = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT}};

		VkAttachmentReference2 attachment_ref{
		    .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
		    .pNext      = nullptr,
		    .attachment = 0,
		    .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT};

		VkSubpassDescription2 subpass{
		    .sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
		    .pNext                   = nullptr,
		    .flags                   = 0,
		    .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
		    .viewMask                = 0,
		    .inputAttachmentCount    = 0,
		    .pInputAttachments       = nullptr,
		    .colorAttachmentCount    = 1,
		    .pColorAttachments       = &attachment_ref,
		    .pResolveAttachments     = nullptr,
		    .pDepthStencilAttachment = nullptr,
		    .preserveAttachmentCount = 0,
		    .pPreserveAttachments    = nullptr};

		VkSubpassDependency2 dependency{
		    .sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
		    .pNext           = nullptr,
		    .srcSubpass      = VK_SUBPASS_EXTERNAL,
		    .dstSubpass      = 0,
		    .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		    .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		    .srcAccessMask   = 0,
		    .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		    .dependencyFlags = 0,
		    .viewOffset      = 0};

		VkRenderPassCreateInfo2 render_pass_info{
		    .sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
		    .pNext                   = nullptr,
		    .flags                   = 0,
		    .attachmentCount         = attachments.size(),
		    .pAttachments            = attachments.data(),
		    .subpassCount            = 1,
		    .pSubpasses              = &subpass,
		    .dependencyCount         = 1,
		    .pDependencies           = &dependency,
		    .correlatedViewMaskCount = 0,
		    .pCorrelatedViewMasks    = nullptr};

		vkDestroyRenderPass(device_handle, fdm.generate.render_pass, nullptr);
		VK_CHECK(vkCreateRenderPass2KHR(device_handle, &render_pass_info, nullptr, &fdm.generate.render_pass));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_RENDER_PASS, get_object_handle(fdm.generate.render_pass), "Write FDM Renderpass");
	}
	// Present
	{
		std::array<VkAttachmentDescription2, 1> attachments{
		    VkAttachmentDescription2{
		        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
		        .pNext          = nullptr,
		        .flags          = 0,
		        .format         = get_render_context().get_format(),
		        .samples        = VK_SAMPLE_COUNT_1_BIT,
		        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
		        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
		        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR}};

		VkAttachmentReference2 attachment_ref{
		    .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
		    .pNext      = nullptr,
		    .attachment = 0,
		    .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT};

		VkSubpassDescription2 subpass{
		    .sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
		    .pNext                   = nullptr,
		    .flags                   = 0,
		    .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
		    .viewMask                = 0,
		    .inputAttachmentCount    = 0,
		    .pInputAttachments       = nullptr,
		    .colorAttachmentCount    = 1,
		    .pColorAttachments       = &attachment_ref,
		    .pResolveAttachments     = nullptr,
		    .pDepthStencilAttachment = nullptr,
		    .preserveAttachmentCount = 0,
		    .pPreserveAttachments    = nullptr};

		VkSubpassDependency2 dependency{
		    .sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
		    .pNext           = nullptr,
		    .srcSubpass      = VK_SUBPASS_EXTERNAL,
		    .dstSubpass      = 0,
		    .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		    .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		    .srcAccessMask   = 0,
		    .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		    .dependencyFlags = 0,
		    .viewOffset      = 0};

		VkRenderPassCreateInfo2 render_pass_info{
		    .sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
		    .pNext                   = nullptr,
		    .flags                   = 0,
		    .attachmentCount         = attachments.size(),
		    .pAttachments            = attachments.data(),
		    .subpassCount            = 1,
		    .pSubpasses              = &subpass,
		    .dependencyCount         = 1,
		    .pDependencies           = &dependency,
		    .correlatedViewMaskCount = 0,
		    .pCorrelatedViewMasks    = nullptr};

		vkDestroyRenderPass(device_handle, present.render_pass, nullptr);
		VK_CHECK(vkCreateRenderPass2KHR(device_handle, &render_pass_info, nullptr, &present.render_pass));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_RENDER_PASS, get_object_handle(present.render_pass), "Present Renderpass");
	}
}
void FragmentDensityMap::setup_framebuffer()
{
	VkDevice device_handle = get_device().get_handle();

	// Main pass framebuffer.
	{
		std::array<VkImageView, 3> attachments{
		    main_pass.image.view,
		    depth_stencil.view,
		    fdm.image.view};
		VkFramebufferCreateInfo framebuffer_create_info{
		    .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		    .pNext           = nullptr,
		    .renderPass      = render_pass,
		    .attachmentCount = is_fdm_enabled() ? vkb::to_u32(attachments.size()) : (vkb::to_u32(attachments.size()) - 1u),
		    .pAttachments    = attachments.data(),
		    .width           = main_pass.extend.width,
		    .height          = main_pass.extend.height,
		    .layers          = 1};

		vkDestroyFramebuffer(device_handle, main_pass.framebuffer, nullptr);
		VK_CHECK(vkCreateFramebuffer(device_handle, &framebuffer_create_info, nullptr, &main_pass.framebuffer));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_FRAMEBUFFER, get_object_handle(main_pass.framebuffer), "Main pass Framebuffer");
	}

	// Present framebuffer.
	{
		// Delete existing framebuffers.
		if (!framebuffers.empty())
		{
			for (auto &framebuffer : framebuffers)
			{
				vkDestroyFramebuffer(device_handle, framebuffer, nullptr);
			}
		}

		VkFramebufferCreateInfo framebuffer_create_info{
		    .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		    .pNext           = nullptr,
		    .renderPass      = present.render_pass,
		    .attachmentCount = 1,
		    .width           = get_render_context().get_surface_extent().width,
		    .height          = get_render_context().get_surface_extent().height,
		    .layers          = 1};

		// Create framebuffers for every swap chain image.
		framebuffers.resize(get_render_context().get_render_frames().size());
		for (uint32_t i = 0; i < framebuffers.size(); i++)
		{
			framebuffer_create_info.pAttachments = &swapchain_buffers[i].view;
			VK_CHECK(vkCreateFramebuffer(device_handle, &framebuffer_create_info, nullptr, &framebuffers[i]));
			std::string object_debug_name{fmt::format("Swapchain Framebuffer {}", i)};
			debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_FRAMEBUFFER, get_object_handle(framebuffers[i]), object_debug_name.c_str());
			object_debug_name = fmt::format("Swapchain Image {}", i);
			debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_IMAGE, get_object_handle(swapchain_buffers[i].image), object_debug_name.c_str());
			object_debug_name = fmt::format("Swapchain Image View {}", i);
			debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_IMAGE_VIEW, get_object_handle(swapchain_buffers[i].view), object_debug_name.c_str());
		}
	}

	// Write FDM (fragment) framebuffer.
	{
		vkDestroyFramebuffer(device_handle, fdm.generate.framebuffer, nullptr);
		if (is_fdm_enabled() && !is_generate_fdm_compute())
		{
			std::array<VkImageView, 1> attachments{fdm.image.view};
			VkFramebufferCreateInfo    framebuffer_create_info{
			       .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			       .pNext           = nullptr,
			       .renderPass      = fdm.generate.render_pass,
			       .attachmentCount = vkb::to_u32(attachments.size()),
			       .pAttachments    = attachments.data(),
			       .width           = fdm.extend.width,
			       .height          = fdm.extend.height,
			       .layers          = 1};

			VK_CHECK(vkCreateFramebuffer(device_handle, &framebuffer_create_info, nullptr, &fdm.generate.framebuffer));
			debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_FRAMEBUFFER, get_object_handle(fdm.generate.framebuffer), "Write FDM Framebuffer");
		}
		else
		{
			fdm.generate.framebuffer = VK_NULL_HANDLE;
		}
	}
}

void FragmentDensityMap::update_extents()
{
	// Rendering at 4× the resolution to make performance improvements more noticeable.
	glm::vec2 rendering_factor{4.0f, 4.0f};
	fdm.extend = {
	    std::max(1u,
	             static_cast<uint32_t>(std::ceil(static_cast<float>(rendering_factor.x * get_render_context().get_surface_extent().width) / static_cast<float>(fdm.texel_size.width)))),
	    std::max(1u,
	             static_cast<uint32_t>(std::ceil(static_cast<float>(rendering_factor.y * get_render_context().get_surface_extent().height) / static_cast<float>(fdm.texel_size.height)))),
	    1};

	main_pass.extend = {
	    fdm.extend.width * fdm.texel_size.width,
	    fdm.extend.height * fdm.texel_size.height};

	camera.update_aspect_ratio(static_cast<float>(main_pass.extend.width) / static_cast<float>(main_pass.extend.height));
}

void FragmentDensityMap::setup_depth_stencil()
{
	destroy_image(depth_stencil);

	update_extents();
	assert(main_pass.extend.width == (fdm.extend.width * fdm.texel_size.width));
	assert(main_pass.extend.height == (fdm.extend.height * fdm.texel_size.height));
	assert((main_pass.extend.height > 0) && (main_pass.extend.width > 0));

	// Create depth stencil image.
	{
		// This sample needs to add the subsampled flag so we cannot use the framework function.
		VkImageCreateInfo image_create_info{
		    .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		    .flags     = is_fdm_enabled() ? VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT : 0u,
		    .imageType = VK_IMAGE_TYPE_2D,
		    .format    = depth_format,
		    .extent{main_pass.extend.width, main_pass.extend.height, 1},
		    .mipLevels   = 1,
		    .arrayLayers = 1,
		    .samples     = VK_SAMPLE_COUNT_1_BIT,
		    .tiling      = VK_IMAGE_TILING_OPTIMAL,
		    .usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT};

		VkDevice device_handle = get_device().get_handle();
		VK_CHECK(vkCreateImage(device_handle, &image_create_info, nullptr, &depth_stencil.image));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_IMAGE, get_object_handle(depth_stencil.image), "Main pass Depth Image");

		VkMemoryRequirements memReqs{};
		vkGetImageMemoryRequirements(device_handle, depth_stencil.image, &memReqs);

		VkMemoryAllocateInfo memory_allocation{
		    .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		    .allocationSize  = memReqs.size,
		    .memoryTypeIndex = get_device().get_gpu().get_memory_type(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
		VK_CHECK(vkAllocateMemory(device_handle, &memory_allocation, nullptr, &depth_stencil.mem));
		VK_CHECK(vkBindImageMemory(device_handle, depth_stencil.image, depth_stencil.mem, 0));

		VkImageViewCreateInfo image_view_create_info{
		    .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		    .image    = depth_stencil.image,
		    .viewType = VK_IMAGE_VIEW_TYPE_2D,
		    .format   = depth_format,
		    .subresourceRange{
		        .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
		        .baseMipLevel   = 0,
		        .levelCount     = 1,
		        .baseArrayLayer = 0,
		        .layerCount     = 1}};
		// Stencil aspect should only be set on depth + stencil formats.
		if (depth_format >= VK_FORMAT_D16_UNORM_S8_UINT)
		{
			image_view_create_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		VK_CHECK(vkCreateImageView(device_handle, &image_view_create_info, nullptr, &depth_stencil.view));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_IMAGE_VIEW, get_object_handle(depth_stencil.view), "Main pass depth image view");
	}
}

void FragmentDensityMap::setup_color()
{
	assert(main_pass.extend.width == (fdm.extend.width * fdm.texel_size.width));
	assert(main_pass.extend.height == (fdm.extend.height * fdm.texel_size.height));
	assert((main_pass.extend.height > 0) && (main_pass.extend.width > 0));

	destroy_image(main_pass.image);

	// Create images used to render the framebuffer.
	// Note: We need to add the subsampled flag.
	VkImageCreateInfo image_create_info{
	    .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	    .flags     = is_fdm_enabled() ? VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT : 0u,
	    .imageType = VK_IMAGE_TYPE_2D,
	    .format    = get_render_context().get_format(),
	    .extent{main_pass.extend.width, main_pass.extend.height, 1},
	    .mipLevels   = 1,
	    .arrayLayers = 1,
	    .samples     = VK_SAMPLE_COUNT_1_BIT,
	    .tiling      = VK_IMAGE_TILING_OPTIMAL,
	    .usage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT};

	VkDevice device_handle = get_device().get_handle();
	VK_CHECK(vkCreateImage(device_handle, &image_create_info, nullptr, &main_pass.image.image));

	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_IMAGE, get_object_handle(main_pass.image.image), "Main pass color image");

	VkMemoryRequirements mem_reqs{};
	vkGetImageMemoryRequirements(device_handle, main_pass.image.image, &mem_reqs);

	VkMemoryAllocateInfo mem_alloc{
	    .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	    .allocationSize  = mem_reqs.size,
	    .memoryTypeIndex = get_device().get_gpu().get_memory_type(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
	VK_CHECK(vkAllocateMemory(device_handle, &mem_alloc, nullptr, &main_pass.image.mem));
	VK_CHECK(vkBindImageMemory(device_handle, main_pass.image.image, main_pass.image.mem, 0));

	VkImageViewCreateInfo image_view_create_info{
	    .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	    .pNext            = nullptr,
	    .image            = main_pass.image.image,
	    .viewType         = VK_IMAGE_VIEW_TYPE_2D,
	    .format           = get_render_context().get_format(),
	    .components       = vkb::initializers::component_mapping(),
	    .subresourceRange = {
	        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
	        .baseMipLevel   = 0,
	        .levelCount     = 1,
	        .baseArrayLayer = 0,
	        .layerCount     = 1,
	    }};
	VK_CHECK(vkCreateImageView(device_handle, &image_view_create_info, nullptr, &main_pass.image.view));
	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_IMAGE_VIEW, get_object_handle(main_pass.image.view), "Main pass color image view");
}

bool FragmentDensityMap::is_fdm_supported()
{
	return available_options.supports_fdm;
}

bool FragmentDensityMap::is_generate_fdm_compute()
{
	return is_fdm_enabled() && last_options.generate_fdm_compute;
}

bool FragmentDensityMap::is_show_fdm_enabled()
{
	return is_fdm_enabled() && last_options.show_fdm;
}

bool FragmentDensityMap::is_show_stats()
{
	return has_gui() && last_options.show_stats;
}

bool FragmentDensityMap::is_debug_fdm_enabled()
{
	return is_fdm_enabled() && last_options.debug_fdm;
}

bool FragmentDensityMap::is_update_fdm_enabled()
{
	return is_fdm_enabled() && last_options.update_fdm && available_options.supports_dynamic_fdm;
}

bool FragmentDensityMap::is_fdm_enabled()
{
	return last_options.enable_fdm && is_fdm_supported();
}

void FragmentDensityMap::setup_fragment_density_map()
{
	assert(main_pass.extend.width == (fdm.extend.width * fdm.texel_size.width));
	assert(main_pass.extend.height == (fdm.extend.height * fdm.texel_size.height));

	destroy_image(fdm.image);
	if (!is_fdm_enabled())
	{
		return;
	}

	VkImageCreateInfo image_create_info{
	    .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	    .flags       = 0u,
	    .imageType   = VK_IMAGE_TYPE_2D,
	    .format      = VK_FORMAT_R8G8_UNORM,
	    .extent      = fdm.extend,
	    .mipLevels   = 1,
	    .arrayLayers = 1,
	    .samples     = VK_SAMPLE_COUNT_1_BIT,
	    .tiling      = VK_IMAGE_TILING_OPTIMAL,
	    .usage       = 0u};

	if (is_generate_fdm_compute())
	{
		image_create_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}
	else
	{
		image_create_info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	if (is_fdm_enabled())
	{
		image_create_info.usage |= VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
	}
	if (is_show_fdm_enabled())
	{
		image_create_info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}

	VkDevice device_handle = get_device().get_handle();
	VK_CHECK(vkCreateImage(device_handle, &image_create_info, nullptr, &fdm.image.image));

	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_IMAGE, get_object_handle(fdm.image.image), "FDM Image");

	VkMemoryRequirements mem_reqs{};
	vkGetImageMemoryRequirements(device_handle, fdm.image.image, &mem_reqs);

	VkMemoryAllocateInfo mem_alloc{
	    .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	    .allocationSize  = mem_reqs.size,
	    .memoryTypeIndex = get_device().get_gpu().get_memory_type(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
	VK_CHECK(vkAllocateMemory(device_handle, &mem_alloc, nullptr, &fdm.image.mem));
	VK_CHECK(vkBindImageMemory(device_handle, fdm.image.image, fdm.image.mem, 0));

	VkImageViewCreateInfo image_view_create_info{
	    .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	    .pNext            = nullptr,
	    .flags            = is_update_fdm_enabled() ? VK_IMAGE_VIEW_CREATE_FRAGMENT_DENSITY_MAP_DYNAMIC_BIT_EXT : 0u,
	    .image            = fdm.image.image,
	    .viewType         = VK_IMAGE_VIEW_TYPE_2D,
	    .format           = VK_FORMAT_R8G8_UNORM,
	    .components       = vkb::initializers::component_mapping(),
	    .subresourceRange = {
	        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
	        .baseMipLevel   = 0,
	        .levelCount     = 1,
	        .baseArrayLayer = 0,
	        .layerCount     = 1,
	    }};
	VK_CHECK(vkCreateImageView(device_handle, &image_view_create_info, nullptr, &fdm.image.view));

	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_IMAGE_VIEW, get_object_handle(fdm.image.view), "FDM Image View");
}

void FragmentDensityMap::prepare_gui()
{
	vkb::CounterSamplingConfig config{
	    .mode  = vkb::CounterSamplingMode::Continuous,
	    .speed = 0.1f};
	get_stats().request_stats({
	                              vkb::StatIndex::frame_times,
	                              vkb::StatIndex::gpu_cycles,
	                          },
	                          config);

	create_gui(*window, &get_stats(), 15.0f, true);
	get_gui().prepare(pipeline_cache, present.render_pass,
	                  {load_shader("uioverlay/uioverlay.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
	                   load_shader("uioverlay/uioverlay.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)});
}

void FragmentDensityMap::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (is_fdm_supported())
	{
		drawer.checkbox("Enable FDM", reinterpret_cast<bool *>(&current_options.enable_fdm));
		if (is_fdm_enabled())
		{
			if (available_options.supports_dynamic_fdm)
			{
				drawer.checkbox("Update FDM each frame", reinterpret_cast<bool *>(&current_options.update_fdm));
			}
			else
			{
				drawer.text("Dynamic FDM is not supported");
			}
			drawer.checkbox("Generate FDM with compute", reinterpret_cast<bool *>(&current_options.generate_fdm_compute));
			drawer.checkbox("Show FDM", reinterpret_cast<bool *>(&current_options.show_fdm));
			drawer.checkbox("Debug FDM", reinterpret_cast<bool *>(&current_options.debug_fdm));
		}
	}
	else
	{
		drawer.text("FDM is not supported");
	}
	if (has_gui())
	{
		drawer.checkbox("Show stats", &current_options.show_stats);
		if (is_show_stats())
		{
			get_gui().show_stats(get_stats());
		}
	}
}

std::unique_ptr<vkb::VulkanSampleC> create_fragment_density_map()
{
	return std::make_unique<FragmentDensityMap>();
}
