/* Copyright (c) 2022-2023, Sascha Willems
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
 * Graphics pipeline libraries
 *
 * Note: Requires a device that supports the VK_EXT_graphics_pipeline_library
 *
 * Creates a pipeline library for shared pipeline parts like vertex input and fragment output interfaces. These pre-built pipeline
 * "building blocks" are then used for runtime pipeline creation, which wlll be faster than always creating a full pipeline
 */

#include "graphics_pipeline_library.h"

#include "scene_graph/components/sub_mesh.h"
#include <glsl_compiler.h>

void GraphicsPipelineLibrary::pipeline_creation_threadfn()
{
	const std::lock_guard<std::mutex> lock(mutex);

	auto start = std::chrono::steady_clock::now();

	prepare_new_pipeline();
	new_pipeline_created = true;

	// Change viewport/draw count
	if (pipelines.size() > split_x * split_y)
	{
		split_x++;
		split_y++;
	}

	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
	LOGD("Pipeline created in {} ms", milliseconds.count());
}

GraphicsPipelineLibrary::GraphicsPipelineLibrary()
{
	title = "Graphics pipeline library";

	// Graphics pipeline library related etensions required by this sample
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
	add_device_extension(VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME);
}

void GraphicsPipelineLibrary::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable extension features required by this sample
	VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT &requested_graphics_pipeline_libary_features = gpu.request_extension_features<VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT);
	requested_graphics_pipeline_libary_features.graphicsPipelineLibrary                             = VK_TRUE;
}

GraphicsPipelineLibrary::~GraphicsPipelineLibrary()
{
	if (device)
	{
		for (auto pipeline : pipelines)
		{
			vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		}
		for (auto pipeline : pipeline_library.fragment_shaders)
		{
			vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		}
		vkDestroyPipelineCache(get_device().get_handle(), thread_pipeline_cache, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipeline_library.vertex_input_interface, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipeline_library.pre_rasterization_shaders, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipeline_library.fragment_output_interface, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
	}
}

void GraphicsPipelineLibrary::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.033f, 0.0f}};
	clear_values[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass            = render_pass;
	render_pass_begin_info.renderArea.offset.x   = 0;
	render_pass_begin_info.renderArea.offset.y   = 0;
	render_pass_begin_info.clearValueCount       = 2;
	render_pass_begin_info.pClearValues          = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
		render_pass_begin_info.framebuffer              = framebuffers[i];
		render_pass_begin_info.renderPass               = render_pass;
		render_pass_begin_info.clearValueCount          = 2;
		render_pass_begin_info.renderArea.extent.width  = width;
		render_pass_begin_info.renderArea.extent.height = height;
		render_pass_begin_info.pClearValues             = clear_values;

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

		float w = static_cast<float>(width) / static_cast<float>(split_x);
		float h = static_cast<float>(height) / static_cast<float>(split_y);

		uint32_t idx = 0;
		for (uint32_t y = 0; y < split_y; y++)
		{
			for (uint32_t x = 0; x < split_x; x++)
			{
				VkViewport viewport{};
				viewport.x        = w * static_cast<float>(x);
				viewport.y        = h * static_cast<float>(y);
				viewport.width    = w;
				viewport.height   = h;
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;
				vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

				VkRect2D scissor{};
				scissor.extent.width  = static_cast<uint32_t>(w);
				scissor.extent.height = static_cast<uint32_t>(h);
				scissor.offset.x      = static_cast<uint32_t>(w) * x;
				scissor.offset.y      = static_cast<uint32_t>(h) * y;
				vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

				if (pipelines.size() > idx)
				{
					vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[idx]);
					vkCmdPushConstants(draw_cmd_buffers[i], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec4), &colors[idx % colors.size()]);
					draw_model(scene, draw_cmd_buffers[i]);
				}

				idx++;
			}
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void GraphicsPipelineLibrary::load_assets()
{
	scene = load_model("scenes/teapot.gltf");
}

void GraphicsPipelineLibrary::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)};
	uint32_t                   num_descriptor_sets = 1;
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void GraphicsPipelineLibrary::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);

	// Pass random colors using push constants
	VkPushConstantRange push_constant_range{};
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push_constant_range.offset     = 0;
	push_constant_range.size       = sizeof(glm::vec4);

	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void GraphicsPipelineLibrary::setup_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo            uniform_buffer_descriptor = create_descriptor(*uniform_buffer);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets     = {
        vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniform_buffer_descriptor),
    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

// Compiling shaders can be simplified with the new extension, so we only require code to generate the SPIR-V in this sample
void GraphicsPipelineLibrary::compile_shader(const std::string filename, VkShaderStageFlagBits shader_stage, std::vector<uint32_t> &spirv)
{
	vkb::GLSLCompiler glsl_compiler;
	auto              buffer = vkb::fs::read_shader_binary(filename);
	std::string       info_log;
	if (!glsl_compiler.compile_to_spirv(shader_stage, buffer, "main", {}, spirv, info_log))
	{
		LOGE("Failed to compile shader, Error: {}", info_log.c_str());
		throw std::runtime_error{"Failed to compile shader"};
	}
}

// This function pre-builts shared pipeline parts ("pipeline library")
// E.g. vertex input and fragment out interface, which are the same for all pipelines created in this sample
void GraphicsPipelineLibrary::prepare_pipeline_library()
{
	// Create a pipeline library for the vertex input interface
	{
		VkGraphicsPipelineLibraryCreateInfoEXT library_info{};
		library_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
		library_info.flags = VK_GRAPHICS_PIPELINE_LIBRARY_VERTEX_INPUT_INTERFACE_BIT_EXT;

		VkPipelineInputAssemblyStateCreateInfo       input_assembly_state  = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineVertexInputStateCreateInfo         vertex_input_state    = vkb::initializers::pipeline_vertex_input_state_create_info();
		std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
		    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
		};
		std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
		    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                        // Position
		    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),        // Normal
		    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6),           // UV
		};
		vertex_input_state.vertexBindingDescriptionCount   = static_cast<uint32_t>(vertex_input_bindings.size());
		vertex_input_state.pVertexBindingDescriptions      = vertex_input_bindings.data();
		vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes.size());
		vertex_input_state.pVertexAttributeDescriptions    = vertex_input_attributes.data();

		VkGraphicsPipelineCreateInfo pipeline_library_create_info{};
		pipeline_library_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_library_create_info.flags               = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
		pipeline_library_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_library_create_info.pNext               = &library_info;
		pipeline_library_create_info.pInputAssemblyState = &input_assembly_state;
		pipeline_library_create_info.pVertexInputState   = &vertex_input_state;

		VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_library_create_info, nullptr, &pipeline_library.vertex_input_interface));
	}

	// Creata a pipeline library for the vertex shader stage
	{
		VkGraphicsPipelineLibraryCreateInfoEXT library_info{};
		library_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
		library_info.flags = VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT;

		VkDynamicState vertexDynamicStates[2] = {
		    VK_DYNAMIC_STATE_VIEWPORT,
		    VK_DYNAMIC_STATE_SCISSOR};

		VkPipelineDynamicStateCreateInfo dynamicInfo{};
		dynamicInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicInfo.dynamicStateCount = 2;
		dynamicInfo.pDynamicStates    = vertexDynamicStates;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount                     = 1;
		viewportState.scissorCount                      = 1;

		VkPipelineRasterizationStateCreateInfo rasterizationState = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);

		// Using the pipeline library extension, we can skip the pipeline shader module creation and directly pass the shader code to the pipeline
		std::vector<uint32_t> spirv;
		compile_shader("graphics_pipeline_library/shared.vert", VK_SHADER_STAGE_VERTEX_BIT, spirv);

		VkShaderModuleCreateInfo shader_module_create_info{};
		shader_module_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_module_create_info.codeSize = static_cast<uint32_t>(spirv.size()) * sizeof(uint32_t);
		shader_module_create_info.pCode    = spirv.data();

		VkPipelineShaderStageCreateInfo shader_Stage_create_info{};
		shader_Stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_Stage_create_info.pNext = &shader_module_create_info;
		shader_Stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_Stage_create_info.pName = "main";

		VkGraphicsPipelineCreateInfo pipeline_library_create_info{};
		pipeline_library_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_library_create_info.pNext               = &library_info;
		pipeline_library_create_info.renderPass          = render_pass;
		pipeline_library_create_info.flags               = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
		pipeline_library_create_info.stageCount          = 1;
		pipeline_library_create_info.pStages             = &shader_Stage_create_info;
		pipeline_library_create_info.layout              = pipeline_layout;
		pipeline_library_create_info.pDynamicState       = &dynamicInfo;
		pipeline_library_create_info.pViewportState      = &viewportState;
		pipeline_library_create_info.pRasterizationState = &rasterizationState;

		VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_library_create_info, nullptr, &pipeline_library.pre_rasterization_shaders));
	}

	// Create a pipeline library for the fragment output interface
	{
		VkGraphicsPipelineLibraryCreateInfoEXT library_info{};
		library_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
		library_info.flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_OUTPUT_INTERFACE_BIT_EXT;

		VkPipelineColorBlendAttachmentState  blend_attachment_state = vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo  color_blend_state      = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);
		VkPipelineMultisampleStateCreateInfo multisample_state      = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

		VkGraphicsPipelineCreateInfo pipeline_library_create_info{};
		pipeline_library_create_info.sType             = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_library_create_info.pNext             = &library_info;
		pipeline_library_create_info.layout            = pipeline_layout;
		pipeline_library_create_info.renderPass        = render_pass;
		pipeline_library_create_info.flags             = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
		pipeline_library_create_info.pColorBlendState  = &color_blend_state;
		pipeline_library_create_info.pMultisampleState = &multisample_state;

		VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_library_create_info, nullptr, &pipeline_library.fragment_output_interface));
	}
}

void GraphicsPipelineLibrary::prepare_new_pipeline()
{
	// Create the fragment shader part of the pipeline library with some random options
	VkGraphicsPipelineLibraryCreateInfoEXT library_info{};
	library_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
	library_info.flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_SHADER_BIT_EXT;

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineMultisampleStateCreateInfo  multisample_state   = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

	// Using the pipeline library extension, we can skip the pipeline shader module creation and directly pass the shader code to the pipeline
	std::vector<uint32_t> spirv;
	compile_shader("graphics_pipeline_library/uber.frag", VK_SHADER_STAGE_FRAGMENT_BIT, spirv);

	VkShaderModuleCreateInfo shader_module_create_info{};
	shader_module_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_create_info.codeSize = static_cast<uint32_t>(spirv.size()) * sizeof(uint32_t);
	shader_module_create_info.pCode    = spirv.data();

	VkPipelineShaderStageCreateInfo shader_Stage_create_info{};
	shader_Stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_Stage_create_info.pNext = &shader_module_create_info;
	shader_Stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_Stage_create_info.pName = "main";

	// Select lighting model using a specialization constant
	srand(static_cast<unsigned int>(time(NULL)));
	uint32_t lighting_model = (rand() % 3);

	// Each shader constant of a shader stage corresponds to one map entry
	VkSpecializationMapEntry specialization_map_entry{};
	specialization_map_entry.constantID = 0;
	specialization_map_entry.size       = sizeof(uint32_t);

	VkSpecializationInfo specialization_info{};
	specialization_info.mapEntryCount = 1;
	specialization_info.pMapEntries   = &specialization_map_entry;
	specialization_info.dataSize      = sizeof(uint32_t);
	specialization_info.pData         = &lighting_model;

	shader_Stage_create_info.pSpecializationInfo = &specialization_info;

	VkGraphicsPipelineCreateInfo pipeline_library_create_info{};
	pipeline_library_create_info.sType              = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_library_create_info.pNext              = &library_info;
	pipeline_library_create_info.flags              = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
	pipeline_library_create_info.stageCount         = 1;
	pipeline_library_create_info.pStages            = &shader_Stage_create_info;
	pipeline_library_create_info.layout             = pipeline_layout;
	pipeline_library_create_info.renderPass         = render_pass;
	pipeline_library_create_info.pDepthStencilState = &depth_stencil_state;
	pipeline_library_create_info.pMultisampleState  = &multisample_state;

	VkPipeline fragment_shader = VK_NULL_HANDLE;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), thread_pipeline_cache, 1, &pipeline_library_create_info, nullptr, &fragment_shader));

	// Create the pipeline using the pre-built pipeline library parts
	// Except for above fragment shader part all parts have been pre-built and will be re-used
	std::vector<VkPipeline> libraries = {
	    pipeline_library.vertex_input_interface,
	    pipeline_library.pre_rasterization_shaders,
	    fragment_shader,
	    pipeline_library.fragment_output_interface};

	// Link the library parts into a graphics pipeline
	VkPipelineLibraryCreateInfoKHR linking_info{};
	linking_info.sType        = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
	linking_info.libraryCount = static_cast<uint32_t>(libraries.size());
	linking_info.pLibraries   = libraries.data();

	VkGraphicsPipelineCreateInfo executable_pipeline_create_info{};
	executable_pipeline_create_info.sType  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	executable_pipeline_create_info.pNext  = &linking_info;
	executable_pipeline_create_info.layout = pipeline_layout;
	if (link_time_optimization)
	{
		// If link time optimization is activated in the UI, we set the VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT flag which will let the implementation do additional optimizations at link time
		// This trades in pipeline creation time for run-time performance
		executable_pipeline_create_info.flags = VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT;
	}

	VkPipeline executable = VK_NULL_HANDLE;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), thread_pipeline_cache, 1, &executable_pipeline_create_info, nullptr, &executable));

	pipelines.push_back(executable);

	// Add the fragment shader we created to a deletion list
	pipeline_library.fragment_shaders.push_back(fragment_shader);
}

// Prepare and initialize uniform buffer containing shader uniforms
void GraphicsPipelineLibrary::prepare_uniform_buffers()
{
	// Matrices vertex shader uniform buffer
	uniform_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                     sizeof(ubo_vs),
	                                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                     VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void GraphicsPipelineLibrary::update_uniform_buffers()
{
	camera.set_perspective(45.0f, (static_cast<float>(width) / static_cast<float>(split_x)) / (static_cast<float>(height) / static_cast<float>(split_y)), 0.1f, 256.0f);

	ubo_vs.projection = camera.matrices.perspective;
	ubo_vs.modelview  = camera.matrices.view * glm::rotate(glm::mat4(1.0f), glm::radians(accumulated_time * 360.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo_vs.modelview  = glm::rotate(ubo_vs.modelview, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	uniform_buffer->convert_and_update(ubo_vs);
}

void GraphicsPipelineLibrary::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

bool GraphicsPipelineLibrary::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -7.0f));
	camera.set_rotation(glm::vec3(-30.0f, 0.0f, 0.0f));

	load_assets();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipeline_library();
	setup_descriptor_pool();
	setup_descriptor_sets();
	build_command_buffers();

	// Set up some random colors
	std::random_device                    rnd_device;
	std::default_random_engine            rnd{rnd_device()};
	std::uniform_real_distribution<float> color_distribution{0.2f, 0.8f};
	colors.resize(16);
	for (size_t i = 0; i < colors.size(); i++)
	{
		colors[i].r = color_distribution(rnd);
		colors[i].g = color_distribution(rnd);
		colors[i].b = color_distribution(rnd);
	}

	// Create a separate pipeline cache for the pipeline creation thread
	VkPipelineCacheCreateInfo pipeline_cache_create_info = {};
	pipeline_cache_create_info.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	vkCreatePipelineCache(get_device().get_handle(), &pipeline_cache_create_info, nullptr, &thread_pipeline_cache);

	// Create first pipeline using a background thread
	std::thread pipeline_generation_thread(&GraphicsPipelineLibrary::pipeline_creation_threadfn, this);
	pipeline_generation_thread.detach();

	prepared = true;
	return true;
}

void GraphicsPipelineLibrary::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	if (new_pipeline_created)
	{
		new_pipeline_created = false;
		build_command_buffers();
	}
	draw();

	accumulated_time += 0.2f * delta_time;
	accumulated_time = glm::fract(accumulated_time);

	update_uniform_buffers();
}

void GraphicsPipelineLibrary::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		(drawer.checkbox("Link time optimization", &link_time_optimization));
		if (drawer.button("Add pipeline"))
		{
			// Spwan a thread to create a new pipeline in the background
			std::thread pipeline_generation_thread(&GraphicsPipelineLibrary::pipeline_creation_threadfn, this);
			pipeline_generation_thread.detach();
		}
	}
}

bool GraphicsPipelineLibrary::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

std::unique_ptr<vkb::Application> create_graphics_pipeline_library()
{
	return std::make_unique<GraphicsPipelineLibrary>();
}
