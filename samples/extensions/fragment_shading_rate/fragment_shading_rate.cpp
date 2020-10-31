/* Copyright (c) 2020, Sascha Willems
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
 * Using variable fragment shading rates from a subpass attachment with VK_KHR_fragment_shading_rate
 * This sample creates an image that contains different shading rates, which are then sampled during rendering
 */

/*
* TODOS:
* - Check shading rate image format support for VK_FORMAT_FEATURE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR
*/

#include "fragment_shading_rate.h"

FragmentShadingRate::FragmentShadingRate()
{
	title = "Fragment shading rate";
	// Enable instance and device extensions required to use VK_KHR_fragment_shading_rate
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_MULTIVIEW_EXTENSION_NAME);
	add_device_extension(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
	add_device_extension(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
}

FragmentShadingRate::~FragmentShadingRate()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), pipelines.sphere, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.skysphere, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		vkDestroySampler(get_device().get_handle(), textures.skysphere.sampler, nullptr);
		uniform_buffers.scene.reset();
	}
}

void FragmentShadingRate::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable the shading rate attachment feature required by this sample
	// These are passed to device creation via a pNext structure chain
	auto &requested_extension_features                         = gpu.request_extension_features<VkPhysicalDeviceFragmentShadingRateFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR);
	requested_extension_features.attachmentFragmentShadingRate = VK_TRUE;
	requested_extension_features.pipelineFragmentShadingRate   = VK_FALSE;
	requested_extension_features.primitiveFragmentShadingRate  = VK_FALSE;

	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

/*
* Create an image that contains the values used to determine the shading rates to apply during scene rendering
*/
void FragmentShadingRate::create_shading_rate_attachment()
{
	// Get all available shading rates
	std::vector<VkPhysicalDeviceFragmentShadingRateKHR> fragment_shading_rates{};
	uint32_t                                            fragment_shading_rate_count = 0;
	vkGetPhysicalDeviceFragmentShadingRatesKHR(get_device().get_gpu().get_handle(), &fragment_shading_rate_count, nullptr);
	if (fragment_shading_rate_count > 0)
	{
		fragment_shading_rates.resize(fragment_shading_rate_count);
		vkGetPhysicalDeviceFragmentShadingRatesKHR(get_device().get_gpu().get_handle(), &fragment_shading_rate_count, fragment_shading_rates.data());
	}

	// Shading rate image size depends on shading rate texel size
	// For each texel in the target image, there is a corresponding shading texel size width x height block in the shading rate image
	VkExtent3D image_extent{};
	image_extent.width  = static_cast<uint32_t>(ceil(width / (float) physical_device_fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.width));
	image_extent.height = static_cast<uint32_t>(ceil(height / (float) physical_device_fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.height));
	image_extent.depth  = 1;

	VkImageCreateInfo image_create_info{};
	image_create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType     = VK_IMAGE_TYPE_2D;
	image_create_info.format        = VK_FORMAT_R8_UINT;
	image_create_info.extent        = image_extent;
	image_create_info.mipLevels     = 1;
	image_create_info.arrayLayers   = 1;
	image_create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.usage         = VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VK_CHECK(vkCreateImage(device->get_handle(), &image_create_info, nullptr, &shading_rate_image.image));
	VkMemoryRequirements memory_requirements{};
	vkGetImageMemoryRequirements(device->get_handle(), shading_rate_image.image, &memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info{};
	memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = device->get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(device->get_handle(), &memory_allocate_info, nullptr, &shading_rate_image.memory));
	VK_CHECK(vkBindImageMemory(device->get_handle(), shading_rate_image.image, shading_rate_image.memory, 0));

	VkImageViewCreateInfo image_view_create_info{};
	image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.image                           = shading_rate_image.image;
	image_view_create_info.format                          = VK_FORMAT_R8_UINT;
	image_view_create_info.subresourceRange.baseMipLevel   = 0;
	image_view_create_info.subresourceRange.levelCount     = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount     = 1;
	image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	VK_CHECK(vkCreateImageView(device->get_handle(), &image_view_create_info, nullptr, &shading_rate_image.view));

	// Allocate a buffer that stores the shading rates
	VkDeviceSize buffer_size = image_extent.width * image_extent.height * sizeof(uint8_t);

	// @todo: Lazily taken from GLSL_EXT_fragment_shading_rate
	const int gl_ShadingRateFlag2VerticalPixelsEXT   = 1;
	const int gl_ShadingRateFlag4VerticalPixelsEXT   = 2;
	const int gl_ShadingRateFlag2HorizontalPixelsEXT = 4;
	const int gl_ShadingRateFlag4HorizontalPixelsEXT = 8;

	// Populate the buffer with lowest possible shading rate pattern (4x4)
	uint8_t  val                       = gl_ShadingRateFlag4VerticalPixelsEXT | gl_ShadingRateFlag4HorizontalPixelsEXT;
	uint8_t *shading_rate_pattern_data = new uint8_t[buffer_size];
	memset(shading_rate_pattern_data, val, buffer_size);

	// Create a circular pattern with decreasing sampling rates outwards (max. range, pattern)
	// @todo: Take from actual list of available shading rates?
	std::map<float, uint8_t> pattern_lookup = {
	    {8.0f, 0},
	    {12.0f, gl_ShadingRateFlag2VerticalPixelsEXT},
	    {16.0f, gl_ShadingRateFlag2HorizontalPixelsEXT},
	    {18.0f, gl_ShadingRateFlag2VerticalPixelsEXT | gl_ShadingRateFlag2HorizontalPixelsEXT},
	    {20.0f, gl_ShadingRateFlag4VerticalPixelsEXT | gl_ShadingRateFlag2HorizontalPixelsEXT},
	    {24.0f, gl_ShadingRateFlag2VerticalPixelsEXT | gl_ShadingRateFlag4HorizontalPixelsEXT}};

	uint8_t *ptrData = shading_rate_pattern_data;
	for (uint32_t y = 0; y < image_extent.height; y++)
	{
		for (uint32_t x = 0; x < image_extent.width; x++)
		{
			const float deltaX = ((float) image_extent.width / 2.0f - (float) x) / image_extent.width * 100.0f;
			const float deltaY = ((float) image_extent.height / 2.0f - (float) y) / image_extent.height * 100.0f;
			const float dist   = std::sqrt(deltaX * deltaX + deltaY * deltaY);
			for (auto pattern : pattern_lookup)
			{
				if (dist < pattern.first)
				{
					*ptrData = pattern.second;
					break;
				}
			}
			ptrData++;
		}
	}

	// Move shading rate pattern data to staging buffer
	std::unique_ptr<vkb::core::Buffer> staging_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                                                        buffer_size,
	                                                                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	                                                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);
	staging_buffer->update(shading_rate_pattern_data, buffer_size);
	delete[] shading_rate_pattern_data;

	// Upload the buffer containing the shading rates to the image that'll be used as the shading rate attachment inside our renderpass
	VkCommandBuffer         copy_cmd          = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkImageSubresourceRange subresource_range = {};
	subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.levelCount              = 1;
	subresource_range.layerCount              = 1;

	VkImageMemoryBarrier image_memory_barrier = vkb::initializers::image_memory_barrier();
	image_memory_barrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barrier.newLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	image_memory_barrier.srcAccessMask        = 0;
	image_memory_barrier.dstAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;
	image_memory_barrier.image                = shading_rate_image.image;
	image_memory_barrier.subresourceRange     = subresource_range;
	vkCmdPipelineBarrier(copy_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

	VkBufferImageCopy buffer_copy_region           = {};
	buffer_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	buffer_copy_region.imageSubresource.layerCount = 1;
	buffer_copy_region.imageExtent.width           = image_extent.width;
	buffer_copy_region.imageExtent.height          = image_extent.height;
	buffer_copy_region.imageExtent.depth           = 1;
	vkCmdCopyBufferToImage(copy_cmd, staging_buffer->get_handle(), shading_rate_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_copy_region);

	// Transfer image layout to fragment shading rate attachment layout required to access this in the renderpass
	image_memory_barrier.oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	image_memory_barrier.newLayout        = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
	image_memory_barrier.srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
	image_memory_barrier.dstAccessMask    = 0;
	image_memory_barrier.image            = shading_rate_image.image;
	image_memory_barrier.subresourceRange = subresource_range;
	vkCmdPipelineBarrier(copy_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

	device->flush_command_buffer(copy_cmd, queue, true);

	staging_buffer.reset();
}

/*
* The shading rate image needs to be invalidated and recreated when the frame buffer is resized
*/
void FragmentShadingRate::invalidate_shading_rate_attachment()
{
	vkDestroyImageView(device->get_handle(), shading_rate_image.view, nullptr);
	vkDestroyImage(device->get_handle(), shading_rate_image.image, nullptr);
	vkFreeMemory(device->get_handle(), shading_rate_image.memory, nullptr);
	shading_rate_image.view   = VK_NULL_HANDLE;
	shading_rate_image.image  = VK_NULL_HANDLE;
	shading_rate_image.memory = VK_NULL_HANDLE;
}

/*
* This sample uses a custom render pass setup, as the shading rate image needs to be passed to the sample's render / sub pass
*/
void FragmentShadingRate::setup_render_pass()
{
	// Query the fragment shading rate properties of the current implementation, we will need them later on
	physical_device_fragment_shading_rate_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 device_properties{};
	device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	device_properties.pNext = &physical_device_fragment_shading_rate_properties;
	vkGetPhysicalDeviceProperties2(get_device().get_gpu().get_handle(), &device_properties);

	std::array<VkAttachmentDescription2, 3> attachments = {};
	// Color attachment
	attachments[0].sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	attachments[0].format         = render_context->get_format();
	attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// Depth attachment
	attachments[1].sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	attachments[1].format         = depth_format;
	attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	// Fragment shading rate attachment
	attachments[2].sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	attachments[2].format         = VK_FORMAT_R8_UINT;
	attachments[2].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[2].loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachments[2].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].initialLayout  = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
	attachments[2].finalLayout    = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

	VkAttachmentReference2 color_reference = {};
	color_reference.sType                  = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	color_reference.attachment             = 0;
	color_reference.layout                 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color_reference.aspectMask             = VK_IMAGE_ASPECT_COLOR_BIT;

	VkAttachmentReference2 depth_reference = {};
	depth_reference.sType                  = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	depth_reference.attachment             = 1;
	depth_reference.layout                 = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depth_reference.aspectMask             = VK_IMAGE_ASPECT_DEPTH_BIT;

	// Setup the attachment reference for the shading rate image attachment in slot 2
	VkAttachmentReference2 fragment_shading_rate_reference = {};
	fragment_shading_rate_reference.sType                  = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	fragment_shading_rate_reference.attachment             = 2;
	fragment_shading_rate_reference.layout                 = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

	// Setup the attachment info for the shading rate image, which will be addeed to the sub pass via structure chaining (in pNext)
	VkFragmentShadingRateAttachmentInfoKHR fragment_shading_rate_attachment_info = {};
	fragment_shading_rate_attachment_info.sType                                  = VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
	fragment_shading_rate_attachment_info.pFragmentShadingRateAttachment         = &fragment_shading_rate_reference;
	fragment_shading_rate_attachment_info.shadingRateAttachmentTexelSize.width   = physical_device_fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.width;
	fragment_shading_rate_attachment_info.shadingRateAttachmentTexelSize.height  = physical_device_fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.height;

	VkSubpassDescription2 subpass_description   = {};
	subpass_description.sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
	subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.colorAttachmentCount    = 1;
	subpass_description.pColorAttachments       = &color_reference;
	subpass_description.pDepthStencilAttachment = &depth_reference;
	subpass_description.inputAttachmentCount    = 0;
	subpass_description.pInputAttachments       = nullptr;
	subpass_description.preserveAttachmentCount = 0;
	subpass_description.pPreserveAttachments    = nullptr;
	subpass_description.pResolveAttachments     = nullptr;
	subpass_description.pNext                   = &fragment_shading_rate_attachment_info;

	// Subpass dependencies for layout transitions
	std::array<VkSubpassDependency2, 2> dependencies = {};

	dependencies[0].sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
	dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass      = 0;
	dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
	dependencies[1].srcSubpass      = 0;
	dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo2 render_pass_create_info = {};
	render_pass_create_info.sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
	render_pass_create_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
	render_pass_create_info.pAttachments            = attachments.data();
	render_pass_create_info.subpassCount            = 1;
	render_pass_create_info.pSubpasses              = &subpass_description;
	render_pass_create_info.dependencyCount         = static_cast<uint32_t>(dependencies.size());
	render_pass_create_info.pDependencies           = dependencies.data();

	VK_CHECK(vkCreateRenderPass2(device->get_handle(), &render_pass_create_info, nullptr, &render_pass));
}

/*
* This sample uses a custom frame buffer setup, that includes the fragment shading rate image attachment
*/
void FragmentShadingRate::setup_framebuffer()
{
	// Create ths shading rate image attachment if not defined (first run and resize)
	if (shading_rate_image.image == VK_NULL_HANDLE)
	{
		create_shading_rate_attachment();
	}

	VkImageView attachments[3];
	// Depth/Stencil attachment is the same for all frame buffers
	attachments[1] = depth_stencil.view;
	// Fragment shading rate attachment
	attachments[2] = shading_rate_image.view;

	VkFramebufferCreateInfo framebuffer_create_info = {};
	framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_create_info.renderPass              = render_pass;
	framebuffer_create_info.attachmentCount         = 3;
	framebuffer_create_info.pAttachments            = attachments;
	framebuffer_create_info.width                   = get_render_context().get_surface_extent().width;
	framebuffer_create_info.height                  = get_render_context().get_surface_extent().height;
	framebuffer_create_info.layers                  = 1;

	// Create frame buffers for every swap chain image
	framebuffers.resize(render_context->get_render_frames().size());
	for (uint32_t i = 0; i < framebuffers.size(); i++)
	{
		attachments[0] = swapchain_buffers[i].view;
		VK_CHECK(vkCreateFramebuffer(device->get_handle(), &framebuffer_create_info, nullptr, &framebuffers[i]));
	}
}

void FragmentShadingRate::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[3];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};
	clear_values[2].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};

	VkRenderPassBeginInfo render_pass_begin_info = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass            = render_pass;
	render_pass_begin_info.renderArea.offset.x   = 0;
	render_pass_begin_info.renderArea.offset.y   = 0;
	render_pass_begin_info.clearValueCount       = 3;
	render_pass_begin_info.pClearValues          = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		VkClearValue clear_values[3];
		clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
		clear_values[1].depthStencil = {0.0f, 0};
		clear_values[2].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};

		// Final composition
		VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
		render_pass_begin_info.framebuffer              = framebuffers[i];
		render_pass_begin_info.renderPass               = render_pass;
		render_pass_begin_info.clearValueCount          = 3;
		render_pass_begin_info.renderArea.extent.width  = width;
		render_pass_begin_info.renderArea.extent.height = height;
		render_pass_begin_info.pClearValues             = clear_values;

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

		if (display_skysphere)
		{
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skysphere);
			push_const_block.object_type = 0;
			vkCmdPushConstants(draw_cmd_buffers[i], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_const_block), &push_const_block);
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
			draw_model(models.skysphere, draw_cmd_buffers[i]);
		}

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.sphere);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
		std::vector<glm::vec3> mesh_offsets = {
		    glm::vec3(-2.5f, 0.0f, 0.0f),
		    glm::vec3(0.0f, 0.0f, 0.0f),
		    glm::vec3(2.5f, 0.0f, 0.0f),
		};
		for (uint32_t j = 0; j < 3; j++)
		{
			push_const_block.object_type = 1;
			push_const_block.offset      = glm::vec4(mesh_offsets[j], 0.0f);
			vkCmdPushConstants(draw_cmd_buffers[i], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_const_block), &push_const_block);
			draw_model(models.scene, draw_cmd_buffers[i]);
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void FragmentShadingRate::load_assets()
{
	models.skysphere   = load_model("scenes/geosphere.gltf");
	textures.skysphere = load_texture("textures/skysphere_rgba.ktx");
	models.scene       = load_model("scenes/textured_unit_cube.gltf");
	textures.scene     = load_texture("textures/crate02_color_height_rgba.ktx");
}

void FragmentShadingRate::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6)};
	uint32_t                   num_descriptor_sets = 4;
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void FragmentShadingRate::setup_descriptor_set_layout()
{
	// Scene rendering descriptors
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);

	// Pass object offset and color via push constant
	VkPushConstantRange push_constant_range            = vkb::initializers::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(push_const_block), 0);
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void FragmentShadingRate::setup_descriptor_sets()
{
	// Shared model object descriptor set
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo            scene_buffer_descriptor      = create_descriptor(*uniform_buffers.scene);
	VkDescriptorImageInfo             environment_image_descriptor = create_descriptor(textures.skysphere);
	VkDescriptorImageInfo             sphere_image_descriptor      = create_descriptor(textures.scene);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets        = {
        vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &scene_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &sphere_image_descriptor),
    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void FragmentShadingRate::prepare_pipelines()
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

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(
	        pipeline_layout,
	        render_pass,
	        0);

	std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_states = {
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	};

	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages    = shader_stages.data();

	// Scene rendering pipeline

	// Setup the fragment shading rate state for our pipeline
	VkPipelineFragmentShadingRateStateCreateInfoKHR pipeline_fragment_shading_rate_state = {};
	pipeline_fragment_shading_rate_state.sType                                           = VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR;
	pipeline_fragment_shading_rate_state.fragmentSize.width                              = 1;
	pipeline_fragment_shading_rate_state.fragmentSize.height                             = 1;
	// The combiners determine how the different shading rate values for the pipeline, primitives and attachment are combined
	// We set them up so that the shading rates stored in the shading rate attachment replace all other values (combinerOps[1])
	// Combiner for pipeline (A) and primitive (B) - Not used in this sample
	pipeline_fragment_shading_rate_state.combinerOps[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
	// Combiner for pipeline (A) and attachment (B)
	pipeline_fragment_shading_rate_state.combinerOps[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
	// Pass the state via structure chaining
	pipeline_create_info.pNext = &pipeline_fragment_shading_rate_state;

	// Vertex bindings an attributes for model rendering
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};
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

	pipeline_create_info.pVertexInputState = &vertex_input_state;

	pipeline_create_info.layout     = pipeline_layout;
	pipeline_create_info.renderPass = render_pass;

	// Skysphere
	shader_stages[0] = load_shader("fragment_shading_rate/scene.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("fragment_shading_rate/scene.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.skysphere));

	// Objects
	// Enable depth test and write
	depth_stencil_state.depthWriteEnable = VK_TRUE;
	depth_stencil_state.depthTestEnable  = VK_TRUE;
	// Flip cull mode
	rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.sphere));
}

void FragmentShadingRate::prepare_uniform_buffers()
{
	uniform_buffers.scene = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                            sizeof(ubo_scene),
	                                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                            VMA_MEMORY_USAGE_CPU_TO_GPU);
	update_uniform_buffers();
}

void FragmentShadingRate::update_uniform_buffers()
{
	ubo_scene.projection          = camera.matrices.perspective;
	ubo_scene.modelview           = camera.matrices.view * glm::mat4(1.0f);
	ubo_scene.skysphere_modelview = camera.matrices.view;
	ubo_scene.color_shading_rate  = color_shading_rate;
	uniform_buffers.scene->convert_and_update(ubo_scene);
}

void FragmentShadingRate::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

bool FragmentShadingRate::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::FirstPerson;
	camera.set_position(glm::vec3(0.0f, 0.0f, -4.0f));

	// Note: Using Revsered depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, (float) width / (float) height, 256.0f, 0.1f);

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

void FragmentShadingRate::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
	if (camera.updated)
		update_uniform_buffers();
}

void FragmentShadingRate::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.checkbox("Color shading rates", &color_shading_rate))
		{
			update_uniform_buffers();
		}
		if (drawer.checkbox("skysphere", &display_skysphere))
		{
			build_command_buffers();
		}
	}
}

void FragmentShadingRate::resize(const uint32_t width, const uint32_t height)
{
	invalidate_shading_rate_attachment();
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
}

std::unique_ptr<vkb::VulkanSample> create_fragment_shading_rate()
{
	return std::make_unique<FragmentShadingRate>();
}