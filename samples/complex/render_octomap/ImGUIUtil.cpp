//
// Created by Steven Winston on 4/14/24.
//

#include "ImGUIUtil.h"
#include "GLFW/glfw3.h"
#include "api_vulkan_sample.h"
#include <algorithm>
#include <cstdarg>

ImGuiKey Glfw_KeyToImGuiKey(int keycode)
{
    switch (keycode)
    {
        case GLFW_KEY_TAB: return ImGuiKey_Tab;
        case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
        case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
        case GLFW_KEY_UP: return ImGuiKey_UpArrow;
        case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
        case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
        case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
        case GLFW_KEY_HOME: return ImGuiKey_Home;
        case GLFW_KEY_END: return ImGuiKey_End;
        case GLFW_KEY_INSERT: return ImGuiKey_Insert;
        case GLFW_KEY_DELETE: return ImGuiKey_Delete;
        case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
        case GLFW_KEY_SPACE: return ImGuiKey_Space;
        case GLFW_KEY_ENTER: return ImGuiKey_Enter;
        case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
        case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
        case GLFW_KEY_COMMA: return ImGuiKey_Comma;
        case GLFW_KEY_MINUS: return ImGuiKey_Minus;
        case GLFW_KEY_PERIOD: return ImGuiKey_Period;
        case GLFW_KEY_SLASH: return ImGuiKey_Slash;
        case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
        case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
        case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
        case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
        case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
        case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
        case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
        case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
        case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
        case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
        case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
        case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
        case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
        case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
        case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
        case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
        case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
        case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
        case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
        case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
        case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
        case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
        case GLFW_KEY_MENU: return ImGuiKey_Menu;
        case GLFW_KEY_0: return ImGuiKey_0;
        case GLFW_KEY_1: return ImGuiKey_1;
        case GLFW_KEY_2: return ImGuiKey_2;
        case GLFW_KEY_3: return ImGuiKey_3;
        case GLFW_KEY_4: return ImGuiKey_4;
        case GLFW_KEY_5: return ImGuiKey_5;
        case GLFW_KEY_6: return ImGuiKey_6;
        case GLFW_KEY_7: return ImGuiKey_7;
        case GLFW_KEY_8: return ImGuiKey_8;
        case GLFW_KEY_9: return ImGuiKey_9;
        case GLFW_KEY_A: return ImGuiKey_A;
        case GLFW_KEY_B: return ImGuiKey_B;
        case GLFW_KEY_C: return ImGuiKey_C;
        case GLFW_KEY_D: return ImGuiKey_D;
        case GLFW_KEY_E: return ImGuiKey_E;
        case GLFW_KEY_F: return ImGuiKey_F;
        case GLFW_KEY_G: return ImGuiKey_G;
        case GLFW_KEY_H: return ImGuiKey_H;
        case GLFW_KEY_I: return ImGuiKey_I;
        case GLFW_KEY_J: return ImGuiKey_J;
        case GLFW_KEY_K: return ImGuiKey_K;
        case GLFW_KEY_L: return ImGuiKey_L;
        case GLFW_KEY_M: return ImGuiKey_M;
        case GLFW_KEY_N: return ImGuiKey_N;
        case GLFW_KEY_O: return ImGuiKey_O;
        case GLFW_KEY_P: return ImGuiKey_P;
        case GLFW_KEY_Q: return ImGuiKey_Q;
        case GLFW_KEY_R: return ImGuiKey_R;
        case GLFW_KEY_S: return ImGuiKey_S;
        case GLFW_KEY_T: return ImGuiKey_T;
        case GLFW_KEY_U: return ImGuiKey_U;
        case GLFW_KEY_V: return ImGuiKey_V;
        case GLFW_KEY_W: return ImGuiKey_W;
        case GLFW_KEY_X: return ImGuiKey_X;
        case GLFW_KEY_Y: return ImGuiKey_Y;
        case GLFW_KEY_Z: return ImGuiKey_Z;
        case GLFW_KEY_F1: return ImGuiKey_F1;
        case GLFW_KEY_F2: return ImGuiKey_F2;
        case GLFW_KEY_F3: return ImGuiKey_F3;
        case GLFW_KEY_F4: return ImGuiKey_F4;
        case GLFW_KEY_F5: return ImGuiKey_F5;
        case GLFW_KEY_F6: return ImGuiKey_F6;
        case GLFW_KEY_F7: return ImGuiKey_F7;
        case GLFW_KEY_F8: return ImGuiKey_F8;
        case GLFW_KEY_F9: return ImGuiKey_F9;
        case GLFW_KEY_F10: return ImGuiKey_F10;
        case GLFW_KEY_F11: return ImGuiKey_F11;
        case GLFW_KEY_F12: return ImGuiKey_F12;
        case GLFW_KEY_F13: return ImGuiKey_F13;
        case GLFW_KEY_F14: return ImGuiKey_F14;
        case GLFW_KEY_F15: return ImGuiKey_F15;
        case GLFW_KEY_F16: return ImGuiKey_F16;
        case GLFW_KEY_F17: return ImGuiKey_F17;
        case GLFW_KEY_F18: return ImGuiKey_F18;
        case GLFW_KEY_F19: return ImGuiKey_F19;
        case GLFW_KEY_F20: return ImGuiKey_F20;
        case GLFW_KEY_F21: return ImGuiKey_F21;
        case GLFW_KEY_F22: return ImGuiKey_F22;
        case GLFW_KEY_F23: return ImGuiKey_F23;
        case GLFW_KEY_F24: return ImGuiKey_F24;
        default: return ImGuiKey_None;
    }
}


ImGUIUtil::ImGUIUtil(ApiVulkanSample *_base) : base(_base)
{
	// This conflicts with the hpp_gui context.  Disable for now.
    // ImGuiContext* context = ImGui::CreateContext();
    // ImGui::SetCurrentContext(context);
	auto &device = base->get_render_context().get_device();
	vertexBuffer =
	vkb::core::BufferBuilderC(1)
		.with_usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
		.with_vma_usage(VMA_MEMORY_USAGE_GPU_TO_CPU)
		.with_debug_name("GUI vertex buffer")
		.build_unique(device);

	indexBuffer =
		vkb::core::BufferBuilderC(1)
			.with_usage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
			.with_vma_usage(VMA_MEMORY_USAGE_GPU_TO_CPU)
			.with_debug_name("GUI index buffer")
			.build_unique(device);
}

ImGUIUtil::~ImGUIUtil()
{
    // ImGui::DestroyContext(); // this would double free due to the default one in hpp_gui...
    // Release all Vulkan resources required for rendering imGui
    vkFreeMemory(base->get_render_context().get_device().get_handle(), fontMemory, nullptr);
    vkDestroySampler(base->get_render_context().get_device().get_handle(), sampler, nullptr);
    vkDestroyPipelineCache(base->get_render_context().get_device().get_handle(), pipelineCache, nullptr);
    vkDestroyPipeline(base->get_render_context().get_device().get_handle(), pipeline, nullptr);
    vkDestroyPipelineLayout(base->get_render_context().get_device().get_handle(), pipelineLayout, nullptr);
    vkDestroyDescriptorPool(base->get_render_context().get_device().get_handle(), descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(base->get_render_context().get_device().get_handle(), descriptorSetLayout, nullptr);
}

// Initialize styles, keys, etc.
void ImGUIUtil::init(float width, float height)
{
    // Color scheme
    vulkanStyle = ImGui::GetStyle();
    vulkanStyle.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    vulkanStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    vulkanStyle.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    vulkanStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

    setStyle(0);
    // Dimensions
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(width, height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}

void ImGUIUtil::setStyle(uint32_t index)
{
    switch (index)
    {
        case 0:
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style = vulkanStyle;
            break;
        }
        case 1:
            ImGui::StyleColorsClassic();
            break;
        case 2:
            ImGui::StyleColorsDark();
            break;
        case 3:
            ImGui::StyleColorsLight();
            break;
    }
}

// Initialize all Vulkan resources used by the ui
void ImGUIUtil::initResources(VkRenderPass renderPass, VkQueue copyQueue)
{
    ImGuiIO& io = ImGui::GetIO();

    // Create font texture
    unsigned char* fontData;
    int texWidth, texHeight;
    
    io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
    VkDeviceSize uploadSize = texWidth*texHeight * 4 * sizeof(char);
    
    //SRS - Get Vulkan device driver information if available, use later for display
    if (base->get_render_context().get_device().get_gpu().is_extension_supported(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME))
    {
        VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &driverProperties;
        driverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
        vkGetPhysicalDeviceProperties2(base->get_render_context().get_device().get_gpu().get_handle(), &deviceProperties2);
    }

    // Create target image for copy
    VkExtent3D font_extent{vkb::to_u32(texWidth), vkb::to_u32(texHeight), 1u};

	font_image = std::make_unique<vkb::core::Image>(base->get_render_context().get_device(), font_extent, VK_FORMAT_R8G8B8A8_UNORM,
	                                           VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
	                                           VMA_MEMORY_USAGE_GPU_ONLY);
	font_image->set_debug_name("GUI font image");

	font_image_view = std::make_unique<vkb::core::ImageView>(*font_image, VK_IMAGE_VIEW_TYPE_2D);
	font_image_view->set_debug_name("View on GUI font image");

	// Upload font data into the vulkan image memory
	{
		vkb::core::BufferC stage_buffer = vkb::core::BufferC::create_staging_buffer(base->get_render_context().get_device(), uploadSize, fontData);

    	auto &command_buffer = base->get_render_context().get_device().request_command_buffer();

    	vkb::FencePool fence_pool{base->get_render_context().get_device()};

    	// Begin recording
    	command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0);

	    {
			// Prepare for transfer
			vkb::ImageMemoryBarrier memory_barrier{};
			memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
			memory_barrier.new_layout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			memory_barrier.src_access_mask = 0;
			memory_barrier.dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_HOST_BIT;
			memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;

			command_buffer.image_memory_barrier(*font_image_view, memory_barrier);
	    }

    	// Copy
    	VkBufferImageCopy buffer_copy_region{};
    	buffer_copy_region.imageSubresource.layerCount = font_image_view->get_subresource_range().layerCount;
    	buffer_copy_region.imageSubresource.aspectMask = font_image_view->get_subresource_range().aspectMask;
    	buffer_copy_region.imageExtent                 = font_image->get_extent();

    	command_buffer.copy_buffer_to_image(stage_buffer, *font_image, {buffer_copy_region});

	    {
			// Prepare for fragmen shader
			vkb::ImageMemoryBarrier memory_barrier{};
			memory_barrier.old_layout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			memory_barrier.new_layout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			memory_barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memory_barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
			memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
			memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			command_buffer.image_memory_barrier(*font_image_view, memory_barrier);
	    }
    	// End recording
    	command_buffer.end();
    	auto &queue = base->get_render_context().get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

    	queue.submit(command_buffer, base->get_render_context().get_device().request_fence());

    	// Wait for the command buffer to finish its work before destroying the staging buffer
    	VK_CHECK(base->get_render_context().get_device().get_fence_pool().wait());
    	base->get_render_context().get_device().get_fence_pool().reset();
    	base->get_render_context().get_device().get_command_pool().reset_pool();
	}
    // Font texture Sampler
    VkSamplerCreateInfo samplerInfo = vkb::initializers::sampler_create_info();
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VK_CHECK(vkCreateSampler(base->get_render_context().get_device().get_handle(), &samplerInfo, nullptr, &sampler));

    // Descriptor pool
    std::vector poolSizes = {
            vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6)
    };
    VkDescriptorPoolCreateInfo descriptorPoolInfo = vkb::initializers::descriptor_pool_create_info(poolSizes, 7);
    VK_CHECK(vkCreateDescriptorPool(base->get_render_context().get_device().get_handle(), &descriptorPoolInfo, nullptr, &descriptorPool));

    // Descriptor set layout
    std::vector setLayoutBindings = {
            vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                     VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };
    VkDescriptorSetLayoutCreateInfo descriptorLayout = vkb::initializers::descriptor_set_layout_create_info(setLayoutBindings);
    VK_CHECK(vkCreateDescriptorSetLayout(base->get_render_context().get_device().get_handle(), &descriptorLayout, nullptr, &descriptorSetLayout));

    // Descriptor set
    VkDescriptorSetAllocateInfo allocInfo = vkb::initializers::descriptor_set_allocate_info(descriptorPool,
                                                                                    &descriptorSetLayout, 1);
    VK_CHECK(vkAllocateDescriptorSets(base->get_render_context().get_device().get_handle(), &allocInfo, &descriptorSet));
    VkDescriptorImageInfo fontDescriptor = vkb::initializers::descriptor_image_info(
            sampler,
            font_image_view->get_handle(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
            vkb::initializers::write_descriptor_set(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                             &fontDescriptor)
    };

    std::vector<VkWriteDescriptorSet> liveMapDescriptorSets = MapsView.LoadAssets(base, allocInfo, copyQueue);
    if (liveMapDescriptorSets.size() > 0)
    {
	    writeDescriptorSets.insert(liveMapDescriptorSets.end(), liveMapDescriptorSets.begin(), liveMapDescriptorSets.end());
    }

  vkUpdateDescriptorSets(base->get_render_context().get_device().get_handle(), static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(),
                         0, nullptr);

    // Pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK(vkCreatePipelineCache(base->get_render_context().get_device().get_handle(), &pipelineCacheCreateInfo, nullptr, &pipelineCache));

    // Pipeline layout
    // Push constants for UI rendering parameters
    VkPushConstantRange pushConstantRange = vkb::initializers::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, sizeof
    (PushConstBlock), 0);
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vkb::initializers::pipeline_layout_create_info
            (&descriptorSetLayout, 1);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    VK_CHECK(vkCreatePipelineLayout(base->get_render_context().get_device().get_handle(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

    // Setup graphics pipeline for UI rendering
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
            vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

    VkPipelineRasterizationStateCreateInfo rasterizationState =
            vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
                                                               VK_FRONT_FACE_COUNTER_CLOCKWISE);

    // Enable blending
    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.blendEnable = VK_TRUE;
    blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    
    VkPipelineColorBlendStateCreateInfo colorBlendState =
            vkb::initializers::pipeline_color_blend_state_create_info(1, &blendAttachmentState);

    VkPipelineDepthStencilStateCreateInfo depthStencilState =
            vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);

    VkPipelineViewportStateCreateInfo viewportState =
            vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

    VkPipelineMultisampleStateCreateInfo multisampleState =
            vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

    std::vector<VkDynamicState> dynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState =
            vkb::initializers::pipeline_dynamic_state_create_info(dynamicStateEnables);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
    	base->load_shader("render_octomap", "imgui.vert", VK_SHADER_STAGE_VERTEX_BIT),
    	base->load_shader("render_octomap", "imgui.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = vkb::initializers::pipeline_create_info(pipelineLayout, renderPass);

    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();

    // Vertex bindings an attributes based on ImGui vertex definition
    std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
            vkb::initializers::vertex_input_binding_description(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
    };
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
            vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),
            // Location 0: Position
            vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),
            // Location 1: UV
            vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)),
            // Location 0: Color
    };
    VkPipelineVertexInputStateCreateInfo vertexInputState = vkb::initializers::pipeline_vertex_input_state_create_info();
    vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
    vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

    pipelineCreateInfo.pVertexInputState = &vertexInputState;

    VK_CHECK(vkCreateGraphicsPipelines(base->get_render_context().get_device().get_handle(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

// Starts a new imGui frame and sets up windows and ui elements
bool ImGUIUtil::newFrame(bool updateFrameGraph) {
    ImGui::NewFrame();
    
    {
        ImGuiStyle &style = ImGui::GetStyle();
        style.ChildRounding = 0.0f;
        
        
        style.WindowPadding = ImVec2(15, 15); // Set window padding
        style.FramePadding = ImVec2(5, 5); // Set padding within the widgets
        style.ItemInnerSpacing = ImVec2(10, 10); // Set spacing between elements inside widgets
        
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::SetNextWindowPos(ImVec2(0,0));
        
        // NEW WINDOW //
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Remove padding
        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0,0,0,0 });
        ImGui::Begin(" ", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar();
        ImGui:: PopStyleColor();
        
        
        MapsView.DrawUI();
        // END WINDOW //
        ImGui::End();
    }
    ImGui::EndFrame();
    
    // Render to generate draw buffers
    ImGui::Render();
    if(needsUpdateBuffers) {
        needsUpdateBuffers = false;
        return true;
    }
    return false;

}

// Update vertex and index buffer containing the imGui elements when required
void ImGUIUtil::updateBuffers()
{
    ImDrawData* imDrawData = ImGui::GetDrawData();

    if (!imDrawData)
    	return;

    // Note: Alignment is done inside buffer creation
    VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
    VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

    if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
        return;
    }

    // Update buffers only if vertex or index count has been changed compared to current buffer size
	if ((vertexBuffer->get_handle() == VK_NULL_HANDLE) || (vertexBufferSize != imDrawData->TotalVtxCount))
    {
    	vertexBuffer.reset();
    	vertexBuffer = std::make_unique<vkb::core::BufferC>(base->get_render_context().get_device(), vertexBufferSize,
															 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
															 VMA_MEMORY_USAGE_GPU_TO_CPU);
		vertexCount = imDrawData->TotalVtxCount;
    	vertexBuffer->set_debug_name("GUI Util vertex buffer");
    }

	if ((indexBuffer->get_handle() == VK_NULL_HANDLE) || (indexCount != imDrawData->TotalIdxCount))
	{
		indexCount = imDrawData->TotalIdxCount;

		indexBuffer.reset();
		indexBuffer = std::make_unique<vkb::core::BufferC>(base->get_render_context().get_device(), indexBufferSize,
															VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
															VMA_MEMORY_USAGE_GPU_TO_CPU);
		indexBuffer->set_debug_name("GUI index buffer");
	}

    // Upload data
    ImDrawVert* vtxDst = (ImDrawVert*)vertexBuffer->map();
    ImDrawIdx* idxDst = (ImDrawIdx*)indexBuffer->map();

    for (int n = 0; n < imDrawData->CmdListsCount; n++) {
        const ImDrawList* cmd_list = imDrawData->CmdLists[n];
        memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += cmd_list->VtxBuffer.Size;
        idxDst += cmd_list->IdxBuffer.Size;
    }

    // Flush to make writes visible to GPU
    vertexBuffer->flush();
    indexBuffer->flush();
}

void ImGUIUtil::drawFrame(VkCommandBuffer commandBuffer)
{
    ImGuiIO& io = ImGui::GetIO();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkViewport viewport = vkb::initializers::viewport(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // UI scale and translate via push constants
    pushConstBlock.scale = glm::vec2 (2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
    pushConstBlock.translate = glm::vec2 (-1.0f);
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

    // Render commands
    ImDrawData* imDrawData = ImGui::GetDrawData();
    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;

    if (imDrawData->CmdListsCount > 0) {

        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->get_handle(), offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->get_handle(), 0, VK_INDEX_TYPE_UINT16);

        for (auto i = 0; i < imDrawData->CmdListsCount; i++)
        {
            const ImDrawList* cmd_list = imDrawData->CmdLists[i];
            for (auto j = 0; j < cmd_list->CmdBuffer.Size; j++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
                VkRect2D scissorRect;
                scissorRect.offset.x = glm::max(pcmd->ClipRect.x, 0.0f);
                scissorRect.offset.y = glm::max(pcmd->ClipRect.y, 0.0f);
                scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
                
                if(((void*)pcmd->TextureId) != nullptr)
                {
	                VkDescriptorSet desc_set[1] = { (VkDescriptorSet)pcmd->TextureId };
                	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, desc_set, 0, nullptr);
                } else {
	                //bind the font
                	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
                }
                vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                indexOffset += pcmd->ElemCount;
            }
            vertexOffset += cmd_list->VtxBuffer.Size;
        }
    }
}

void ImGUIUtil::TextColorAlign(int align, const ImVec4& col, const char* text, ...) {
    va_list vaList;
    va_start(vaList, text);
    
    float font_width = ImGui::CalcTextSize(text).x;

    switch(align) {
        case 1:
            ImGui::SameLine(
                ImGui::GetContentRegionAvail().x * 0.5f - font_width * 0.5f
            );
            break;
        case 2:
            ImGui::SameLine(
                ImGui::GetContentRegionAvail().x - font_width
            );
        break;
        case 0:
        default:
            break;
    }

    ImGui::TextColoredV(col, text, vaList);

    va_end(vaList);
}

void ImGUIUtil::handleKey(int key, int, int action, int) {
#if !(defined TARGET_OS_IPHONE) && !(defined __ANDROID__)
  ImGuiIO &io = ImGui::GetIO();
  if (action == GLFW_PRESS)
    io.AddKeyEvent(Glfw_KeyToImGuiKey(key), true);
  if (action == GLFW_RELEASE)
    io.AddKeyEvent(Glfw_KeyToImGuiKey(key), false);

    // Modifiers are not reliable across systems
    io.KeyCtrl = ImGui::IsKeyPressed(Glfw_KeyToImGuiKey(GLFW_KEY_LEFT_CONTROL)) || ImGui::IsKeyPressed(Glfw_KeyToImGuiKey(GLFW_KEY_RIGHT_CONTROL));
    io.KeyShift = ImGui::IsKeyPressed(Glfw_KeyToImGuiKey(GLFW_KEY_LEFT_SHIFT)) || ImGui::IsKeyPressed(Glfw_KeyToImGuiKey(GLFW_KEY_RIGHT_SHIFT));
    io.KeyAlt = ImGui::IsKeyPressed(Glfw_KeyToImGuiKey(GLFW_KEY_LEFT_ALT)) || ImGui::IsKeyPressed(Glfw_KeyToImGuiKey(GLFW_KEY_RIGHT_ALT));
    io.KeySuper = ImGui::IsKeyPressed(Glfw_KeyToImGuiKey(GLFW_KEY_LEFT_SUPER)) || ImGui::IsKeyPressed(Glfw_KeyToImGuiKey(GLFW_KEY_RIGHT_SUPER));
#endif
}

bool ImGUIUtil::GetWantKeyCapture() {
  ImGuiIO &io = ImGui::GetIO();
  return io.WantCaptureKeyboard;
}

void ImGUIUtil::charPressed(uint32_t key) {
  ImGuiIO& io = ImGui::GetIO();
  io.AddInputCharacter(key);
}
