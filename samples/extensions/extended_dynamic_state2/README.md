


# Extended dynamic state 2

**SCREENSHOT**

## Overview

This sample demostrates hot to use `VK_EXT_extended_dynamic_state2` extension, which eliminates the need to create multiple pipeline in case of specific different parameters.

This extension changes how `Depth Bias`, `Primitive Restart`, `Rasterizer Discard` and `Patch Control Points` are managed. Instead of static description during pipeline creation, this extension allows developers to change those parameters by using function before every draw.

Below is a comparison of common Vulkan static and dynamic implementation of those extensions with additional usage of `vkCmdSetPrimitiveTopologyEXT` extension from dynamic state . 
  
| Static/Non-dynamic | Dynamic State 2 |
| ------------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------------------- |
| dynamic_state = {} | dynamic_state = {VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE_EXT,<br>VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE_EXT,<br>VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT,<br>VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE_EXT} |
| vkCreateGraphicsPipelines(pipeline1)<br>vkCreateGraphicsPipelines(pipeline2)<br>vkCreateGraphicsPipelines(pipeline3)<br>vkCreateGraphicsPipelines(pipeline4) | vkCreateGraphicsPipelines(pipeline1)<br>vkCreateGraphicsPipelines(pipeline2) |
| draw(model1, pipeline1)<br>draw(model2, pipeline2)<br>draw(model3, pipeline3)<br>draw(model4, pipeline4) | vkCmdSetPrimitiveRestartEnableEXT(model1, primitiveBoolParam)<br>vkCmdSetDepthBiasEnableEXT(model3, depthBiasBoolParam)<br>vkCmdSetRasterizerDiscardEnableEXT(model1,rasterizerBoolParam)<br>vkCmdSetPrimitiveTopologyEXT(model1, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)<br>draw(model1, pipeline1)<br>vkCmdSetPrimitiveTopologyEXT(model2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)<br>vkCmdSetPrimitiveRestartEnableEXT(model2, primitiveBoolParam)<br>draw(model2, pipeline1)<br>vkCmdSetDepthBiasEnableEXT(model3, depthBiasBoolParam)<br>vkCmdSetPrimitiveRestartEnableEXT(model3, primitiveBoolParam)<br>draw(model3, pipeline1)<br>vkCmdSetPatchControlPointsEXT(model4, patchControlPoints)<br>draw(model4, pipeline2) |

More details are provided in the sections that follow.

## Pipelines
Previously developers had to create multiple pipelines for different parameters in `Depth Bias`, `Primitive Restart`, `Rasterizer Discard` and `Patch Control Points`. This is illustrated in static/non-dynamic pipeline creation.

```C++
...
	// First pipeline Creation
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

	rasterization_state.depthBiasConstantFactor = 1.0f;
	rasterization_state.depthBiasSlopeFactor    = 1.0f;
	rasterization_state.depthBiasClamp          = 0.0f;

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	        VK_TRUE);

	blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

	/* Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept */
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE, /* changed */
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
	    VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	// Vertex bindings an attributes for model rendering
	// Binding description
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	// Attribute descriptions
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Position
	    vkb::initializers::vertex_input_attribute_description(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Normal
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	std::array<VkPipelineShaderStageCreateInfo, 4> shader_stages{};
	shader_stages[0] = load_shader("extended_dynamic_state2/baseline.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("extended_dynamic_state2/baseline.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

...

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
	graphics_create.pVertexInputState   = &vertex_input_state;
	graphics_create.pTessellationState  = VK_NULL_HANDLE;
	graphics_create.stageCount          = 2;
	graphics_create.pStages             = shader_stages.data();
	graphics_create.layout              = pipeline_layouts.baseline;

	graphics_create.pNext      = VK_NULL_HANDLE;
	graphics_create.renderPass = render_pass;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipeline1));

    // Second pipeline creation
    rasterization_state.rasterizerDiscardEnable = VK_TRUE

    VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipeline2));

    // Third pipeline creation
    rasterization_state.rasterizerDiscardEnable = VK_FALSE
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    input_assembly_state.primitiveRestartEnable = VK_TRUE

    VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipeline3));

    // Fourth pipeline creation
    VkPipelineTessellationStateCreateInfo tessellation_state =
	    vkb::initializers::pipeline_tessellation_state_create_info(3);
    graphics_create.layout = pipeline_layouts.tesselation;
    graphics_create.pTessellationState  = &tessellation_state;
    
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
    input_assembly_state.primitiveRestartEnable = VK_FALSE
    rasterization_state.depthBiasEnable = VK_TRUE;
    
    vertex_input_state.vertexBindingDescriptionCount   = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions      = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions    = vertex_input_attributes.data();

	if (get_device().get_gpu().get_features().fillModeNonSolid)
	{
		rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;        //VK_POLYGON_MODE_LINE; /* Wireframe mode */
	}

	shader_stages[0]           = load_shader("extended_dynamic_state2/tess.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]           = load_shader("extended_dynamic_state2/tess.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	shader_stages[2]           = load_shader("extended_dynamic_state2/tess.tesc", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
	shader_stages[3]           = load_shader("extended_dynamic_state2/tess.tese", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
	graphics_create.stageCount = static_cast<uint32_t>(shader_stages.size());
	graphics_create.pStages    = shader_stages.data();
	/* Enable depth test and write */
	depth_stencil_state.depthWriteEnable = VK_TRUE;
	depth_stencil_state.depthTestEnable  = VK_TRUE;
    
    VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipeline4));
...
```

In approach above, if we would want to change patch control points number, then for each different number we would need to create new pipeline.

However, with dynamic state 2 the number of pipelines can be reduced because of possibility to change parameters of `Depth Bias`, `Primitive Restart`, `Rasterizer Discard` and `Patch Control Points` by calling `cmdSetDepthBiasEnableEXT`, `cmdSetPrimitiveRestartEnableEXT`, `cmdSetRasterizerDiscardEnableEXT` and `cmdSetPatchControlPointsEXT` before `draw_model`.

With usage of above functions we can reduce number of pipelines.

```C+
...
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

	rasterization_state.depthBiasConstantFactor = 1.0f;
	rasterization_state.depthBiasSlopeFactor    = 1.0f;
	rasterization_state.depthBiasClamp          = 0.0f;

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	        VK_TRUE);

	blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

	/* Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept */
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE, /* changed */
	        VK_TRUE,
	        VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0);

	VkPipelineTessellationStateCreateInfo tessellation_state =
	    vkb::initializers::pipeline_tessellation_state_create_info(3);


	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT,
	    VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE_EXT,
	    VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE_EXT,
	    VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE_EXT,
	};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	// Vertex bindings an attributes for model rendering
	// Binding description
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	// Attribute descriptions
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Position
	    vkb::initializers::vertex_input_attribute_description(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Normal
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	std::array<VkPipelineShaderStageCreateInfo, 4> shader_stages{};
	shader_stages[0] = load_shader("extended_dynamic_state2/baseline.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("extended_dynamic_state2/baseline.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	/* Create graphics pipeline for dynamic rendering */
	VkFormat color_rendering_format = render_context->get_format();

	/* Provide information for dynamic rendering */
	VkPipelineRenderingCreateInfoKHR pipeline_create{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
	pipeline_create.pNext                   = VK_NULL_HANDLE;
	pipeline_create.colorAttachmentCount    = 1;
	pipeline_create.pColorAttachmentFormats = &color_rendering_format;
	pipeline_create.depthAttachmentFormat   = depth_format;
	pipeline_create.stencilAttachmentFormat = depth_format;

	/* Skybox pipeline (background cube) */
	VkSpecializationInfo                    specialization_info;
	std::array<VkSpecializationMapEntry, 1> specialization_map_entries{};
	specialization_map_entries[0]        = vkb::initializers::specialization_map_entry(0, 0, sizeof(uint32_t));
	uint32_t shadertype                  = 0;
	specialization_info                  = vkb::initializers::specialization_info(1, specialization_map_entries.data(), sizeof(shadertype), &shadertype);
	shader_stages[0].pSpecializationInfo = &specialization_info;
	shader_stages[1].pSpecializationInfo = &specialization_info;

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
	graphics_create.pVertexInputState   = &vertex_input_state;
	graphics_create.pTessellationState  = VK_NULL_HANDLE;
	graphics_create.stageCount          = 2;
	graphics_create.pStages             = shader_stages.data();
	graphics_create.layout              = pipeline_layouts.baseline;

	

	graphics_create.pNext      = VK_NULL_HANDLE;
	graphics_create.renderPass = render_pass;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &graphics_create, VK_NULL_HANDLE, &pipeline.baseline));
...
```

And now, thanks to dynamic state 2, we can change parameters before each corresponding draw call. 

```C++
...
	vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.baseline, 0, 1, &descriptor_sets.baseline, 0, nullptr);
	vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.baseline);

	vkCmdSetPrimitiveTopologyEXT(draw_cmd_buffers[i], VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	vkCmdSetPrimitiveRestartEnableEXT(draw_cmd_buffers[i], VK_FALSE);
	draw_from_scene(draw_cmd_buffers[i], &scene_nodes, SCENE_BASELINE_OBJ_INDEX);

	vkCmdSetPrimitiveTopologyEXT(draw_cmd_buffers[i], VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
	vkCmdSetPrimitiveRestartEnableEXT(draw_cmd_buffers[i], VK_TRUE);

	draw_created_model(draw_cmd_buffers[i]);

	vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.tesselation, 0, 1, &descriptor_sets.tesselation, 0, nullptr);
	vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.tesselation);

	vkCmdSetPrimitiveTopologyEXT(draw_cmd_buffers[i], VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	vkCmdSetPatchControlPointsEXT(draw_cmd_buffers[i], gui_settings.patch_control_points);

	draw_from_scene(draw_cmd_buffers[i], &scene_nodes, SCENE_TESSELLATION_OBJ_INDEX);

	vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.background, 0, 1, &descriptor_sets.background, 0, nullptr);
	vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.background);

	draw_model(background_model, draw_cmd_buffers[i]);
...

```

The usage of depth bias dynamic state is implemented in `draw_from_scene` function which is described below.

```C++
void ExtendedDynamicState2::draw_from_scene(VkCommandBuffer command_buffer, std::vector<std::vector<SceneNode>> *scene_node, sceneObjType_t scene_index)
{
	auto &node               = scene_node->at(scene_index);
	int   scene_elements_cnt = scene_node->at(scene_index).size();

	for (int i = 0; i < scene_elements_cnt; i++)
	{
		const auto &vertex_buffer_pos    = node[i].sub_mesh->vertex_buffers.at("position");
		const auto &vertex_buffer_normal = node[i].sub_mesh->vertex_buffers.at("normal");
		auto &      index_buffer         = node[i].sub_mesh->index_buffer;

		if (scene_index == SCENE_BASELINE_OBJ_INDEX)
		{
			vkCmdSetDepthBiasEnableEXT(command_buffer, gui_settings.objects[i].depth_bias);
			vkCmdSetRasterizerDiscardEnableEXT(command_buffer, gui_settings.objects[i].rasterizer_discard);
		}

		// Pass data for the current node via push commands
		auto node_material            = dynamic_cast<const vkb::sg::PBRMaterial *>(node[i].sub_mesh->get_material());
		push_const_block.model_matrix = node[i].node->get_transform().get_world_matrix();
		if (i != gui_settings.selected_obj ||
		    gui_settings.selection_active == false)
		{
			push_const_block.color = node_material->base_color_factor;
		}
		else
		{
			vkb::sg::PBRMaterial temp_material{"Selected_Material"};
			selection_indicator(node_material, &temp_material);
			push_const_block.color = temp_material.base_color_factor;
		}
		vkCmdPushConstants(command_buffer, pipeline_layouts.baseline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_const_block), &push_const_block);

		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffer_pos.get(), offsets);
		vkCmdBindVertexBuffers(command_buffer, 1, 1, vertex_buffer_normal.get(), offsets);
		vkCmdBindIndexBuffer(command_buffer, index_buffer->get_handle(), 0, node[i].sub_mesh->index_type);

		vkCmdDrawIndexed(command_buffer, node[i].sub_mesh->vertex_indices, 1, 0, 0, 0);
	}
}
```


## Enabling the Extension
The extended dynamic state 2 api is provided in Vulkan 1.3 and the appropriate headers / SDK is required.

The device extension is provided by `VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME`. It also requires 
`VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME` instance extension to be enabled

```C++
add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
```

Additional features are provided by the VkPhysicalDeviceExtendedDynamicState2FeaturesEXT struct:

```C++
typedef struct VkPhysicalDeviceExtendedDynamicState2FeaturesEXT {
    VkStructureType    sType;
    void*              pNext;
    VkBool32           extendedDynamicState2;
    VkBool32           extendedDynamicState2LogicOp;
    VkBool32           extendedDynamicState2PatchControlPoints;
} VkPhysicalDeviceExtendedDynamicState2FeaturesEXT;
```

