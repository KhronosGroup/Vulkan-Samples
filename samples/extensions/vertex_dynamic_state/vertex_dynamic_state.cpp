/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "vertex_dynamic_state.h"

#include "common/vk_common.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "rendering/subpasses/forward_subpass.h"
#include "stats/stats.h"

vertex_dynamic_state::vertex_dynamic_state()
{
	title = "Vertex Dynamic State";

	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);
}

vertex_dynamic_state::~vertex_dynamic_state()
{
	if (device)
	{
		vkDestroySampler(get_device().get_handle(), textures.envmap.sampler, VK_NULL_HANDLE);
		textures = {};
		skybox.reset();
		object.reset();
		ubo.reset();

		vkDestroyPipeline(get_device().get_handle(), model_pipeline, VK_NULL_HANDLE);
		vkDestroyPipeline(get_device().get_handle(), skybox_pipeline, VK_NULL_HANDLE);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, VK_NULL_HANDLE);
		vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, VK_NULL_HANDLE);
	}
}

bool vertex_dynamic_state::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

#if VK_NO_PROTOTYPES
	VkInstance instance = get_device().get_gpu().get_instance().get_handle();
	assert(!!instance);
	vkCmdSetVertexInputEXT = (PFN_vkCmdSetVertexInputEXT) vkGetInstanceProcAddr(instance, "vkCmdSetVertexInputEXT");
	if (!vkCmdSetVertexInputEXT)
	{
		throw std::runtime_error("Unable to dynamically load vkCmdSetVertexInputEXT");
	}
#endif

	vertex_input_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT;
	VkPhysicalDeviceFeatures2 device_features{};
	device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	device_features.pNext = &vertex_input_features;
	vkGetPhysicalDeviceFeatures2(get_device().get_gpu().get_handle(), &device_features);

	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.f, 0.f, -4.0f});
	camera.set_rotation({0.f, 0.f, 0.f});
	camera.set_perspective(60.f, (float) width / (float) height, 256.f, 0.1f);

	load_assets();
	prepare_uniform_buffers();
	model_data_creation();
	create_descriptor_pool();
	setup_descriptor_set_layout();
	create_descriptor_sets();
	setup_framebuffer();
	create_pipeline();
	init_dynamic_vertex_structures();
	build_command_buffers();
	prepared = true;

	return true;
}
/**
 * 	@fn void vertex_dynamic_state::load_assets()
 *	@brief Loading extra models, textures from assets 
 */
void vertex_dynamic_state::load_assets()
{
	/* Models */
	skybox = load_model("scenes/cube.gltf");
	object = load_model("scenes/cube.gltf");

	/* Load HDR cube map */
	textures.envmap = load_texture_cubemap("textures/uffizi_rgba16f_cube.ktx", vkb::sg::Image::Color);
}

/**
 * 	@fn void vertex_dynamic_state::draw()
 *  @brief Preparing frame and submitting it to the present queue
 */
void vertex_dynamic_state::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void vertex_dynamic_state::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
	if (camera.updated)
		update_uniform_buffers();
}

/**
 * 	@fn void vertex_dynamic_state::prepare_uniform_buffers()
 * 	@brief Preparing uniform buffer (setting bits) and updating UB data
 */
void vertex_dynamic_state::prepare_uniform_buffers()
{
	ubo = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubo_vs), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

/**
 * 	@fn void vertex_dynamic_state::update_uniform_buffers()
 * 	@brief Updating data from application to GPU uniform buffer
 */
void vertex_dynamic_state::update_uniform_buffers()
{
	ubo_vs.projection       = camera.matrices.perspective;
	ubo_vs.modelview        = camera.matrices.view * glm::mat4(1.f);
	ubo_vs.skybox_modelview = camera.matrices.view;
	ubo->convert_and_update(ubo_vs);
}

/**
 * 	@fn void vertex_dynamic_state::create_pipeline()
 * 	@brief Creating graphical pipeline
 * 	@details Preparing pipeline structures:
 * 			 - VkPipelineInputAssemblyStateCreateInfo
 * 			 - VkPipelineRasterizationStateCreateInfo
 * 			 - VkPipelineColorBlendAttachmentState
 * 			 - VkPipelineColorBlendStateCreateInfo
 * 			 - VkPipelineDepthStencilStateCreateInfo
 * 			 - VkPipelineViewportStateCreateInfo
 * 			 - VkPipelineMultisampleStateCreateInfo
 * 			 - VkPipelineDynamicStateCreateInfo
 * 			 - VkPipelineShaderStageCreateInfo
 * 			 - VkPipelineRenderingCreateInfoKHR
 * 			 - VkGraphicsPipelineCreateInfo
 * 
 * 	@note Specific settings that were used to implement Vertex Input Dynamic State extension in this sample:
 * 			 - In VkPipelineDynamicStateCreateInfo use "VK_DYNAMIC_STATE_VERTEX_INPUT_EXT" enumeration in config vector.
 * 			 - In VkGraphicsPipelineCreateInfo "pVertexInputState" element is not require to declare (when using vertex input dynamic state)
 * 
 */
void vertex_dynamic_state::create_pipeline()
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
	        VK_FRONT_FACE_COUNTER_CLOCKWISE,
	        0);

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        0xf,
	        VK_FALSE);

	const auto color_attachment_state = vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments    = &color_attachment_state;

	/* Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept */
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_FALSE,
	        VK_FALSE,
	        VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_VERTEX_INPUT_EXT};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
	shader_stages[0] = load_shader("vertex_dynamic_state/gbuffer.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("vertex_dynamic_state/gbuffer.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	/* Create graphics pipeline for dynamic rendering */
	VkFormat color_rendering_format = render_context->get_format();

	/* Provide information for dynamic rendering */
	VkPipelineRenderingCreateInfoKHR pipeline_create{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
	pipeline_create.pNext                   = VK_NULL_HANDLE;
	pipeline_create.colorAttachmentCount    = 1;
	pipeline_create.pColorAttachmentFormats = &color_rendering_format;
	pipeline_create.depthAttachmentFormat   = depth_format;
	pipeline_create.stencilAttachmentFormat = depth_format;

	/* Use the pNext to point to the rendering create struct */
	VkGraphicsPipelineCreateInfo graphics_create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	graphics_create.pNext               = VK_NULL_HANDLE;
	graphics_create.renderPass          = VK_NULL_HANDLE;
	graphics_create.pInputAssemblyState = &input_assembly_state;
	graphics_create.pRasterizationState = &rasterization_state;
	graphics_create.pColorBlendState    = &color_blend_state;
	graphics_create.pMultisampleState   = &multisample_state;
	graphics_create.pViewportState      = &viewport_state;
	graphics_create.pDepthStencilState  = &depth_stencil_state;
	graphics_create.pDynamicState       = &dynamic_state;
	graphics_create.pVertexInputState   = VK_NULL_HANDLE;
	graphics_create.stageCount          = static_cast<uint32_t>(shader_stages.size());
	graphics_create.pStages             = shader_stages.data();
	graphics_create.layout              = pipeline_layout;

	/* Skybox pipeline (background cube) */
	VkSpecializationInfo                    specialization_info;
	std::array<VkSpecializationMapEntry, 1> specialization_map_entries{};
	specialization_map_entries[0]        = vkb::initializers::specialization_map_entry(0, 0, sizeof(uint32_t));
	uint32_t shadertype                  = 0;
	specialization_info                  = vkb::initializers::specialization_info(1, specialization_map_entries.data(), sizeof(shadertype), &shadertype);
	shader_stages[0].pSpecializationInfo = &specialization_info;
	shader_stages[1].pSpecializationInfo = &specialization_info;

	graphics_create.pNext      = VK_NULL_HANDLE;
	graphics_create.renderPass = render_pass;

	vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &graphics_create, VK_NULL_HANDLE, &skybox_pipeline);
	/* Object rendering pipeline */
	shadertype = 1;

	/* Enable depth test and write */
	depth_stencil_state.depthWriteEnable = VK_TRUE;
	depth_stencil_state.depthTestEnable  = VK_TRUE;
	/* Flip cull mode */
	rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &graphics_create, VK_NULL_HANDLE, &model_pipeline);
}

/**
 * 	@fn void vertex_dynamic_state::build_command_buffers()
 * 	@brief Creating command buffers and drawing particular elements on window.
 * 	@details Drawing object list:
 * 			 - Skybox - cube that have background texture attached (easy way to generate background to scene).
 * 			 - Object - cube that was placed in the middle with some reflection shader effect.
 * 			 - Created model - cube that was created on runtime.
 * 			 - UI - some statistic tab
 * 
 * 	@note In case of Vertex Input Dynamic State feature sample need to create model in runtime because of requirement to have different data structure.
 * 		  By default function "load_model" from framework is parsing data from .gltf files and build it every time in declared structure (see Vertex structure in framework files).
 * 		  Before drawing different models (in case of vertex input data structure) "change_vertex_input_data" fuction is call for dynamically change Vertex Input data.
 */
void vertex_dynamic_state::build_command_buffers()
{
	std::array<VkClearValue, 2> clear_values{};
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	int i = -1;
	for (auto &draw_cmd_buffer : draw_cmd_buffers)
	{
		i++;
		auto command_begin = vkb::initializers::command_buffer_begin_info();
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffer, &command_begin));

		auto draw_scene = [&] {
			VkViewport viewport = vkb::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
			vkCmdSetViewport(draw_cmd_buffer, 0, 1, &viewport);

			VkRect2D scissor = vkb::initializers::rect2D(static_cast<int>(width), static_cast<int>(height), 0, 0);
			vkCmdSetScissor(draw_cmd_buffer, 0, 1, &scissor);

			/* One descriptor set is used, and the draw type is toggled by a specialization constant */
			vkCmdBindDescriptorSets(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

			/* skybox */
			change_vertex_input_data(VERTEX_DYNAMIC_STATE_USE_FRAMEWORK_VERTEX_STRUCT);
			vkCmdSetVertexInputEXT(draw_cmd_buffer, 1, vertex_input_bindings_ext, 2, vertex_attribute_description_ext);
			vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_pipeline);
			draw_model(skybox, draw_cmd_buffer);

			/* object */
			vkCmdSetVertexInputEXT(draw_cmd_buffer, 1, vertex_input_bindings_ext, 2, vertex_attribute_description_ext);
			vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, model_pipeline);
			draw_model(object, draw_cmd_buffer);

			/* Change vertex input architecture from framework "Vertex" to sample "SampleVertex" */
			change_vertex_input_data(VERTEX_DYNAMIC_STATE_USE_EXT_VERTEX_STRUCT);
			vkCmdSetVertexInputEXT(draw_cmd_buffer, 1, vertex_input_bindings_ext, 2, vertex_attribute_description_ext);
			draw_created_model(draw_cmd_buffer);

			/* UI */
			draw_ui(draw_cmd_buffer);
		};

		VkImageSubresourceRange range{};
		range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel   = 0;
		range.levelCount     = VK_REMAINING_MIP_LEVELS;
		range.baseArrayLayer = 0;
		range.layerCount     = VK_REMAINING_ARRAY_LAYERS;

		VkImageSubresourceRange depth_range{range};
		depth_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
		render_pass_begin_info.renderPass               = render_pass;
		render_pass_begin_info.framebuffer              = framebuffers[i];
		render_pass_begin_info.renderArea.extent.width  = width;
		render_pass_begin_info.renderArea.extent.height = height;
		render_pass_begin_info.clearValueCount          = 3;
		render_pass_begin_info.pClearValues             = clear_values.data();

		vkCmdBeginRenderPass(draw_cmd_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		draw_scene();

		vkCmdEndRenderPass(draw_cmd_buffer);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffer));
	}
}

void vertex_dynamic_state::create_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)};
	uint32_t                   num_descriptor_sets = 4;
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void vertex_dynamic_state::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void vertex_dynamic_state::create_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo            matrix_buffer_descriptor     = create_descriptor(*ubo);
	VkDescriptorImageInfo             environment_image_descriptor = create_descriptor(textures.envmap);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets        = {
        vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor),
    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void vertex_dynamic_state::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	/* Enable extension features required by this sample
	   These are passed to device creation via a pNext structure chain */
	auto &requested_vertex_input_features                   = gpu.request_extension_features<VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT);
	requested_vertex_input_features.vertexInputDynamicState = VK_TRUE;

	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = true;
	}
}

/**
 * 	@fn void vertex_dynamic_state::draw_created_model(VkCommandBuffer commandBuffer)
 * 	@brief Drawing created model using indexing buffer
 */
void vertex_dynamic_state::draw_created_model(VkCommandBuffer commandBuffer)
{
	VkDeviceSize offsets[1] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, cube.vertices->get(), offsets);
	vkCmdBindIndexBuffer(commandBuffer, cube.indices->get_handle(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, cube.index_count, 1, 0, 0, 0);
}

/**
 * 	@fn void vertex_dynamic_state::calc_triangle_normal(triangle *tris)
 * 	@brief Calculating normal vector from triangle
 * 	@param tris - pointer to struct triangle
 */
void vertex_dynamic_state::calc_triangle_normal(triangle *tris)
{
	glm::vec3 edge1, edge2, normal_before_normalize;

	edge1                   = tris->B - tris->A;
	edge2                   = tris->C - tris->A;
	normal_before_normalize = glm::cross(edge1, edge2);
	normal_before_normalize *= -1;
	tris->Normal = glm::normalize(normal_before_normalize);
}

/**
 *  @fn void vertex_dynamic_state::model_data_creation()
 *  @brief Generate vertex input data for simple cube (position and normal vectors)
 */
void vertex_dynamic_state::model_data_creation()
{
	const uint32_t vertex_count   = 8;
	const uint32_t triangle_count = 12;
	SampleVertex * vertices       = new SampleVertex[vertex_count];
	triangle *     triangles      = new triangle[triangle_count];

	vertices[0].pos = {0.0f, 0.0f, 0.0f};
	vertices[1].pos = {1.0f, 0.0f, 0.0f};
	vertices[2].pos = {1.0f, 1.0f, 0.0f};
	vertices[3].pos = {0.0f, 1.0f, 0.0f};
	vertices[4].pos = {0.0f, 0.0f, 1.0f};
	vertices[5].pos = {1.0f, 0.0f, 1.0f};
	vertices[6].pos = {1.0f, 1.0f, 1.0f};
	vertices[7].pos = {0.0f, 1.0f, 1.0f};

	for (uint8_t i = 0; i < vertex_count; i++)
	{
		vertices[i].pos *= (glm::vec3){10.0f, 10.0f, 10.0f};
		vertices[i].pos -= (glm::vec3){5.0f, 20.0f, 5.0f};
#if (DEBUG_SAMPLE == 1) /* Macro from cmake to print vertices positions */
		LOGI("Vertices position: X{}, Y{}, Z{}.", vertices[i].pos.x, vertices[i].pos.y, vertices[i].pos.z);
#endif
	}

	triangles[0]  = {vertices[0].pos, vertices[1].pos, vertices[2].pos, (glm::vec3){0.0f, 0.0f, 0.0f}, {0, 1, 2}};
	triangles[1]  = {vertices[2].pos, vertices[3].pos, vertices[0].pos, (glm::vec3){0.0f, 0.0f, 0.0f}, {2, 3, 0}};
	triangles[2]  = {vertices[6].pos, vertices[5].pos, vertices[4].pos, (glm::vec3){0.0f, 0.0f, 0.0f}, {6, 5, 4}};
	triangles[3]  = {vertices[4].pos, vertices[7].pos, vertices[6].pos, (glm::vec3){0.0f, 0.0f, 0.0f}, {4, 7, 6}};
	triangles[4]  = {vertices[5].pos, vertices[1].pos, vertices[0].pos, (glm::vec3){0.0f, 0.0f, 0.0f}, {5, 1, 0}};
	triangles[5]  = {vertices[0].pos, vertices[4].pos, vertices[5].pos, (glm::vec3){0.0f, 0.0f, 0.0f}, {0, 4, 5}};
	triangles[6]  = {vertices[6].pos, vertices[2].pos, vertices[1].pos, (glm::vec3){0.0f, 0.0f, 0.0f}, {6, 2, 1}};
	triangles[7]  = {vertices[1].pos, vertices[5].pos, vertices[6].pos, (glm::vec3){0.0f, 0.0f, 0.0f}, {1, 5, 6}};
	triangles[8]  = {vertices[7].pos, vertices[3].pos, vertices[2].pos, (glm::vec3){0.0f, 0.0f, 0.0f}, {7, 3, 2}};
	triangles[9]  = {vertices[2].pos, vertices[6].pos, vertices[7].pos, (glm::vec3){0.0f, 0.0f, 0.0f}, {2, 6, 7}};
	triangles[10] = {vertices[3].pos, vertices[7].pos, vertices[4].pos, (glm::vec3){0.0f, 0.0f, 0.0f}, {3, 7, 4}};
	triangles[11] = {vertices[4].pos, vertices[0].pos, vertices[3].pos, (glm::vec3){0.0f, 0.0f, 0.0f}, {4, 0, 3}};

	for (uint32_t i = 0; i < vertex_count; i++)
	{
		vertices[i].normal = {0.0, 0.0, 0.0};
	}

	uint32_t  index_count        = triangle_count * 3;
	uint32_t *indices            = new uint32_t[index_count];
	uint32_t  vertex_buffer_size = vertex_count * sizeof(SampleVertex);
	uint32_t  index_buffer_size  = index_count * sizeof(uint32_t);
	cube.index_count             = index_count;

	uint32_t index_counter = 0;
	for (uint8_t i = 0; i < triangle_count; i++)
	{
		for (uint8_t j = 0; j < 3; j++)
		{
			indices[index_counter] = triangles[i].vertices[j];
			index_counter++;
		}
	}

	struct
	{
		VkBuffer       buffer;
		VkDeviceMemory memory;
	} vertex_staging, index_staging;

	vertex_staging.buffer = get_device().create_buffer(
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	    vertex_buffer_size,
	    &vertex_staging.memory,
	    vertices);

	index_staging.buffer = get_device().create_buffer(
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	    index_buffer_size,
	    &index_staging.memory,
	    indices);

	cube.vertices = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                    vertex_buffer_size,
	                                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                    VMA_MEMORY_USAGE_GPU_ONLY);

	cube.indices = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                   index_buffer_size,
	                                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                   VMA_MEMORY_USAGE_GPU_ONLY);

	/* Copy from staging buffers */
	VkCommandBuffer copy_command = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copy_region = {};

	copy_region.size = vertex_buffer_size;
	vkCmdCopyBuffer(
	    copy_command,
	    vertex_staging.buffer,
	    cube.vertices->get_handle(),
	    1,
	    &copy_region);

	copy_region.size = index_buffer_size;
	vkCmdCopyBuffer(
	    copy_command,
	    index_staging.buffer,
	    cube.indices->get_handle(),
	    1,
	    &copy_region);

	device->flush_command_buffer(copy_command, queue, true);

	vkDestroyBuffer(get_device().get_handle(), vertex_staging.buffer, nullptr);
	vkFreeMemory(get_device().get_handle(), vertex_staging.memory, nullptr);
	vkDestroyBuffer(get_device().get_handle(), index_staging.buffer, nullptr);
	vkFreeMemory(get_device().get_handle(), index_staging.memory, nullptr);

	delete[] vertices;
	delete[] triangles;
	delete[] indices;
}
/**
 * 	@fn void vertex_dynamic_state::init_dynamic_vertex_structures()
 * 	@brief Initialize specific structures related to Vertex Input Dynamic State extension
 */
void vertex_dynamic_state::init_dynamic_vertex_structures()
{
	vertex_input_bindings_ext[0].sType     = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
	vertex_input_bindings_ext[0].binding   = 0;
	vertex_input_bindings_ext[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertex_input_bindings_ext[0].divisor   = 1;

	vertex_attribute_description_ext[0].sType    = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
	vertex_attribute_description_ext[0].location = 0;
	vertex_attribute_description_ext[0].binding  = 0;

	vertex_attribute_description_ext[1].sType    = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
	vertex_attribute_description_ext[1].location = 1;
	vertex_attribute_description_ext[1].binding  = 0;
}

/**
 * 	@fn void vertex_dynamic_state::change_vertex_input_data(vertexDynamicStateVertexStruct_t variant)
 * 	@brief Change Vertex Input data structure dynamically in runtime.
 */
void vertex_dynamic_state::change_vertex_input_data(vertexDynamicStateVertexStruct_t variant)
{
	if (variant == VERTEX_DYNAMIC_STATE_USE_FRAMEWORK_VERTEX_STRUCT)
	{
		vertex_input_bindings_ext[0].stride = sizeof(Vertex);

		vertex_attribute_description_ext[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_description_ext[0].offset = 0;

		vertex_attribute_description_ext[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_description_ext[1].offset = sizeof(float) * 3;
	}
	else if (variant == VERTEX_DYNAMIC_STATE_USE_EXT_VERTEX_STRUCT)
	{
		/* MK: binding information for second vertex input data architecture) */

		vertex_input_bindings_ext[0].stride = sizeof(SampleVertex);

		vertex_attribute_description_ext[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_description_ext[0].offset = 0;

		vertex_attribute_description_ext[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_description_ext[1].offset = 2 * (sizeof(float) * 3);
	}
}
std::unique_ptr<vkb::VulkanSample> create_vertex_dynamic_state()
{
	return std::make_unique<vertex_dynamic_state>();
}
