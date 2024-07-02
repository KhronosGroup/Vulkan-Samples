/* Copyright (c) 2021-2024, Arm Limited and Contributors
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

#include "descriptor_indexing.h"

static constexpr uint32_t NumDescriptorsStreaming  = 2048;
static constexpr uint32_t NumDescriptorsNonUniform = 64;

DescriptorIndexing::DescriptorIndexing()
{
	title = "Descriptor indexing";

	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	add_device_extension(VK_KHR_MAINTENANCE3_EXTENSION_NAME);

	// Works around a validation layer bug with descriptor pool allocation with VARIABLE_COUNT.
	// See: https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/2350.
	add_device_extension(VK_KHR_MAINTENANCE1_EXTENSION_NAME);

#if defined(PLATFORM__MACOS) && TARGET_OS_OSX && defined(VK_EXT_layer_settings)
	// On macOS use layer setting to configure MoltenVK for using Metal argument buffers (needed for descriptor indexing)
	// MoltenVK supports Metal argument buffers on macOS, iOS possible in future (see https://github.com/KhronosGroup/MoltenVK/issues/1651)
	add_instance_extension(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME, /*optional*/ false);

	VkLayerSettingEXT layerSetting;
	layerSetting.pLayerName = "MoltenVK";
	layerSetting.pSettingName = "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS";
	layerSetting.type = VK_LAYER_SETTING_TYPE_INT32_EXT;
	layerSetting.valueCount = 1;

	static const int32_t useMetalArgumentBuffers = 1;
	layerSetting.pValues = &useMetalArgumentBuffers;

	add_layer_setting(layerSetting);
#endif
}

DescriptorIndexing::~DescriptorIndexing()
{
	if (has_device())
	{
		VkDevice vk_device = get_device().get_handle();
		vkDestroyPipelineLayout(vk_device, pipelines.pipeline_layout, nullptr);
		vkDestroyPipeline(vk_device, pipelines.non_uniform_indexing, nullptr);
		vkDestroyPipeline(vk_device, pipelines.update_after_bind, nullptr);

		vkDestroyDescriptorSetLayout(vk_device, descriptors.set_layout, nullptr);
		vkDestroyDescriptorPool(vk_device, descriptors.descriptor_pool, nullptr);

		vkDestroyDescriptorSetLayout(vk_device, sampler.set_layout, nullptr);
		vkDestroySampler(vk_device, sampler.sampler, nullptr);
		vkDestroyDescriptorPool(vk_device, sampler.descriptor_pool, nullptr);

		for (auto &image : test_images)
		{
			vkDestroyImageView(vk_device, image.image_view, nullptr);
			vkDestroyImage(vk_device, image.image, nullptr);
			vkFreeMemory(vk_device, image.memory, nullptr);
		}
	}
}

void DescriptorIndexing::build_command_buffers()
{
	// We build command buffers every frame in render(), so don't build anything here.
}

void DescriptorIndexing::render(float delta_time)
{
	ApiVulkanSample::prepare_frame();

	VK_CHECK(vkWaitForFences(get_device().get_handle(), 1, &wait_fences[current_buffer], VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(get_device().get_handle(), 1, &wait_fences[current_buffer]));

	VkViewport viewport = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
	VkRect2D   scissor  = {{0, 0}, {width, height}};

	recreate_current_command_buffer();
	auto cmd         = draw_cmd_buffers[current_buffer];
	auto begin_info  = vkb::initializers::command_buffer_begin_info();
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &begin_info);

	VkRenderPassBeginInfo render_pass_begin    = vkb::initializers::render_pass_begin_info();
	render_pass_begin.renderPass               = render_pass;
	render_pass_begin.renderArea.extent.width  = width;
	render_pass_begin.renderArea.extent.height = height;
	render_pass_begin.clearValueCount          = 2;
	VkClearValue clears[2]                     = {};
	clears[0].color.float32[0]                 = 0.033f;
	clears[0].color.float32[1]                 = 0.073f;
	clears[0].color.float32[2]                 = 0.133f;
	render_pass_begin.pClearValues             = clears;
	render_pass_begin.framebuffer              = framebuffers[current_buffer];

	vkCmdBeginRenderPass(cmd, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

	// First, draw all textures with nonuniform indexing. Each instance will sample from its own texture.
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.non_uniform_indexing);

	accumulated_time += 0.2f * delta_time;
	accumulated_time = glm::fract(accumulated_time);
	float phase      = glm::two_pi<float>() * accumulated_time;
	vkCmdPushConstants(cmd, pipelines.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &phase);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.pipeline_layout, 0, 1, &descriptors.descriptor_set_nonuniform, 0, nullptr);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.pipeline_layout, 1, 1, &sampler.descriptor_set, 0, nullptr);
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	vkCmdDraw(cmd, 4, NumDescriptorsNonUniform, 0, 0);

	// The update-after-bind style, i.e. "streamed" descriptors. We bind the descriptor set once, and update descriptors as we go.
	// With update-after-bind we can update the descriptor set from multiple threads, and we can update descriptors while the descriptor set is bound.
	// We can update descriptors at any time, as long as the GPU is not actually accessing the descriptor.
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.update_after_bind);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.pipeline_layout, 0, 1, &descriptors.descriptor_set_update_after_bind, 0, nullptr);

	for (unsigned i = 0; i < NumDescriptorsNonUniform; i++)
	{
		VkDescriptorImageInfo image_info = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, test_images[i].image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkWriteDescriptorSet  write      = vkb::initializers::write_descriptor_set(descriptors.descriptor_set_update_after_bind, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0, &image_info);

		// One way we can use VK_EXT_descriptor_indexing is to treat the update-after-bind descriptor set as a ring buffer where we write descriptors,
		// and we use push constants as a way to index into the "bindless" descriptor set.
		write.dstArrayElement = descriptor_offset;
		vkCmdPushConstants(cmd, pipelines.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(uint32_t), sizeof(uint32_t), &descriptor_offset);
		descriptor_offset = (descriptor_offset + 1) % NumDescriptorsStreaming;
		vkUpdateDescriptorSets(get_device().get_handle(), 1, &write, 0, nullptr);

		// We can use base instance as a way to offset gl_InstanceIndex in a shader.
		// This can also be a nice way to pass down an offset for bindless purposes in vertex shaders that does not consume a push constant.
		// In this case however, we only use the instance offset to place the textures where we expect
		// and we cannot directly access gl_InstanceIndex in fragment shaders.
		vkCmdDraw(cmd, 4, 1, 0, i);
	}

	draw_ui(cmd);

	vkCmdEndRenderPass(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, wait_fences[current_buffer]));
	ApiVulkanSample::submit_frame();
}

void DescriptorIndexing::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Device properties"))
	{
		// Display some common properties. Only bother with sampled image since that's what we're using here.
		drawer.text("maxDescriptorSetUpdateAfterBindSampledImages: %u", descriptor_indexing_properties.maxDescriptorSetUpdateAfterBindSampledImages);
		drawer.text("maxPerStageUpdateAfterBindResources: %u", descriptor_indexing_properties.maxPerStageUpdateAfterBindResources);
		drawer.text("quadDivergentImplicitLod: %u", descriptor_indexing_properties.quadDivergentImplicitLod);
		drawer.text("shaderSampledImageArrayNonUniformIndexingNative: %u", descriptor_indexing_properties.shaderSampledImageArrayNonUniformIndexingNative);
		drawer.text("maxUpdateAfterBindDescriptorsInAllPools: %u", descriptor_indexing_properties.maxUpdateAfterBindDescriptorsInAllPools);
	}
}

void DescriptorIndexing::create_immutable_sampler_descriptor_set()
{
	// Calculate valid filter
	VkFilter filter = VK_FILTER_LINEAR;
	vkb::make_filters_valid(get_device().get_gpu().get_handle(), format, &filter);

	// The common case for bindless is to have an array of sampled images, not combined image sampler.
	// It is more efficient to use a single sampler instead, and we can just use a single immutable sampler for this purpose.
	// Create the sampler, descriptor set layout and allocate an immutable descriptor set.
	VkSamplerCreateInfo create_info = vkb::initializers::sampler_create_info();
	create_info.minFilter           = filter;
	create_info.magFilter           = filter;
	create_info.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	create_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.maxLod              = VK_LOD_CLAMP_NONE;

	VK_CHECK(vkCreateSampler(get_device().get_handle(), &create_info, nullptr, &sampler.sampler));

	VkDescriptorSetLayoutBinding binding = vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	binding.pImmutableSamplers           = &sampler.sampler;

	VkDescriptorSetLayoutCreateInfo set_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(&binding, 1);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &set_layout_create_info, nullptr, &sampler.set_layout));

	VkDescriptorPoolSize       pool_size = vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_SAMPLER, 1);
	VkDescriptorPoolCreateInfo pool      = vkb::initializers::descriptor_pool_create_info(1, &pool_size, 1);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &pool, nullptr, &sampler.descriptor_pool));

	VkDescriptorSetAllocateInfo allocate_info = vkb::initializers::descriptor_set_allocate_info(sampler.descriptor_pool, &sampler.set_layout, 1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &allocate_info, &sampler.descriptor_set));
}

void DescriptorIndexing::create_bindless_descriptors()
{
	VkDescriptorSetLayoutBinding binding = vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
#if defined(PLATFORM__MACOS)
																							// on macOS variable descriptor counts don't seem to work, use max expected count
																							std::max(NumDescriptorsStreaming, NumDescriptorsNonUniform));
#else
	                                                                                        descriptor_indexing_properties.maxDescriptorSetUpdateAfterBindSampledImages);
#endif

	VkDescriptorSetLayoutCreateInfo set_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(&binding, 1);

	// We're going to use update-after-bind, so we need to make sure the flag is set correctly in the set layout.
	// These sets need to be allocated with UPDATE_AFTER_BIND pools later.
	set_layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

	// We're going to use the full flexibility VK_EXT_descriptor_indexing allows us, in order, these binding flags express that we can:
	// - Use a variable amount of descriptors in an array. This is extremely useful when using VK_EXT_descriptor_indexing, since we do not have to
	//   allocate a fixed amount of descriptors for each descriptor set. In many cases, it is far more flexible to use runtime sized descriptor arrays.
	//   The descriptorCount in the descriptor set layout now just expresses an upper bound.
	//   When we later allocate the descriptor set, we can declare how large we want the array to be.
	// - Partially bound means that we don't have to bind every descriptor. This is critical if we want to make use of descriptor "streaming".
	//   A descriptor only has to be bound if it is actually used by a shader.
	// - Update-after-bind is another critical component of descriptor indexing,
	//   which allows us to update descriptors after a descriptor set has been bound to a command buffer.
	//   This is critical for streaming descriptors, but it also relaxed threading requirements.
	//   Multiple threads can update descriptors concurrently on the same descriptor set.
	// - Update-Unused-While-Pending is somewhat subtle, and allows you to update a descriptor while a command buffer is executing.
	//   The only restriction is that the descriptor cannot actually be accessed by the GPU.

	// Typically, if you're using descriptor indexing, you will want to use all four of these, but all of these are separate feature bits.
	const VkDescriptorBindingFlagsEXT flags =
	    VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
	    VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
	    VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
	    VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

	// In unextended Vulkan, there is no way to pass down flags to a binding, so we're going to do so via a pNext.
	// Each pBinding has a corresponding pBindingFlags.
	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags{};
	binding_flags.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	binding_flags.bindingCount   = 1;
	binding_flags.pBindingFlags  = &flags;
	set_layout_create_info.pNext = &binding_flags;

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &set_layout_create_info, nullptr, &descriptors.set_layout));

	// We're going to allocate two separate descriptor sets from the same pool, and here VARIABLE_DESCRIPTOR_COUNT comes in handy!
	// For the non-uniform indexing part, we allocate few descriptors, and for the streaming case, we allocate a fairly large ring buffer of descriptors we can play around with.
#if defined(PLATFORM__MACOS)
	// on macOS variable descriptor counts don't seem to work, use pool size of max expected count x 2 (for 2 allocations)
	VkDescriptorPoolSize       pool_size = vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, std::max(NumDescriptorsStreaming, NumDescriptorsNonUniform)*2);
#else
	VkDescriptorPoolSize       pool_size = vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, NumDescriptorsStreaming + NumDescriptorsNonUniform);
#endif
	VkDescriptorPoolCreateInfo pool      = vkb::initializers::descriptor_pool_create_info(1, &pool_size, 2);

	// The pool is marked update-after-bind. Be aware that there is a global limit to the number of descriptors can be allocated at any one time.
	// UPDATE_AFTER_BIND descriptors is somewhat of a precious resource, but min-spec in Vulkan is at least 500k descriptors, which should be more than enough.
	pool.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &pool, nullptr, &descriptors.descriptor_pool));

	VkDescriptorSetAllocateInfo allocate_info = vkb::initializers::descriptor_set_allocate_info(descriptors.descriptor_pool, &descriptors.set_layout, 1);

	// Just like descriptor flags, for each descriptor set we allocate, we can describe how large the descriptor array should be.
	VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variable_info{};
	variable_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
	variable_info.descriptorSetCount = 1;
	allocate_info.pNext              = &variable_info;

	variable_info.pDescriptorCounts = &NumDescriptorsStreaming;
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &allocate_info, &descriptors.descriptor_set_update_after_bind));
	variable_info.pDescriptorCounts = &NumDescriptorsNonUniform;
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &allocate_info, &descriptors.descriptor_set_nonuniform));
}

void DescriptorIndexing::create_pipelines()
{
	VkDescriptorSetLayout      set_layouts[]      = {descriptors.set_layout, sampler.set_layout};
	VkPipelineLayoutCreateInfo layout_create_info = vkb::initializers::pipeline_layout_create_info(set_layouts, 2);

	// To vertex shader we pass a phase to rotate the quads.
	// To fragment shader we pass down an index, which is used to access the descriptor array.
	const std::vector<VkPushConstantRange> ranges = {
	    vkb::initializers::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, sizeof(uint32_t), 0),
	    vkb::initializers::push_constant_range(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(uint32_t), sizeof(uint32_t)),
	};
	layout_create_info.pushConstantRangeCount = static_cast<uint32_t>(ranges.size());
	layout_create_info.pPushConstantRanges    = ranges.data();
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_create_info, nullptr, &pipelines.pipeline_layout));

	VkGraphicsPipelineCreateInfo    info{};
	VkPipelineShaderStageCreateInfo stages[2];
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	VkPipelineVertexInputStateCreateInfo   vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_GREATER);
	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState>      dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables);

	info.pVertexInputState   = &vertex_input_state;
	info.pInputAssemblyState = &input_assembly_state;
	info.pRasterizationState = &rasterization_state;
	info.pColorBlendState    = &color_blend_state;
	info.pDepthStencilState  = &depth_stencil_state;
	info.pViewportState      = &viewport_state;
	info.pMultisampleState   = &multisample_state;
	info.pDynamicState       = &dynamic_state;
	info.layout              = pipelines.pipeline_layout;
	info.renderPass          = render_pass;

	info.pStages    = stages;
	info.stageCount = 2;

	stages[0] = load_shader("descriptor_indexing", "nonuniform-quads.vert", VK_SHADER_STAGE_VERTEX_BIT);
	stages[1] = load_shader("descriptor_indexing", "nonuniform-quads.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &pipelines.non_uniform_indexing));

	stages[0] = load_shader("descriptor_indexing", "update-after-bind-quads.vert", VK_SHADER_STAGE_VERTEX_BIT);
	stages[1] = load_shader("descriptor_indexing", "update-after-bind-quads.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &pipelines.update_after_bind));
}

DescriptorIndexing::TestImage DescriptorIndexing::create_image(const float rgb[3], unsigned image_seed)
{
	// Fairly basic setup, generate some random textures so we can visualize that we are sampling many different textures.
	// Note: since we're creating the texture data ourselves, it will already be in linear colorspace so we set the format
	// as unorm, not sRGB.
	DescriptorIndexing::TestImage test_image;

	VkImageCreateInfo image_info = vkb::initializers::image_create_info();
	image_info.format            = format;
	image_info.extent            = {16, 16, 1};
	image_info.mipLevels         = 1;
	image_info.arrayLayers       = 1;
	image_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage             = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	image_info.imageType         = VK_IMAGE_TYPE_2D;
	image_info.samples           = VK_SAMPLE_COUNT_1_BIT;
	image_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateImage(get_device().get_handle(), &image_info, nullptr, &test_image.image));

	VkMemoryAllocateInfo memory_allocation_info = vkb::initializers::memory_allocate_info();
	VkMemoryRequirements memory_requirements;

	vkGetImageMemoryRequirements(get_device().get_handle(), test_image.image, &memory_requirements);
	memory_allocation_info.allocationSize  = memory_requirements.size;
	memory_allocation_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocation_info, nullptr, &test_image.memory));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), test_image.image, test_image.memory, 0));

	VkImageViewCreateInfo image_view           = vkb::initializers::image_view_create_info();
	image_view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	image_view.format                          = format;
	image_view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	image_view.subresourceRange.baseMipLevel   = 0;
	image_view.subresourceRange.levelCount     = 1;
	image_view.subresourceRange.baseArrayLayer = 0;
	image_view.subresourceRange.layerCount     = 1;
	image_view.image                           = test_image.image;
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &image_view, nullptr, &test_image.image_view));

	auto staging_buffer = vkb::core::Buffer::create_staging_buffer(get_device(), image_info.extent.width * image_info.extent.height * sizeof(uint32_t), nullptr);

	// Generate a random texture.
	// Fairly simple, create different colors and some different patterns.
	uint8_t *buffer = staging_buffer.map();
	for (uint32_t y = 0; y < image_info.extent.height; y++)
	{
		for (uint32_t x = 0; x < image_info.extent.width; x++)
		{
			uint8_t *rgba = buffer + 4 * (y * image_info.extent.width + x);

			const auto float_to_unorm8 = [](float v) -> uint8_t {
				v *= 255.0f;
				int rounded = static_cast<int>(v + 0.5f);
				if (rounded < 0)
				{
					return 0;
				}
				else if (rounded > 255)
				{
					return 255;
				}
				else
				{
					return static_cast<uint8_t>(rounded);
				}
			};

			uint32_t pattern;
			switch (image_seed & 3u)
			{
				default:
				{
					// Checkerboard
					pattern = ((x >> 2u) ^ (y >> 2u)) & 1u;
					break;
				}

				case 1:
				{
					// Horizontal stripes
					pattern = (x >> 2u) & 1u;
					break;
				}

				case 2:
				{
					// Vertical stripes
					pattern = (y >> 2u) & 1u;
					break;
				}

				case 3:
				{
					// Diagonal stripes
					pattern = ((x + y) >> 2u) & 1u;
					break;
				}
			}

			float pattern_color = pattern ? 0.25f : 1.0f;

			for (unsigned i = 0; i < 3; i++)
			{
				// Add in some random noise for good measure so we're sure we're not sampling the exact same texture over and over.
				rgba[i] = float_to_unorm8(pattern_color * rgb[i] + distribution(rnd));
			}
			rgba[3] = 0xff;
		}
	}
	staging_buffer.flush();
	staging_buffer.unmap();

	auto &cmd = get_device().request_command_buffer();
	cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vkb::image_layout_transition(cmd.get_handle(), test_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkBufferImageCopy copy_info{};
	copy_info.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
	copy_info.imageExtent      = image_info.extent;
	vkCmdCopyBufferToImage(cmd.get_handle(), staging_buffer.get_handle(), test_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_info);

	vkb::image_layout_transition(cmd.get_handle(), test_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VK_CHECK(cmd.end());

	// Not very optimal, but it's the simplest solution.
	get_device().get_suitable_graphics_queue().submit(cmd, VK_NULL_HANDLE);
	get_device().get_suitable_graphics_queue().wait_idle();

	return test_image;
}

void DescriptorIndexing::create_images()
{
	std::uniform_real_distribution<float> color_distribution{0.2f, 0.8f};
	float                                 colors[NumDescriptorsNonUniform][3];

	for (unsigned i = 0; i < NumDescriptorsNonUniform; i++)
	{
		for (unsigned j = 0; j < 3; j++)
		{
			colors[i][j] = color_distribution(rnd);
		}
	}

	test_images.reserve(NumDescriptorsNonUniform);
	for (unsigned i = 0; i < NumDescriptorsNonUniform; i++)
	{
		test_images.push_back(create_image(colors[i], i));
	}

	// For the non-uniform case, we're going to access every texture in a single draw call,
	// prepare a descriptor set with all textures prepared ahead of time.
	for (unsigned i = 0; i < NumDescriptorsNonUniform; i++)
	{
		VkDescriptorImageInfo image_info = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, test_images[i].image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkWriteDescriptorSet  write      = vkb::initializers::write_descriptor_set(descriptors.descriptor_set_nonuniform, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0, &image_info);
		write.dstArrayElement            = i;
		vkUpdateDescriptorSets(get_device().get_handle(), 1, &write, 0, nullptr);
	}
}

bool DescriptorIndexing::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	create_bindless_descriptors();
	create_immutable_sampler_descriptor_set();
	create_pipelines();
	create_images();

	prepared = true;
	return true;
}

void DescriptorIndexing::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	gpu.get_mutable_requested_features().shaderSampledImageArrayDynamicIndexing = VK_TRUE;

	auto &features = gpu.request_extension_features<VkPhysicalDeviceDescriptorIndexingFeaturesEXT>(
	    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT);

	features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

	// These are required to support the 4 descriptor binding flags we use in this sample.
	features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	features.descriptorBindingPartiallyBound              = VK_TRUE;
	features.descriptorBindingUpdateUnusedWhilePending    = VK_TRUE;
	features.descriptorBindingVariableDescriptorCount     = VK_TRUE;

	// Enables use of runtimeDescriptorArrays in SPIR-V shaders.
	features.runtimeDescriptorArray = VK_TRUE;

	// There are lot of properties associated with descriptor_indexing, grab them here.
	auto vkGetPhysicalDeviceProperties2KHR =
	    reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(get_instance().get_handle(), "vkGetPhysicalDeviceProperties2KHR"));
	assert(vkGetPhysicalDeviceProperties2KHR);
	VkPhysicalDeviceProperties2KHR device_properties{};

	descriptor_indexing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT;
	device_properties.sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
	device_properties.pNext              = &descriptor_indexing_properties;
	vkGetPhysicalDeviceProperties2KHR(gpu.get_handle(), &device_properties);
}

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_descriptor_indexing()
{
	return std::make_unique<DescriptorIndexing>();
}
