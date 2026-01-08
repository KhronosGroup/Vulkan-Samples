/* Copyright (c) 2024-2025, Holochip Inc.
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

#include "ImGUIUtil.h"
#include "api_vulkan_sample.h"
#include "platform/input_events.h"
#include <cstdarg>

// Map framework keycodes to ImGuiKey (platform-agnostic)
static ImGuiKey KeyCodeToImGuiKey(vkb::KeyCode code)
{
	using vkb::KeyCode;
	switch (code)
	{
		case KeyCode::Tab:
			return ImGuiKey_Tab;
		case KeyCode::Left:
			return ImGuiKey_LeftArrow;
		case KeyCode::Right:
			return ImGuiKey_RightArrow;
		case KeyCode::Up:
			return ImGuiKey_UpArrow;
		case KeyCode::Down:
			return ImGuiKey_DownArrow;
		case KeyCode::PageUp:
			return ImGuiKey_PageUp;
		case KeyCode::PageDown:
			return ImGuiKey_PageDown;
		case KeyCode::Home:
			return ImGuiKey_Home;
		case KeyCode::End:
			return ImGuiKey_End;
		case KeyCode::Insert:
			return ImGuiKey_Insert;
		case KeyCode::DelKey:
			return ImGuiKey_Delete;
		case KeyCode::Backspace:
			return ImGuiKey_Backspace;
		case KeyCode::Space:
			return ImGuiKey_Space;
		case KeyCode::Enter:
			return ImGuiKey_Enter;
		case KeyCode::Escape:
			return ImGuiKey_Escape;
		case KeyCode::Apostrophe:
			return ImGuiKey_Apostrophe;
		case KeyCode::Comma:
			return ImGuiKey_Comma;
		case KeyCode::Minus:
			return ImGuiKey_Minus;
		case KeyCode::Period:
			return ImGuiKey_Period;
		case KeyCode::Slash:
			return ImGuiKey_Slash;
		case KeyCode::Semicolon:
			return ImGuiKey_Semicolon;
		case KeyCode::Equal:
			return ImGuiKey_Equal;
		case KeyCode::LeftBracket:
			return ImGuiKey_LeftBracket;
		case KeyCode::Backslash:
			return ImGuiKey_Backslash;
		case KeyCode::RightBracket:
			return ImGuiKey_RightBracket;
		case KeyCode::GraveAccent:
			return ImGuiKey_GraveAccent;
		case KeyCode::CapsLock:
			return ImGuiKey_CapsLock;
		case KeyCode::ScrollLock:
			return ImGuiKey_ScrollLock;
		case KeyCode::NumLock:
			return ImGuiKey_NumLock;
		case KeyCode::PrintScreen:
			return ImGuiKey_PrintScreen;
		case KeyCode::Pause:
			return ImGuiKey_Pause;
		case KeyCode::KP_0:
			return ImGuiKey_Keypad0;
		case KeyCode::KP_1:
			return ImGuiKey_Keypad1;
		case KeyCode::KP_2:
			return ImGuiKey_Keypad2;
		case KeyCode::KP_3:
			return ImGuiKey_Keypad3;
		case KeyCode::KP_4:
			return ImGuiKey_Keypad4;
		case KeyCode::KP_5:
			return ImGuiKey_Keypad5;
		case KeyCode::KP_6:
			return ImGuiKey_Keypad6;
		case KeyCode::KP_7:
			return ImGuiKey_Keypad7;
		case KeyCode::KP_8:
			return ImGuiKey_Keypad8;
		case KeyCode::KP_9:
			return ImGuiKey_Keypad9;
		case KeyCode::KP_Decimal:
			return ImGuiKey_KeypadDecimal;
		case KeyCode::KP_Divide:
			return ImGuiKey_KeypadDivide;
		case KeyCode::KP_Multiply:
			return ImGuiKey_KeypadMultiply;
		case KeyCode::KP_Subtract:
			return ImGuiKey_KeypadSubtract;
		case KeyCode::KP_Add:
			return ImGuiKey_KeypadAdd;
		case KeyCode::KP_Enter:
			return ImGuiKey_KeypadEnter;
		case KeyCode::KP_Equal:
			return ImGuiKey_KeypadEqual;
		case KeyCode::LeftShift:
			return ImGuiKey_LeftShift;
		case KeyCode::LeftControl:
			return ImGuiKey_LeftCtrl;
		case KeyCode::LeftAlt:
			return ImGuiKey_LeftAlt;
		case KeyCode::RightShift:
			return ImGuiKey_RightShift;
		case KeyCode::RightControl:
			return ImGuiKey_RightCtrl;
		case KeyCode::RightAlt:
			return ImGuiKey_RightAlt;
		case KeyCode::F1:
			return ImGuiKey_F1;
		case KeyCode::F2:
			return ImGuiKey_F2;
		case KeyCode::F3:
			return ImGuiKey_F3;
		case KeyCode::F4:
			return ImGuiKey_F4;
		case KeyCode::F5:
			return ImGuiKey_F5;
		case KeyCode::F6:
			return ImGuiKey_F6;
		case KeyCode::F7:
			return ImGuiKey_F7;
		case KeyCode::F8:
			return ImGuiKey_F8;
		case KeyCode::F9:
			return ImGuiKey_F9;
		case KeyCode::F10:
			return ImGuiKey_F10;
		case KeyCode::F11:
			return ImGuiKey_F11;
		case KeyCode::F12:
			return ImGuiKey_F12;
		case KeyCode::_0:
			return ImGuiKey_0;
		case KeyCode::_1:
			return ImGuiKey_1;
		case KeyCode::_2:
			return ImGuiKey_2;
		case KeyCode::_3:
			return ImGuiKey_3;
		case KeyCode::_4:
			return ImGuiKey_4;
		case KeyCode::_5:
			return ImGuiKey_5;
		case KeyCode::_6:
			return ImGuiKey_6;
		case KeyCode::_7:
			return ImGuiKey_7;
		case KeyCode::_8:
			return ImGuiKey_8;
		case KeyCode::_9:
			return ImGuiKey_9;
		case KeyCode::A:
			return ImGuiKey_A;
		case KeyCode::B:
			return ImGuiKey_B;
		case KeyCode::C:
			return ImGuiKey_C;
		case KeyCode::D:
			return ImGuiKey_D;
		case KeyCode::E:
			return ImGuiKey_E;
		case KeyCode::F:
			return ImGuiKey_F;
		case KeyCode::G:
			return ImGuiKey_G;
		case KeyCode::H:
			return ImGuiKey_H;
		case KeyCode::I:
			return ImGuiKey_I;
		case KeyCode::J:
			return ImGuiKey_J;
		case KeyCode::K:
			return ImGuiKey_K;
		case KeyCode::L:
			return ImGuiKey_L;
		case KeyCode::M:
			return ImGuiKey_M;
		case KeyCode::N:
			return ImGuiKey_N;
		case KeyCode::O:
			return ImGuiKey_O;
		case KeyCode::P:
			return ImGuiKey_P;
		case KeyCode::Q:
			return ImGuiKey_Q;
		case KeyCode::R:
			return ImGuiKey_R;
		case KeyCode::S:
			return ImGuiKey_S;
		case KeyCode::T:
			return ImGuiKey_T;
		case KeyCode::U:
			return ImGuiKey_U;
		case KeyCode::V:
			return ImGuiKey_V;
		case KeyCode::W:
			return ImGuiKey_W;
		case KeyCode::X:
			return ImGuiKey_X;
		case KeyCode::Y:
			return ImGuiKey_Y;
		case KeyCode::Z:
			return ImGuiKey_Z;
		default:
			break;
	}
	return ImGuiKey_None;
}

ImGUIUtil::ImGUIUtil(ApiVulkanSample *_base) :
    base(_base)
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
	vulkanStyle                                = ImGui::GetStyle();
	vulkanStyle.Colors[ImGuiCol_TitleBg]       = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	vulkanStyle.Colors[ImGuiCol_MenuBarBg]     = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	vulkanStyle.Colors[ImGuiCol_Header]        = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	vulkanStyle.Colors[ImGuiCol_CheckMark]     = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

	setStyle(0);
	// Dimensions
	ImGuiIO &io                = ImGui::GetIO();
	io.DisplaySize             = ImVec2(width, height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}

void ImGUIUtil::setStyle(uint32_t index)
{
	switch (index)
	{
		case 0:
		{
			ImGuiStyle &style = ImGui::GetStyle();
			style             = vulkanStyle;
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
	ImGuiIO &io = ImGui::GetIO();

	// Create font texture
	unsigned char *fontData;
	int            texWidth, texHeight;

	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
	VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

	// SRS - Get Vulkan device driver information if available, use later for display
	if (base->get_render_context().get_device().get_gpu().is_extension_supported(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME))
	{
		VkPhysicalDeviceProperties2 deviceProperties2 = {};
		deviceProperties2.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProperties2.pNext                       = &driverProperties;
		driverProperties.sType                        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
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

		auto command_buffer = base->get_render_context().get_device().get_command_pool().request_command_buffer();

		vkb::FencePool fence_pool{base->get_render_context().get_device()};

		// Begin recording
		command_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0);

		{
			// Prepare for transfer
			vkb::ImageMemoryBarrier memory_barrier{};
			memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
			memory_barrier.new_layout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			memory_barrier.src_access_mask = 0;
			memory_barrier.dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_HOST_BIT;
			memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;

			command_buffer->image_memory_barrier(*font_image_view, memory_barrier);
		}

		// Copy
		VkBufferImageCopy buffer_copy_region{};
		buffer_copy_region.imageSubresource.layerCount = font_image_view->get_subresource_range().layerCount;
		buffer_copy_region.imageSubresource.aspectMask = font_image_view->get_subresource_range().aspectMask;
		buffer_copy_region.imageExtent                 = font_image->get_extent();

		command_buffer->copy_buffer_to_image(stage_buffer, *font_image, {buffer_copy_region});

		{
			// Prepare for fragmen shader
			vkb::ImageMemoryBarrier memory_barrier{};
			memory_barrier.old_layout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			memory_barrier.new_layout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			memory_barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memory_barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
			memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
			memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			command_buffer->image_memory_barrier(*font_image_view, memory_barrier);
		}
		// End recording
		command_buffer->end();
		auto &queue = base->get_render_context().get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

		queue.submit(*command_buffer, base->get_render_context().get_device().get_fence_pool().request_fence());

		// Wait for the command buffer to finish its work before destroying the staging buffer
		VK_CHECK(base->get_render_context().get_device().get_fence_pool().wait());
		base->get_render_context().get_device().get_fence_pool().reset();
		base->get_render_context().get_device().get_command_pool().reset_pool();
	}
	// Font texture Sampler
	VkSamplerCreateInfo samplerInfo = vkb::initializers::sampler_create_info();
	samplerInfo.magFilter           = VK_FILTER_LINEAR;
	samplerInfo.minFilter           = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK(vkCreateSampler(base->get_render_context().get_device().get_handle(), &samplerInfo, nullptr, &sampler));

	// Descriptor pool
	std::vector poolSizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6)};
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
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
	    vkb::initializers::write_descriptor_set(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
	                                            &fontDescriptor)};

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
	pipelineCacheCreateInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VK_CHECK(vkCreatePipelineCache(base->get_render_context().get_device().get_handle(), &pipelineCacheCreateInfo, nullptr, &pipelineCache));

	// Pipeline layout
	// Push constants for UI rendering parameters
	VkPushConstantRange        pushConstantRange        = vkb::initializers::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0);
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vkb::initializers::pipeline_layout_create_info(&descriptorSetLayout, 1);
	pipelineLayoutCreateInfo.pushConstantRangeCount     = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges        = &pushConstantRange;
	VK_CHECK(vkCreatePipelineLayout(base->get_render_context().get_device().get_handle(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

	// Setup graphics pipeline for UI rendering
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
	    vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterizationState =
	    vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
	                                                                VK_FRONT_FACE_COUNTER_CLOCKWISE);

	// Enable blending
	VkPipelineColorBlendAttachmentState blendAttachmentState{};
	blendAttachmentState.blendEnable         = VK_TRUE;
	blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;
	blendAttachmentState.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

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
	    VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicState =
	    vkb::initializers::pipeline_dynamic_state_create_info(dynamicStateEnables);

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
	    base->load_shader("render_octomap", "imgui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
	    base->load_shader("render_octomap", "imgui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = vkb::initializers::pipeline_create_info(pipelineLayout, renderPass);

	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState    = &colorBlendState;
	pipelineCreateInfo.pMultisampleState   = &multisampleState;
	pipelineCreateInfo.pViewportState      = &viewportState;
	pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
	pipelineCreateInfo.pDynamicState       = &dynamicState;
	pipelineCreateInfo.stageCount          = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages             = shaderStages.data();

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
	vertexInputState.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertexInputBindings.size());
	vertexInputState.pVertexBindingDescriptions           = vertexInputBindings.data();
	vertexInputState.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertexInputAttributes.size());
	vertexInputState.pVertexAttributeDescriptions         = vertexInputAttributes.data();

	pipelineCreateInfo.pVertexInputState = &vertexInputState;

	VK_CHECK(vkCreateGraphicsPipelines(base->get_render_context().get_device().get_handle(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

// Starts a new imGui frame and sets up windows and ui elements
bool ImGUIUtil::newFrame(bool updateFrameGraph)
{
	ImGui::NewFrame();

	// Draw only the left sidebar as an interactive window.
	// The map viewport area remains free for the camera to receive mouse input.
	{
		ImGuiStyle &style    = ImGui::GetStyle();
		style.ChildRounding  = 0.0f;
		style.WindowRounding = 12.0f;
		style.FrameRounding  = 12.0f;

		ImGuiIO &io = ImGui::GetIO();

		const float padding             = 20.0f;
		const float sidebar_inner_width = 240.0f;
		const float sidebar_width       = sidebar_inner_width + padding * 2.0f;
		const float btn_w               = sidebar_inner_width;
		const float btn_h               = 52.0f;
		const float gap                 = 10.0f;

		// Colors (same palette as MapView)
		const ImVec4 sidebarColor      = ImVec4(0x41 / 255.0f, 0x40 / 255.0f, 0x42 / 255.0f, 1.0f);
		const ImVec4 buttonColor       = ImVec4(0x00 / 255.0f, 0xF1 / 255.0f, 0xC6 / 255.0f, 1.0f);
		const ImVec4 buttonActiveColor = ImVec4(0x00 / 255.0f, 0x94 / 255.0f, 0x81 / 255.0f, 1.0f);
		const ImVec4 blackColor        = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(sidebar_width, io.DisplaySize.y), ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, sidebarColor);
		ImGui::PushStyleColor(ImGuiCol_Text, blackColor);
		ImGui::Begin("Sidebar##render_octomap", nullptr,
		             ImGuiWindowFlags_NoTitleBar |
		                 ImGuiWindowFlags_NoResize |
		                 ImGuiWindowFlags_NoMove |
		                 ImGuiWindowFlags_NoScrollbar |
		                 ImGuiWindowFlags_NoScrollWithMouse |
		                 ImGuiWindowFlags_NoSavedSettings);

		auto sidebar_button = [&](const char *label, MapView::ViewState state, const char *id) {
			ImVec4 c = (MapsView.currentState == state) ? buttonActiveColor : buttonColor;
			ImGui::PushStyleColor(ImGuiCol_Button, c);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, c);
			ImGui::PushID(id);
			bool pressed = ImGui::Button(label, ImVec2(btn_w, btn_h));
			ImGui::PopID();
			ImGui::PopStyleColor(3);
			if (pressed)
			{
				MapsView.currentState = state;
				MapsView.stateChanged = true;
			}
			ImGui::Dummy(ImVec2(0.0f, gap));
		};

		sidebar_button("OCTOMAP", MapView::ViewState::Octomap, "##btn_octomap");
		sidebar_button("GLTF MAP", MapView::ViewState::GLTFRegular, "##btn_gltf");
		sidebar_button("SPLATS", MapView::ViewState::GLTFSplats, "##btn_splats");

		ImGui::End();
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();

		// Compute 3D viewport rectangle (right side)
		MapsView.mapPos  = {sidebar_width, padding};
		MapsView.mapSize = {io.DisplaySize.x - sidebar_width - padding, io.DisplaySize.y - padding * 2.0f};

		// Draw a non-interactive semi-transparent map panel background.
		ImGui::SetNextWindowPos(ImVec2(MapsView.mapPos.x, MapsView.mapPos.y), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(MapsView.mapSize.x, MapsView.mapSize.y), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.35f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, sidebarColor);
		ImGui::Begin("MapPanel##render_octomap", nullptr,
		             ImGuiWindowFlags_NoTitleBar |
		                 ImGuiWindowFlags_NoResize |
		                 ImGuiWindowFlags_NoMove |
		                 ImGuiWindowFlags_NoScrollbar |
		                 ImGuiWindowFlags_NoScrollWithMouse |
		                 ImGuiWindowFlags_NoSavedSettings |
		                 ImGuiWindowFlags_NoInputs);
		ImGui::End();
		ImGui::PopStyleColor();
	}

	ImGui::EndFrame();

	// Render to generate draw buffers
	ImGui::Render();
	if (needsUpdateBuffers)
	{
		needsUpdateBuffers = false;
		return true;
	}
	return false;
}

// Update vertex and index buffer containing the imGui elements when required
// Returns true if buffers were recreated (requiring command buffer rebuild)
bool ImGUIUtil::updateBuffers()
{
	ImDrawData *imDrawData = ImGui::GetDrawData();

	if (!imDrawData)
	{
		return false;
	}

	// Note: Alignment is done inside buffer creation
	VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize  = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

	if ((vertexBufferSize == 0) || (indexBufferSize == 0))
	{
		return false;
	}

	bool buffersRecreated = false;

	// Update buffers only if vertex or index count has been changed compared to current buffer size
	if ((vertexBuffer->get_handle() == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount))
	{
		// Wait for GPU to finish using the old buffer before destroying it
		vkDeviceWaitIdle(base->get_render_context().get_device().get_handle());
		vertexBuffer.reset();
		vertexBuffer = std::make_unique<vkb::core::BufferC>(base->get_render_context().get_device(), vertexBufferSize,
		                                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		                                                    VMA_MEMORY_USAGE_GPU_TO_CPU);
		vertexCount  = imDrawData->TotalVtxCount;
		vertexBuffer->set_debug_name("GUI Util vertex buffer");
		buffersRecreated = true;
	}

	if ((indexBuffer->get_handle() == VK_NULL_HANDLE) || (indexCount != imDrawData->TotalIdxCount))
	{
		// Wait for GPU to finish using the old buffer before destroying it
		if (!buffersRecreated)
		{
			vkDeviceWaitIdle(base->get_render_context().get_device().get_handle());
		}
		indexCount = imDrawData->TotalIdxCount;

		indexBuffer.reset();
		indexBuffer = std::make_unique<vkb::core::BufferC>(base->get_render_context().get_device(), indexBufferSize,
		                                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		                                                   VMA_MEMORY_USAGE_GPU_TO_CPU);
		indexBuffer->set_debug_name("GUI index buffer");
		buffersRecreated = true;
	}

	// Upload data
	ImDrawVert *vtxDst = reinterpret_cast<ImDrawVert *>(vertexBuffer->map());
	ImDrawIdx  *idxDst = reinterpret_cast<ImDrawIdx *>(indexBuffer->map());

	for (int n = 0; n < imDrawData->CmdListsCount; n++)
	{
		const ImDrawList *cmd_list = imDrawData->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	vertexBuffer->flush();
	indexBuffer->flush();

	return buffersRecreated;
}

void ImGUIUtil::drawFrame(VkCommandBuffer commandBuffer)
{
	ImGuiIO &io = ImGui::GetIO();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	VkViewport viewport = vkb::initializers::viewport(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	// UI scale and translate via push constants
	pushConstBlock.scale     = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
	pushConstBlock.translate = glm::vec2(-1.0f);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

	// Render commands
	ImDrawData *imDrawData   = ImGui::GetDrawData();
	uint32_t    vertexOffset = 0;
	uint32_t    indexOffset  = 0;

	if (imDrawData->CmdListsCount > 0)
	{
		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->get_handle(), offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer->get_handle(), 0, VK_INDEX_TYPE_UINT16);

		for (auto i = 0; i < imDrawData->CmdListsCount; i++)
		{
			const ImDrawList *cmd_list = imDrawData->CmdLists[i];
			for (auto j = 0; j < cmd_list->CmdBuffer.Size; j++)
			{
				const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
				VkRect2D         scissorRect;
				scissorRect.offset.x      = glm::max(pcmd->ClipRect.x, 0.0f);
				scissorRect.offset.y      = glm::max(pcmd->ClipRect.y, 0.0f);
				scissorRect.extent.width  = static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x);
				scissorRect.extent.height = static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y);
				vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

				if (static_cast<void *>(pcmd->TextureId) != nullptr)
				{
					VkDescriptorSet desc_set[1] = {static_cast<VkDescriptorSet>(pcmd->TextureId)};
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, desc_set, 0, nullptr);
				}
				else
				{
					// bind the font
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
				}
				vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				indexOffset += pcmd->ElemCount;
			}
			vertexOffset += cmd_list->VtxBuffer.Size;
		}
	}
}

void ImGUIUtil::TextColorAlign(int align, const ImVec4 &col, const char *text, ...)
{
	va_list vaList;
	va_start(vaList, text);

	float font_width = ImGui::CalcTextSize(text).x;

	switch (align)
	{
		case 1:
			ImGui::SameLine(
			    ImGui::GetContentRegionAvail().x * 0.5f - font_width * 0.5f);
			break;
		case 2:
			ImGui::SameLine(
			    ImGui::GetContentRegionAvail().x - font_width);
			break;
		case 0:
		default:
			break;
	}

	ImGui::TextColoredV(col, text, vaList);

	va_end(vaList);
}

void ImGUIUtil::handle_key_event(vkb::KeyCode code, vkb::KeyAction action)
{
	ImGuiIO       &io        = ImGui::GetIO();
	const ImGuiKey imgui_key = KeyCodeToImGuiKey(code);
	if (imgui_key != ImGuiKey_None)
	{
		io.AddKeyEvent(imgui_key, action == vkb::KeyAction::Down || action == vkb::KeyAction::Repeat);
	}

	// Update modifier states using ImGui keys
	io.KeyCtrl  = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
	io.KeyShift = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
	io.KeyAlt   = ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt);
	io.KeySuper = ImGui::IsKeyDown(ImGuiKey_LeftSuper) || ImGui::IsKeyDown(ImGuiKey_RightSuper);
}

bool ImGUIUtil::GetWantKeyCapture()
{
	ImGuiIO &io = ImGui::GetIO();
	return io.WantCaptureKeyboard;
}

void ImGUIUtil::charPressed(uint32_t key)
{
	ImGuiIO &io = ImGui::GetIO();
	io.AddInputCharacter(key);
}
