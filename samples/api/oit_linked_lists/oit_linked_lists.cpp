/* Copyright (c) 2023, Maximilien Dagois
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

OITLinkedLists::OITLinkedLists()
{
}

OITLinkedLists::~OITLinkedLists()
{
	if (!device)
	{
		return;
	}

    vkDestroyPipeline(get_device().get_handle(), combine_pipeline, nullptr);
    vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
	vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
	object_desc.reset();
	scene_constants.reset();
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
	camera.set_rotation({0.0f, 180.0f, 0.0f});
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	load_assets();
	create_resources();
	create_descriptors();
    create_pipelines();
	build_command_buffers();

	update_scene_constants();
	fill_object_data();

	prepared = true;
	return true;
}

void OITLinkedLists::load_assets()
{
	object = load_model("scenes/geosphere.gltf");
}

void OITLinkedLists::create_resources()
{
	scene_constants = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(SceneConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	object_desc = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ObjectDesc) * kObjectCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void OITLinkedLists::create_descriptors()
{
    {
        std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
            vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
            vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
        };
        VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
        VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout));
    }

    {
        std::vector<VkDescriptorPoolSize> pool_sizes = {
            vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
        };
        const uint32_t num_descriptor_sets = 1;
        VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
        VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
    }

    {
        VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);
        VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

        VkDescriptorBufferInfo scene_constants_descriptor = create_descriptor(*scene_constants);
        VkDescriptorBufferInfo object_desc_descriptor = create_descriptor(*object_desc);
        std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
            vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &scene_constants_descriptor),
            vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &object_desc_descriptor),
        };
        vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
    }
}

void OITLinkedLists::create_pipelines()
{
    {
        VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);
        VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
    }

    {
        const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
            vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
        };
        const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
            vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),
        };
        VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
        vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
        vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
        vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
        vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

        VkPipelineRasterizationStateCreateInfo rasterization_state = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);


        VkPipelineColorBlendAttachmentState blend_attachment_state = vkb::initializers::pipeline_color_blend_attachment_state(0xF, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo color_blend_state = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

        VkPipelineMultisampleStateCreateInfo multisample_state = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

        VkPipelineViewportStateCreateInfo viewport_state = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS);

        std::vector<VkDynamicState> dynamic_state_enables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState = vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables.data(), static_cast<uint32_t>(dynamic_state_enables.size()), 0);

        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
        shader_stages[0] = load_shader("oit_linked_lists/gather.vert", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = load_shader("oit_linked_lists/gather.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

        VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout, render_pass, 0);
        pipeline_create_info.pVertexInputState   = &vertex_input_state;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state;
        pipeline_create_info.pRasterizationState = &rasterization_state;
        pipeline_create_info.pColorBlendState    = &color_blend_state;
        pipeline_create_info.pMultisampleState   = &multisample_state;
        pipeline_create_info.pViewportState      = &viewport_state;
        pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
        pipeline_create_info.pDynamicState       = &dynamicState;
        pipeline_create_info.stageCount          = static_cast<uint32_t>(shader_stages.size());
        pipeline_create_info.pStages             = shader_stages.data();
        pipeline_create_info.renderPass          = render_pass;

        VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &combine_pipeline));
    }
}

void OITLinkedLists::request_gpu_features(vkb::PhysicalDevice &gpu)
{
}

void OITLinkedLists::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.3f, 0.3f, 0.3f, 1.0f}};
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
		render_pass_begin_info.framebuffer = framebuffers[i];
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));
        {
            vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
            {
                VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
                vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

                VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
                vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

                vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, combine_pipeline);
                vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
                draw_model(object, draw_cmd_buffers[i], kObjectCount);

                draw_ui(draw_cmd_buffers[i]);
            }
            vkCmdEndRenderPass(draw_cmd_buffers[i]);
        }
		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void OITLinkedLists::update_scene_constants()
{
	SceneConstants constants;
	constants.projection       = camera.matrices.perspective;
	constants.view             = camera.matrices.view;
	scene_constants->convert_and_update(constants);
}

void OITLinkedLists::fill_object_data()
{
	ObjectDesc desc[kObjectCount] = {};

	auto get_random_float = []()
	{
		return static_cast<float>(rand()) / (RAND_MAX);
	};

	for (uint32_t l = 0; l < kObjectLayerCount; ++l)
	{
		for (uint32_t c = 0; c < kObjectColumnCount; ++c)
		{
			for (uint32_t r = 0; r < kObjectRowCount; ++r)
			{
				const uint32_t object_index =
					(l * kObjectColumnCount * kObjectRowCount) +
					(c * kObjectRowCount) + r;

				const float x = static_cast<float>(r) - ((kObjectRowCount - 1) * 0.5f);
				const float y = static_cast<float>(c) - ((kObjectColumnCount - 1) * 0.5f);
				const float z = static_cast<float>(l) - ((kObjectLayerCount - 1) * 0.5f);
				const float scale = 0.02f;
				desc[object_index].model =
					glm::scale(
						glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)),
						glm::vec3(scale));

				desc[object_index].color.r = get_random_float();
				desc[object_index].color.g = get_random_float();
				desc[object_index].color.b = get_random_float();
				desc[object_index].color.a = get_random_float() * 0.5f + 0.10f;
			}
		}
	}

	object_desc->convert_and_update(desc);
}

void OITLinkedLists::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}

	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();

	if (camera.updated)
	{
		update_scene_constants();
	}
}

std::unique_ptr<vkb::VulkanSample> create_oit_linked_lists()
{
	return std::make_unique<OITLinkedLists>();
}
