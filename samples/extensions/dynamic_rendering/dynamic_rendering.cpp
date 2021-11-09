#include "dynamic_rendering.h"

DynamicRendering::DynamicRendering()
{
	title = "Dynamic Rendering";

	// Dynamic Rendering is a Vulkan 1.2 extension
	set_api_version(VK_API_VERSION_1_2);
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	add_device_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
}

DynamicRendering::~DynamicRendering()
{
    if (device) {
        textures = {};

        auto destroy_attachment = [](VkDevice device, Attachment &attachment) {
            vkDestroyImage(device, attachment.image, VK_NULL_HANDLE);
            vkDestroyImageView(device, attachment.image_view, VK_NULL_HANDLE);
            vkFreeMemory(device, attachment.device_memory, VK_NULL_HANDLE);
        };
        destroy_attachment(get_device().get_handle(), color_attachment);

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

bool DynamicRendering::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

#if VK_NO_PROTOTYPES
	VkInstance instance = get_device().get_gpu().get_instance().get_handle();
	assert(!!instance);
	vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR) vkGetInstanceProcAddr(instance, "vkCmdBeginRenderingKHR");
	vkCmdEndRenderingKHR   = (PFN_vkCmdEndRenderingKHR) vkGetInstanceProcAddr(instance, "vkCmdEndRenderingKHR");
	assert(!!vkCmdBeginRenderingKHR && !!vkCmdEndRenderingKHR);
#endif

	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.f, 0.f, -4.f});
	camera.set_rotation({0.f, 180.f, 0.f});
	camera.set_perspective(60.f, (float) width / (float) height, 256.f, 0.1f);

	load_assets();
	prepare_uniform_buffers();
	create_attachments();
	create_descriptor_pool();
	setup_descriptor_set_layout();
	create_descriptor_sets();
	create_pipeline();
	build_command_buffers();
	prepared = true;

	return true;
}

void DynamicRendering::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	auto &requested_dynamic_rendering            = gpu.request_extension_features<VkPhysicalDeviceDynamicRenderingFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR);
	requested_dynamic_rendering.dynamicRendering = VK_TRUE;

	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = true;
	}
}

void DynamicRendering::load_assets()
{
	// Models
	skybox = load_model("scenes/cube.gltf");
	object = load_model("scenes/geosphere.gltf");

	// Load HDR cube map
	textures.envmap = load_texture_cubemap("textures/uffizi_rgba16f_cube.ktx");
}

void DynamicRendering::prepare_uniform_buffers()
{
	ubo = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubo_vs), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void DynamicRendering::update_uniform_buffers()
{
	ubo_vs.projection       = camera.matrices.perspective;
	ubo_vs.modelview        = camera.matrices.view * glm::mat4(1.f);
	ubo_vs.skybox_modelview = camera.matrices.view;
	ubo->convert_and_update(ubo_vs);
}

void DynamicRendering::setup_descriptor_set_layout()
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

void DynamicRendering::create_descriptor_sets()
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
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
}

void DynamicRendering::create_attachments()
{
	color_attachment.format = VK_FORMAT_R8G8B8A8_UNORM;

	VkImageCreateInfo image_create = vkb::initializers::image_create_info();
	image_create.imageType         = VK_IMAGE_TYPE_2D;
	image_create.format            = color_attachment.format;
	image_create.extent            = VkExtent3D{width, height, 1};
	image_create.mipLevels         = 1;
	image_create.arrayLayers       = 1;
	image_create.samples           = VK_SAMPLE_COUNT_1_BIT;
	image_create.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image_create.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto device = get_device().get_handle();
	VK_CHECK(vkCreateImage(device, &image_create, VK_NULL_HANDLE, &color_attachment.image));

	VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device, color_attachment.image, &memory_requirements);
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK(vkAllocateMemory(device, &memory_allocate_info, VK_NULL_HANDLE, &color_attachment.device_memory));
	VK_CHECK(vkBindImageMemory(device, color_attachment.image, color_attachment.device_memory, 0));

	VkImageSubresourceRange subresource_range{};
	subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel   = 0;
	subresource_range.levelCount     = 1;
	subresource_range.baseArrayLayer = 0;
	subresource_range.layerCount     = 1;

	VkImageViewCreateInfo image_view_create_info = vkb::initializers::image_view_create_info();
	image_view_create_info.viewType              = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format                = color_attachment.format;
	image_view_create_info.subresourceRange      = subresource_range;
	image_view_create_info.image                 = color_attachment.image;
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &image_view_create_info, nullptr, &color_attachment.image_view));

	auto &cmd = get_device().request_command_buffer();
	cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkImageMemoryBarrier image_memory_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	image_memory_barrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barrier.newLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	image_memory_barrier.srcAccessMask    = 0;
	image_memory_barrier.dstAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	image_memory_barrier.image            = color_attachment.image;
	image_memory_barrier.subresourceRange = subresource_range;

	vkCmdPipelineBarrier(
		cmd.get_handle(),
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1,
		&image_memory_barrier);

	get_device().flush_command_buffer(cmd.get_handle(), get_device().get_suitable_graphics_queue().get_handle(), true);
}

void DynamicRendering::create_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
		vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
		vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)};
	uint32_t                   num_descriptor_sets = 4;
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
		vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void DynamicRendering::create_pipeline()
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

	std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_states = {
		vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
		vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	};

	VkPipelineColorBlendStateCreateInfo color_blend_state =
		vkb::initializers::pipeline_color_blend_state_create_info(
			1,
			&blend_attachment_state);
	color_blend_state.attachmentCount = 2;
	color_blend_state.pAttachments    = blend_attachment_states.data();

	// Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
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

	// Vertex bindings an attributes for model rendering
	// Binding description
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
		vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};

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

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
	shader_stages[0] = load_shader("hdr/gbuffer.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("hdr/gbuffer.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Create graphics pipeline for dynamic rendering
	VkFormat color_rendering_format = VK_FORMAT_R8G8B8A8_UNORM;

	// Provide information for dynamic rendering
	VkPipelineRenderingCreateInfoKHR pipeline_create{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
	pipeline_create.pNext                   = VK_NULL_HANDLE;
	pipeline_create.colorAttachmentCount    = 1;
	pipeline_create.pColorAttachmentFormats = &color_rendering_format;
	pipeline_create.depthAttachmentFormat   = depth_format;
	pipeline_create.stencilAttachmentFormat = depth_format;

	// Use the pNext to point to the rendering create struct
	VkGraphicsPipelineCreateInfo graphics_create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	graphics_create.pNext               = &pipeline_create;
	graphics_create.renderPass          = VK_NULL_HANDLE;
	graphics_create.pInputAssemblyState = &input_assembly_state;
	graphics_create.pRasterizationState = &rasterization_state;
	graphics_create.pColorBlendState    = &color_blend_state;
	graphics_create.pMultisampleState   = &multisample_state;
	graphics_create.pViewportState      = &viewport_state;
	graphics_create.pDepthStencilState  = &depth_stencil_state;
	graphics_create.pDynamicState       = &dynamic_state;
	graphics_create.pVertexInputState   = &vertex_input_state;
	graphics_create.stageCount          = shader_stages.size();
	graphics_create.pStages             = shader_stages.data();
	graphics_create.layout              = pipeline_layout;

	// Skybox pipeline (background cube)
	VkSpecializationInfo                    specialization_info;
	std::array<VkSpecializationMapEntry, 1> specialization_map_entries;
	specialization_map_entries[0]        = vkb::initializers::specialization_map_entry(0, 0, sizeof(uint32_t));
	uint32_t shadertype                  = 0;
	specialization_info                  = vkb::initializers::specialization_info(1, specialization_map_entries.data(), sizeof(shadertype), &shadertype);
	shader_stages[0].pSpecializationInfo = &specialization_info;
	shader_stages[1].pSpecializationInfo = &specialization_info;

	vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &graphics_create, VK_NULL_HANDLE, &skybox_pipeline);

	// Object rendering pipeline
	shadertype = 1;

	// Enable depth test and write
	depth_stencil_state.depthWriteEnable = VK_TRUE;
	depth_stencil_state.depthTestEnable  = VK_TRUE;
	// Flip cull mode
	rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &graphics_create, VK_NULL_HANDLE, &model_pipeline);
}

void DynamicRendering::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void DynamicRendering::build_command_buffers()
{
	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	for (size_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		auto command_begin = vkb::initializers::command_buffer_begin_info();
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_begin));

		VkRenderingAttachmentInfoKHR color_attachment_info = vkb::initializers::rendering_attachment_info();
		color_attachment_info.imageView                    = color_attachment.image_view;
		color_attachment_info.imageLayout                  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment_info.resolveMode                  = VK_RESOLVE_MODE_NONE;
		color_attachment_info.loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment_info.storeOp                      = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment_info.clearValue                   = clear_values[0];

		VkRenderingAttachmentInfoKHR depth_attachment_info = vkb::initializers::rendering_attachment_info();
		depth_attachment_info.imageView                    = depth_stencil.view;
		depth_attachment_info.imageLayout                  = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		depth_attachment_info.resolveMode                  = VK_RESOLVE_MODE_NONE;
		depth_attachment_info.loadOp                       = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment_info.storeOp                      = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment_info.clearValue                   = clear_values[1];

		auto render_area               = VkRect2D{VkOffset2D{}, VkExtent2D{width, height}};
		auto render_info               = vkb::initializers::rendering_info(render_area, 1, &color_attachment_info);
		render_info.layerCount         = 1;
		render_info.pDepthAttachment   = &depth_attachment_info;
		render_info.pStencilAttachment = &depth_attachment_info;

		vkCmdBeginRenderingKHR(draw_cmd_buffers[i], &render_info);

		{
			VkViewport viewport = vkb::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

			// One descriptor set is used, and the draw type is toggled by a specialization constant
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);

			// skybox
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_pipeline);
			draw_model(skybox, draw_cmd_buffers[i]);

			// object
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, model_pipeline);
			draw_model(object, draw_cmd_buffers[i]);
		}

		vkCmdEndRenderingKHR(draw_cmd_buffers[i]);
	}
}

void DynamicRendering::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
	if (camera.updated)
		update_uniform_buffers();
}

void DynamicRendering::view_changed()
{
}

void DynamicRendering::on_update_ui_overlay(vkb::Drawer &drawer)
{
}

std::unique_ptr<vkb::VulkanSample> create_dynamic_rendering()
{
	return std::make_unique<DynamicRendering>();
}
