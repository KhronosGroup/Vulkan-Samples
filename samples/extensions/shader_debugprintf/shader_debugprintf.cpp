/* Copyright (c) 2024, Sascha Willems
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

#include "shader_debugprintf.h"

#include "scene_graph/components/sub_mesh.h"

std::string ShaderDebugPrintf::debug_output{};

VKAPI_ATTR VkBool32 VKAPI_CALL ShaderDebugPrintf::debug_utils_message_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void                                       *pUserData)
{
	if (strcmp(pCallbackData->pMessageIdName, "WARNING-DEBUG-PRINTF") == 0)
	{
		// Validation messages are a bit verbose, but we only want the text from the shader, so we cut off everything before the first word from the shader message
		// See scene.vert: debugPrintfEXT("Position = %v4f", outPos);
		std::string shader_message{pCallbackData->pMessage};
		shader_message = shader_message.substr(shader_message.find("Position"));
		debug_output.append(shader_message + "\n");
	}
	return VK_FALSE;
}

ShaderDebugPrintf::ShaderDebugPrintf()
{
	title = "Shader debugprintf";

	add_device_extension(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);

#if defined(VK_EXT_layer_settings)
	// If layer settings available, use it to configure validation layer for debugPrintfEXT
	add_instance_extension(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME, /*optional*/ false);

	VkLayerSettingEXT layerSetting;
	layerSetting.pLayerName = "VK_LAYER_KHRONOS_validation";
	layerSetting.pSettingName = "enables";
	layerSetting.type = VK_LAYER_SETTING_TYPE_STRING_EXT;
	layerSetting.valueCount = 1;

	static const char * layerEnables = "VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT";
	layerSetting.pValues = &layerEnables;

	add_layer_setting(layerSetting);
#endif
}

ShaderDebugPrintf::~ShaderDebugPrintf()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipelines.skysphere, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.sphere, nullptr);

		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);

		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);

		vkDestroySampler(get_device().get_handle(), textures.skysphere.sampler, nullptr);
	}

	if (has_instance())
	{
		vkDestroyDebugUtilsMessengerEXT(get_instance().get_handle(), debug_utils_messenger, nullptr);
	}
}

void ShaderDebugPrintf::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

void ShaderDebugPrintf::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass            = render_pass;
	render_pass_begin_info.renderArea.offset.x   = 0;
	render_pass_begin_info.renderArea.offset.y   = 0;
	render_pass_begin_info.clearValueCount       = 2;
	render_pass_begin_info.pClearValues          = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		VkClearValue clear_values[2];
		clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
		clear_values[1].depthStencil = {0.0f, 0};

		// Final composition
		VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
		render_pass_begin_info.framebuffer              = framebuffers[i];
		render_pass_begin_info.renderPass               = render_pass;
		render_pass_begin_info.clearValueCount          = 2;
		render_pass_begin_info.renderArea.extent.width  = width;
		render_pass_begin_info.renderArea.extent.height = height;
		render_pass_begin_info.pClearValues             = clear_values;

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		if (display_skysphere)
		{
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skysphere);
			push_const_block.object_type = 0;
			vkCmdPushConstants(draw_cmd_buffers[i], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_const_block), &push_const_block);
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.skysphere, 0, nullptr);

			draw_model(models.skysphere, draw_cmd_buffers[i]);
		}

		// Spheres
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.sphere);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.sphere, 0, nullptr);
		std::vector<glm::vec3> mesh_colors = {
		    glm::vec3(1.0f, 0.0f, 0.0f),
		    glm::vec3(0.0f, 1.0f, 0.0f),
		    glm::vec3(0.0f, 0.0f, 1.0f),
		};
		std::vector<glm::vec3> mesh_offsets = {
		    glm::vec3(-2.5f, 0.0f, 0.0f),
		    glm::vec3(0.0f, 0.0f, 0.0f),
		    glm::vec3(2.5f, 0.0f, 0.0f),
		};
		for (uint32_t j = 0; j < 3; j++)
		{
			push_const_block.object_type = 1;
			push_const_block.offset      = glm::vec4(mesh_offsets[j], 0.0f);
			push_const_block.color       = glm::vec4(mesh_colors[j], 0.0f);
			vkCmdPushConstants(draw_cmd_buffers[i], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_const_block), &push_const_block);
			draw_model(models.scene, draw_cmd_buffers[i]);
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void ShaderDebugPrintf::load_assets()
{
	models.skysphere   = load_model("scenes/geosphere.gltf");
	textures.skysphere = load_texture("textures/skysphere_rgba.ktx", vkb::sg::Image::Color);
	models.scene       = load_model("scenes/geosphere.gltf");
}

void ShaderDebugPrintf::setup_descriptor_pool()
{
	// Note: Using debugprintf in a shader consumes a descriptor set, so we need to allocate one additional descriptor set
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)};
	uint32_t                   num_descriptor_sets = 2;
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void ShaderDebugPrintf::setup_descriptor_set_layout()
{
	// Object rendering (into offscreen buffer)
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

	// Pass object offset and color via push constant
	VkPushConstantRange push_constant_range            = vkb::initializers::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(push_const_block), 0);
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void ShaderDebugPrintf::setup_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	// Sphere model object descriptor set
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.sphere));

	VkDescriptorBufferInfo            matrix_buffer_descriptor     = create_descriptor(*uniform_buffers.matrices);
	VkDescriptorImageInfo             environment_image_descriptor = create_descriptor(textures.skysphere);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets        = {
        vkb::initializers::write_descriptor_set(descriptor_sets.sphere, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.sphere, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor),
    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

	// Sky sphere descriptor set
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.skysphere));

	matrix_buffer_descriptor     = create_descriptor(*uniform_buffers.matrices);
	environment_image_descriptor = create_descriptor(textures.skysphere);
	write_descriptor_sets        = {
        vkb::initializers::write_descriptor_set(descriptor_sets.skysphere, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.skysphere, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor),
    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void ShaderDebugPrintf::prepare_pipelines()
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

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
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
	    VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(
	        pipeline_layout,
	        render_pass,
	        0);

	std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_states = {
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	};

	// Vertex bindings an attributes for model rendering
	// Binding description
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

	// Attribute descriptions
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                        // Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),        // Normal
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6),           // UV
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	pipeline_create_info.layout              = pipeline_layout;
	pipeline_create_info.renderPass          = render_pass;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.pVertexInputState   = &vertex_input_state;
	pipeline_create_info.stageCount          = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();

	shader_stages[0] = load_shader("shader_debugprintf", "scene.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("shader_debugprintf", "scene.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// skysphere pipeline (background cube)
	rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.skysphere));

	// sphere model pipeline
	depth_stencil_state.depthWriteEnable = VK_TRUE;
	depth_stencil_state.depthTestEnable  = VK_TRUE;
	// Flip cull mode
	rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.sphere));
}

// Prepare and initialize uniform buffer containing shader uniforms
void ShaderDebugPrintf::prepare_uniform_buffers()
{
	// Matrices vertex shader uniform buffer
	uniform_buffers.matrices = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                               sizeof(ubo_vs),
	                                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                               VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void ShaderDebugPrintf::update_uniform_buffers()
{
	ubo_vs.projection          = camera.matrices.perspective;
	ubo_vs.modelview           = camera.matrices.view * glm::mat4(1.0f);
	ubo_vs.skysphere_modelview = camera.matrices.view;
	uniform_buffers.matrices->convert_and_update(ubo_vs);
}

void ShaderDebugPrintf::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

bool ShaderDebugPrintf::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	// Register debug utils callback here vs in ShaderDebugPrintf::create_instance() so it works with both override and layer settings
	VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	debug_utils_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
	debug_utils_messenger_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	debug_utils_messenger_create_info.pfnUserCallback = debug_utils_message_callback;
	VK_CHECK(vkCreateDebugUtilsMessengerEXT(get_instance().get_handle(), &debug_utils_messenger_create_info, nullptr, &debug_utils_messenger));

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -6.0f));
	camera.set_rotation(glm::vec3(0.0f, 180.0f, 0.0f));

	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	load_assets();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_sets();
	build_command_buffers();
	prepared = true;
	return true;
}

#if defined(VK_EXT_layer_settings)
// Currently the sample calls through this function in order to get the list of any instance layers, not just validation layers.
// This is not suitable for a real application implementation using the layer, the layer will need to be shipped with the application.
const std::vector<const char*> ShaderDebugPrintf::get_validation_layers()
{
	// Always enable validation layer for access to debugPrintfEXT feature, even for release builds
	return {"VK_LAYER_KHRONOS_validation"};
}
#else
// This sample overrides the instance creation part of the framework to chain in additional structures
// Not required when layer settings available, since debugPrintfEXT feature is enabled using standard framework
std::unique_ptr<vkb::Instance> ShaderDebugPrintf::create_instance(bool headless)
{
	std::vector<const char *> enabled_extensions;
	enabled_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

	for (const char *extension_name : window->get_required_surface_extensions())
	{
		enabled_extensions.push_back(extension_name);
	}

	enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	enabled_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#if (defined(VKB_ENABLE_PORTABILITY))
	enabled_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

	VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	app_info.pApplicationName = "Shader debugprintf";
	app_info.pEngineName      = "Vulkan Samples";
	app_info.apiVersion       = VK_API_VERSION_1_0;

	// Shader printf is a feature of the validation layers that needs to be enabled
	std::vector<VkValidationFeatureEnableEXT> validation_feature_enables = {VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT};

	VkValidationFeaturesEXT validation_features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
	validation_features.enabledValidationFeatureCount = static_cast<uint32_t>(validation_feature_enables.size());
	validation_features.pEnabledValidationFeatures    = validation_feature_enables.data();

	std::vector<const char *> validation_layers = {"VK_LAYER_KHRONOS_validation"};

	VkInstanceCreateInfo instance_create_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	instance_create_info.ppEnabledExtensionNames = enabled_extensions.data();
	instance_create_info.enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions.size());
	instance_create_info.pApplicationInfo        = &app_info;
	instance_create_info.ppEnabledLayerNames     = validation_layers.data();
	instance_create_info.enabledLayerCount       = static_cast<uint32_t>(validation_layers.size());
#if (defined(VKB_ENABLE_PORTABILITY))
	instance_create_info.flags					|= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
	instance_create_info.pNext                   = &validation_features;

	VkInstance vulkan_instance;
	VkResult   result = vkCreateInstance(&instance_create_info, nullptr, &vulkan_instance);

	if (result != VK_SUCCESS)
	{
		throw vkb::VulkanException{result, "Could not create instance"};
	}

	volkLoadInstance(vulkan_instance);

	return std::make_unique<vkb::Instance>(vulkan_instance, enabled_extensions);
}
#endif

void ShaderDebugPrintf::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (camera.updated)
	{
		update_uniform_buffers();
	}
}

void ShaderDebugPrintf::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.checkbox("skysphere", &display_skysphere))
		{
			rebuild_command_buffers();
		}
	}
	if (drawer.header("Debug output"))
	{
		drawer.text(debug_output.c_str());
	}

	// Clear saved debug output, so we only get output for the last frame
	debug_output.clear();
}

bool ShaderDebugPrintf::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

std::unique_ptr<vkb::Application> create_shader_debugprintf()
{
	return std::make_unique<ShaderDebugPrintf>();
}
