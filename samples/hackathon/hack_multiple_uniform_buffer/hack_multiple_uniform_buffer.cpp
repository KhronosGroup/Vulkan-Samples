/* Copyright (c) 2024, Arm Limited and Contributors
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

#include "hack_multiple_uniform_buffer.h"

#include "benchmark_mode/benchmark_mode.h"

hack_multiple_uniform_buffer::hack_multiple_uniform_buffer()
{
	title = "Hack: Multiple uniform buffers";
}

hack_multiple_uniform_buffer::~hack_multiple_uniform_buffer()
{
	if (has_device())
	{
		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);

		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
	}
}

void hack_multiple_uniform_buffer::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color = default_clear_color;
	clear_values[1].depthStencil = { 0.0f, 0 };

	VkRenderPassBeginInfo render_pass_begin_info = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass = render_pass;
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.renderArea.extent.width = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount = 2;
	render_pass_begin_info.pClearValues = clear_values;

	for (int32_t i = 0; i < static_cast<int32_t>(draw_cmd_buffers.size()); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer->get(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);

		// Render multiple objects using different model matrices by dynamically offsetting into one uniform buffer
		for (size_t j = 0; j < OBJECT_INSTANCES; j++)
		{
			// Bind the descriptor set for rendering a mesh using the dynamic offset
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set[j], 0, nullptr);

			vkCmdDrawIndexed(draw_cmd_buffers[i], index_count, 1, 0, 0, 0);
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void hack_multiple_uniform_buffer::setup_descriptor_pool()
{
	// Example uses one ubo and one image sampler
	std::vector<VkDescriptorPoolSize> pool_sizes =
	{
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * OBJECT_INSTANCES)};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
		vkb::initializers::descriptor_pool_create_info(
			static_cast<uint32_t>(pool_sizes.size()),
			pool_sizes.data(),
	        1 + OBJECT_INSTANCES);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void hack_multiple_uniform_buffer::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings =
	{
			vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	        vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1)};

	VkDescriptorSetLayoutCreateInfo descriptor_layout =
		vkb::initializers::descriptor_set_layout_create_info(
			set_layout_bindings.data(),
			static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
		vkb::initializers::pipeline_layout_create_info(
			&descriptor_set_layout,
			1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void hack_multiple_uniform_buffer::setup_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);


	VkDescriptorBufferInfo view_buffer_descriptor = create_descriptor(*view_uniform_buffer.view);

	for (size_t i = 0; i < OBJECT_INSTANCES; i++)
	{
		VkDescriptorBufferInfo cube_buffer_descriptor = create_descriptor(*uniform_buffers.single[i], alignment);

		VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set[i]));
		std::vector<VkWriteDescriptorSet> write_descriptor_sets  = {
            // Binding 0 : Projection/View matrix uniform buffer
		    vkb::initializers::write_descriptor_set(descriptor_set[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &view_buffer_descriptor),
            // Binding 1 : Instance matrix as dynamic uniform buffer
		    vkb::initializers::write_descriptor_set(descriptor_set[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &cube_buffer_descriptor),
        };

		vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
	}

}

void hack_multiple_uniform_buffer::prepare_pipelines()
{
	// Load shaders
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

	shader_stages[0] = load_shader("dynamic_uniform_buffers", "base.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("dynamic_uniform_buffers", "base.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkGraphicsPipelineCreateInfo pipeline_create_info =
		vkb::initializers::pipeline_create_info(
			pipeline_layout,
			render_pass,
			0);

	pipeline_create_info.pVertexInputState = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState = &color_blend_state;
	pipeline_create_info.pMultisampleState = &multisample_state;
	pipeline_create_info.pViewportState = &viewport_state;
	pipeline_create_info.pDepthStencilState = &depth_stencil_state;
	pipeline_create_info.pDynamicState = &dynamic_state;
	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

// Prepare and initialize uniform buffer containing shader uniforms
void hack_multiple_uniform_buffer::prepare_uniform_buffer()
{
	prepare_aligned_cubes(sizeof(glm::mat4), nullptr);

	// Vertex shader uniform buffer block
	for (size_t i = 0; i < OBJECT_INSTANCES; i++)
	{
		uniform_buffers.single[i] = std::make_unique<vkb::core::BufferC>(get_device(),
		                                                                 alignment,
		                                                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		                                                                 VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

	update_uniform_buffer(0.0f, true);
}

void hack_multiple_uniform_buffer::update_uniform_buffer(float delta_time, bool force)
{
	for (size_t i = 0; i < OBJECT_INSTANCES; i++)
	{
		uniform_buffers.single[i]->update(get_aligned_cube(i), static_cast<size_t>(uniform_buffers.single[i]->get_size()));
		// Flush to make changes visible to the device
		uniform_buffers.single[i]->flush();
	}
}

void hack_multiple_uniform_buffer::hack_prepare()
{
	prepare_uniform_buffer();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();
}

void hack_multiple_uniform_buffer::hack_render(float delta_time)
{
	if (!paused)
	{
		update_uniform_buffer(delta_time);
	}
}

///
std::unique_ptr<vkb::VulkanSampleC> create_hack_multiple_uniform_buffer()
{
	return std::make_unique<hack_multiple_uniform_buffer>();
}
