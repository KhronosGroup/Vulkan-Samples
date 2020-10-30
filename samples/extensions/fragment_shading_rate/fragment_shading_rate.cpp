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
 * Using variable fragment shading rates with VK_KHR_fragment_shading_rate
 */

/*
* TODOS:
* - Check shading rate image format support for VK_FORMAT_FEATURE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR
* - Transfer shading rate image to VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR
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
		vkDestroyPipeline(get_device().get_handle(), pipelines.skysphere, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.sphere, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.composition, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.bloom[0], nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.bloom[1], nullptr);

		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.models, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.composition, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.bloom_filter, nullptr);

		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.models, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.composition, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.bloom_filter, nullptr);

		vkDestroyRenderPass(get_device().get_handle(), offscreen.render_pass, nullptr);
		vkDestroyRenderPass(get_device().get_handle(), filter_pass.render_pass, nullptr);

		vkDestroyFramebuffer(get_device().get_handle(), offscreen.framebuffer, nullptr);
		vkDestroyFramebuffer(get_device().get_handle(), filter_pass.framebuffer, nullptr);

		vkDestroySampler(get_device().get_handle(), offscreen.sampler, nullptr);
		vkDestroySampler(get_device().get_handle(), filter_pass.sampler, nullptr);

		offscreen.depth.destroy(get_device().get_handle());
		offscreen.color[0].destroy(get_device().get_handle());
		offscreen.color[1].destroy(get_device().get_handle());

		filter_pass.color[0].destroy(get_device().get_handle());

		vkDestroySampler(get_device().get_handle(), textures.skysphere.sampler, nullptr);
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
	//if (gpu.get_features().samplerAnisotropy)
	//{
	//	gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	//}
}

/*
* Create an attachment that contains the shading rates 
* @todo: better comment
*/
void FragmentShadingRate::create_shading_rate_attachment()
{
	// Shading rate image size depends on shading rate texel size
	// For each texel in the target image, there is a corresponding shading texel size width x height block in the shading rate image
	VkExtent3D imageExtent{};
	imageExtent.width  = width;
	imageExtent.height = height;
	// @todo
	//imageExtent.width  = static_cast<uint32_t>(ceil(width / (float) physicalDeviceShadingRateImagePropertiesNV.shadingRateTexelSize.width));
	//imageExtent.height = static_cast<uint32_t>(ceil(height / (float) physicalDeviceShadingRateImagePropertiesNV.shadingRateTexelSize.height));
	imageExtent.depth = 1;

	// @todo: snake case

	VkImageCreateInfo imageCI{};
	imageCI.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.imageType     = VK_IMAGE_TYPE_2D;
	imageCI.format        = VK_FORMAT_R8_UINT;
	imageCI.extent        = imageExtent;
	imageCI.mipLevels     = 1;
	imageCI.arrayLayers   = 1;
	imageCI.samples       = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.usage         = VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VK_CHECK(vkCreateImage(device->get_handle(), &imageCI, nullptr, &shading_rate_image.image));
	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device->get_handle(), shading_rate_image.image, &memReqs);

	VkDeviceSize bufferSize = imageExtent.width * imageExtent.height * sizeof(uint8_t);

	VkMemoryAllocateInfo memAllloc{};
	memAllloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllloc.allocationSize  = memReqs.size;
	memAllloc.memoryTypeIndex = device->get_memory_type(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(device->get_handle(), &memAllloc, nullptr, &shading_rate_image.memory));
	VK_CHECK(vkBindImageMemory(device->get_handle(), shading_rate_image.image, shading_rate_image.memory, 0));

	VkImageViewCreateInfo imageViewCI{};
	imageViewCI.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCI.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.image                           = shading_rate_image.image;
	imageViewCI.format                          = VK_FORMAT_R8_UINT;
	imageViewCI.subresourceRange.baseMipLevel   = 0;
	imageViewCI.subresourceRange.levelCount     = 1;
	imageViewCI.subresourceRange.baseArrayLayer = 0;
	imageViewCI.subresourceRange.layerCount     = 1;
	imageViewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	VK_CHECK(vkCreateImageView(device->get_handle(), &imageViewCI, nullptr, &shading_rate_image.view));

	// Populate with lowest possible shading rate pattern
	uint8_t  val                    = VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X4_PIXELS_NV;
	uint8_t *shadingRatePatternData = new uint8_t[bufferSize];
	memset(shadingRatePatternData, val, bufferSize);

	// Create a circular pattern with decreasing sampling rates outwards (max. range, pattern)
	std::map<float, VkShadingRatePaletteEntryNV> patternLookup = {
	    {8.0f, VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_PIXEL_NV},
	    {12.0f, VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X1_PIXELS_NV},
	    {16.0f, VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_1X2_PIXELS_NV},
	    {18.0f, VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X2_PIXELS_NV},
	    {20.0f, VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X2_PIXELS_NV},
	    {24.0f, VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X4_PIXELS_NV}};

	uint8_t *ptrData = shadingRatePatternData;
	for (uint32_t y = 0; y < imageExtent.height; y++)
	{
		for (uint32_t x = 0; x < imageExtent.width; x++)
		{
			const float deltaX = (float) imageExtent.width / 2.0f - (float) x;
			const float deltaY = ((float) imageExtent.height / 2.0f - (float) y) * ((float) width / (float) height);
			const float dist   = std::sqrt(deltaX * deltaX + deltaY * deltaY);
			for (auto pattern : patternLookup)
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

	VkBuffer       stagingBuffer;
	VkDeviceMemory stagingMemory;

	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size        = bufferSize;
	bufferCreateInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(device->get_handle(), &bufferCreateInfo, nullptr, &stagingBuffer));
	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memReqs            = {};
	vkGetBufferMemoryRequirements(device->get_handle(), stagingBuffer, &memReqs);
	memAllocInfo.allocationSize  = memReqs.size;
	memAllocInfo.memoryTypeIndex = device->get_memory_type(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK(vkAllocateMemory(device->get_handle(), &memAllocInfo, nullptr, &stagingMemory));
	VK_CHECK(vkBindBufferMemory(device->get_handle(), stagingBuffer, stagingMemory, 0));

	uint8_t *mapped;
	VK_CHECK(vkMapMemory(device->get_handle(), stagingMemory, 0, memReqs.size, 0, (void **) &mapped));
	memcpy(mapped, shadingRatePatternData, bufferSize);
	vkUnmapMemory(device->get_handle(), stagingMemory);

	delete[] shadingRatePatternData;

	// Upload
	VkCommandBuffer         copyCmd          = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.levelCount              = 1;
	subresourceRange.layerCount              = 1;
	{
		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.srcAccessMask    = 0;
		imageMemoryBarrier.dstAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.image            = shading_rate_image.image;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}
	VkBufferImageCopy bufferCopyRegion{};
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width           = imageExtent.width;
	bufferCopyRegion.imageExtent.height          = imageExtent.height;
	bufferCopyRegion.imageExtent.depth           = 1;
	vkCmdCopyBufferToImage(copyCmd, stagingBuffer, shading_rate_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);
	{
		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV;
		imageMemoryBarrier.srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask    = 0;
		imageMemoryBarrier.image            = shading_rate_image.image;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}
	device->flush_command_buffer(copyCmd, queue, true);

	vkFreeMemory(device->get_handle(), stagingMemory, nullptr);
	vkDestroyBuffer(device->get_handle(), stagingBuffer, nullptr);
}

/*
* Custom render pass setup
* @todo: proper comment
*/
void FragmentShadingRate::setup_render_pass()
{
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
	// @todo: comment
	attachments[2].sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	attachments[2].format         = VK_FORMAT_R8_UINT;
	attachments[2].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[2].loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachments[2].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[2].finalLayout    = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

	VkAttachmentReference2 color_reference = {};
	color_reference.sType                  = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	color_reference.attachment             = 0;
	color_reference.layout                 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference2 depth_reference = {};
	depth_reference.sType                  = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	depth_reference.attachment             = 1;
	depth_reference.layout                 = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// @todo: comment
	VkAttachmentReference2 fragment_shading_rate_reference = {};
	fragment_shading_rate_reference.sType                  = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	fragment_shading_rate_reference.attachment             = 2;
	fragment_shading_rate_reference.layout                 = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

	VkFragmentShadingRateAttachmentInfoKHR fragment_shading_rate_attachment_info = {};
	fragment_shading_rate_attachment_info.sType                                  = VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
	fragment_shading_rate_attachment_info.pNext                                  = &fragment_shading_rate_reference;
	// @todo
	fragment_shading_rate_attachment_info.shadingRateAttachmentTexelSize.width  = width;
	fragment_shading_rate_attachment_info.shadingRateAttachmentTexelSize.height = height;

	// @todo: comment
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
	// @todo: comment
	subpass_description.pNext = &fragment_shading_rate_attachment_info;

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
	render_pass_create_info.sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
	render_pass_create_info.pAttachments            = attachments.data();
	render_pass_create_info.subpassCount            = 1;
	render_pass_create_info.pSubpasses              = &subpass_description;
	render_pass_create_info.dependencyCount         = static_cast<uint32_t>(dependencies.size());
	render_pass_create_info.pDependencies           = dependencies.data();

	VK_CHECK(vkCreateRenderPass2(device->get_handle(), &render_pass_create_info, nullptr, &render_pass));
}

/*
* Custom frame buffer setup
* @todo: proper comment
*/
void FragmentShadingRate::setup_framebuffer()
{
	VkImageView attachments[3];

	// Depth/Stencil attachment is the same for all frame buffers
	attachments[1] = depth_stencil.view;

	// @todo: comment
	attachments[2] = shading_rate_image.view;

	VkFramebufferCreateInfo framebuffer_create_info = {};
	framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_create_info.pNext                   = NULL;
	framebuffer_create_info.renderPass              = render_pass;
	framebuffer_create_info.attachmentCount         = 3;        // @todo: not sure on this
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

		{
			/*
				First pass: Render scene to offscreen framebuffer
			*/

			std::array<VkClearValue, 2> clear_values;
			clear_values[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
			clear_values[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

			VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
			render_pass_begin_info.renderPass               = offscreen.render_pass;
			render_pass_begin_info.framebuffer              = offscreen.framebuffer;
			render_pass_begin_info.renderArea.extent.width  = offscreen.width;
			render_pass_begin_info.renderArea.extent.height = offscreen.height;
			render_pass_begin_info.clearValueCount          = 2;
			render_pass_begin_info.pClearValues             = clear_values.data();

			vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkb::initializers::viewport((float) offscreen.width, (float) offscreen.height, 0.0f, 1.0f);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkb::initializers::rect2D(offscreen.width, offscreen.height, 0, 0);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = {0};

			if (display_skysphere)
			{
				vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skysphere);
				push_const_block.object_type = 0;
				vkCmdPushConstants(draw_cmd_buffers[i], pipeline_layouts.models, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_const_block), &push_const_block);
				vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.models, 0, 1, &descriptor_sets.skysphere, 0, NULL);
				draw_model(models.skysphere, draw_cmd_buffers[i]);
			}

			// Spheres
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.sphere);
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.models, 0, 1, &descriptor_sets.sphere, 0, NULL);
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
				vkCmdPushConstants(draw_cmd_buffers[i], pipeline_layouts.models, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_const_block), &push_const_block);
				draw_model(models.scene, draw_cmd_buffers[i]);
			}

			vkCmdEndRenderPass(draw_cmd_buffers[i]);
		}

		/*
			Second render pass: First bloom pass
		*/
		if (bloom)
		{
			VkClearValue clear_values[2];
			clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
			clear_values[1].depthStencil = {0.0f, 0};

			// Bloom filter
			VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
			render_pass_begin_info.framebuffer              = filter_pass.framebuffer;
			render_pass_begin_info.renderPass               = filter_pass.render_pass;
			render_pass_begin_info.clearValueCount          = 1;
			render_pass_begin_info.renderArea.extent.width  = filter_pass.width;
			render_pass_begin_info.renderArea.extent.height = filter_pass.height;
			render_pass_begin_info.pClearValues             = clear_values;

			vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkb::initializers::viewport((float) filter_pass.width, (float) filter_pass.height, 0.0f, 1.0f);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkb::initializers::rect2D(filter_pass.width, filter_pass.height, 0, 0);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.bloom_filter, 0, 1, &descriptor_sets.bloom_filter, 0, NULL);

			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.bloom[1]);
			vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

			vkCmdEndRenderPass(draw_cmd_buffers[i]);
		}

		/*
			Note: Synchronization between render passes is handled via sub pass dependencies.
		*/

		/*
			Third render pass: Scene rendering with applied second bloom pass (when enabled)
		*/
		{
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

			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.composition, 0, 1, &descriptor_sets.composition, 0, NULL);

			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.composition);
			vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

			// Bloom
			if (bloom)
			{
				vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.bloom[0]);
				vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);
			}

			draw_ui(draw_cmd_buffers[i]);

			vkCmdEndRenderPass(draw_cmd_buffers[i]);
		}

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void FragmentShadingRate::create_attachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment)
{
	VkImageAspectFlags aspect_mask = 0;
	VkImageLayout      image_layout;

	attachment->format = format;

	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		aspect_mask  = VK_IMAGE_ASPECT_COLOR_BIT;
		image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
		// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
		if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
		{
			aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	assert(aspect_mask > 0);

	VkImageCreateInfo image = vkb::initializers::image_create_info();
	image.imageType         = VK_IMAGE_TYPE_2D;
	image.format            = format;
	image.extent.width      = offscreen.width;
	image.extent.height     = offscreen.height;
	image.extent.depth      = 1;
	image.mipLevels         = 1;
	image.arrayLayers       = 1;
	image.samples           = VK_SAMPLE_COUNT_1_BIT;
	image.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image.usage             = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

	VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
	VkMemoryRequirements memory_requirements;

	VK_CHECK(vkCreateImage(get_device().get_handle(), &image, nullptr, &attachment->image));
	vkGetImageMemoryRequirements(get_device().get_handle(), attachment->image, &memory_requirements);
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &attachment->mem));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), attachment->image, attachment->mem, 0));

	VkImageViewCreateInfo image_view_create_info           = vkb::initializers::image_view_create_info();
	image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format                          = format;
	image_view_create_info.subresourceRange                = {};
	image_view_create_info.subresourceRange.aspectMask     = aspect_mask;
	image_view_create_info.subresourceRange.baseMipLevel   = 0;
	image_view_create_info.subresourceRange.levelCount     = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount     = 1;
	image_view_create_info.image                           = attachment->image;
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &image_view_create_info, nullptr, &attachment->view));
}

// Prepare a new framebuffer and attachments for offscreen rendering (G-Buffer)
void FragmentShadingRate::prepare_offscreen_buffer()
{
	{
		offscreen.width  = width;
		offscreen.height = height;

		// Color attachments

		create_attachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &offscreen.color[0]);
		create_attachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &offscreen.color[1]);
		// Depth attachment
		create_attachment(depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &offscreen.depth);

		// Set up separate renderpass with references to the colorand depth attachments
		std::array<VkAttachmentDescription, 3> attachment_descriptions = {};

		// Init attachment properties
		for (uint32_t i = 0; i < 3; ++i)
		{
			attachment_descriptions[i].samples        = VK_SAMPLE_COUNT_1_BIT;
			attachment_descriptions[i].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment_descriptions[i].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
			attachment_descriptions[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment_descriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			if (i == 2)
			{
				attachment_descriptions[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				attachment_descriptions[i].finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else
			{
				attachment_descriptions[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				attachment_descriptions[i].finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
		}

		// Formats
		attachment_descriptions[0].format = offscreen.color[0].format;
		attachment_descriptions[1].format = offscreen.color[1].format;
		attachment_descriptions[2].format = offscreen.depth.format;

		std::vector<VkAttachmentReference> color_references;
		color_references.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
		color_references.push_back({1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

		VkAttachmentReference depth_reference = {};
		depth_reference.attachment            = 2;
		depth_reference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass    = {};
		subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments       = color_references.data();
		subpass.colorAttachmentCount    = 2;
		subpass.pDepthStencilAttachment = &depth_reference;

		// Use subpass dependencies for attachment layout transitions
		std::array<VkSubpassDependency, 2> dependencies{};

		dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass    = 0;
		dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		dependencies[1].srcSubpass    = 0;
		dependencies[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VkRenderPassCreateInfo render_pass_create_info = {};
		render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_create_info.pAttachments           = attachment_descriptions.data();
		render_pass_create_info.attachmentCount        = static_cast<uint32_t>(attachment_descriptions.size());
		render_pass_create_info.subpassCount           = 1;
		render_pass_create_info.pSubpasses             = &subpass;
		render_pass_create_info.dependencyCount        = 2;
		render_pass_create_info.pDependencies          = dependencies.data();

		VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &offscreen.render_pass));

		std::array<VkImageView, 3> attachments;
		attachments[0] = offscreen.color[0].view;
		attachments[1] = offscreen.color[1].view;
		attachments[2] = offscreen.depth.view;

		VkFramebufferCreateInfo framebuffer_create_info = {};
		framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.pNext                   = NULL;
		framebuffer_create_info.renderPass              = offscreen.render_pass;
		framebuffer_create_info.pAttachments            = attachments.data();
		framebuffer_create_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
		framebuffer_create_info.width                   = offscreen.width;
		framebuffer_create_info.height                  = offscreen.height;
		framebuffer_create_info.layers                  = 1;
		VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_create_info, nullptr, &offscreen.framebuffer));

		// Create sampler to sample from the color attachments
		VkSamplerCreateInfo sampler = vkb::initializers::sampler_create_info();
		sampler.magFilter           = VK_FILTER_NEAREST;
		sampler.minFilter           = VK_FILTER_NEAREST;
		sampler.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV        = sampler.addressModeU;
		sampler.addressModeW        = sampler.addressModeU;
		sampler.mipLodBias          = 0.0f;
		sampler.maxAnisotropy       = 1.0f;
		sampler.minLod              = 0.0f;
		sampler.maxLod              = 1.0f;
		sampler.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler, nullptr, &offscreen.sampler));
	}

	// Bloom separable filter pass
	{
		filter_pass.width  = width;
		filter_pass.height = height;

		// Color attachments

		// Two color buffers
		create_attachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &filter_pass.color[0]);

		// Set up separate renderpass with references to the colorand depth attachments
		std::array<VkAttachmentDescription, 1> attachment_descriptions = {};

		// Init attachment properties
		attachment_descriptions[0].samples        = VK_SAMPLE_COUNT_1_BIT;
		attachment_descriptions[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment_descriptions[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachment_descriptions[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment_descriptions[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment_descriptions[0].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachment_descriptions[0].format         = filter_pass.color[0].format;

		std::vector<VkAttachmentReference> color_references;
		color_references.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments    = color_references.data();
		subpass.colorAttachmentCount = 1;

		// Use subpass dependencies for attachment layout transitions
		std::array<VkSubpassDependency, 2> dependencies{};

		dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass    = 0;
		dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		dependencies[1].srcSubpass    = 0;
		dependencies[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VkRenderPassCreateInfo render_pass_create_info = {};
		render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_create_info.pAttachments           = attachment_descriptions.data();
		render_pass_create_info.attachmentCount        = static_cast<uint32_t>(attachment_descriptions.size());
		render_pass_create_info.subpassCount           = 1;
		render_pass_create_info.pSubpasses             = &subpass;
		render_pass_create_info.dependencyCount        = 2;
		render_pass_create_info.pDependencies          = dependencies.data();

		VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &filter_pass.render_pass));

		std::array<VkImageView, 1> attachments;
		attachments[0] = filter_pass.color[0].view;

		VkFramebufferCreateInfo framebuffer_create_info = {};
		framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.pNext                   = NULL;
		framebuffer_create_info.renderPass              = filter_pass.render_pass;
		framebuffer_create_info.pAttachments            = attachments.data();
		framebuffer_create_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
		framebuffer_create_info.width                   = filter_pass.width;
		framebuffer_create_info.height                  = filter_pass.height;
		framebuffer_create_info.layers                  = 1;
		VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_create_info, nullptr, &filter_pass.framebuffer));

		// Create sampler to sample from the color attachments
		VkSamplerCreateInfo sampler = vkb::initializers::sampler_create_info();
		sampler.magFilter           = VK_FILTER_NEAREST;
		sampler.minFilter           = VK_FILTER_NEAREST;
		sampler.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV        = sampler.addressModeU;
		sampler.addressModeW        = sampler.addressModeU;
		sampler.mipLodBias          = 0.0f;
		sampler.maxAnisotropy       = 1.0f;
		sampler.minLod              = 0.0f;
		sampler.maxLod              = 1.0f;
		sampler.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler, nullptr, &filter_pass.sampler));
	}
}

void FragmentShadingRate::load_assets()
{
	models.skysphere   = load_model("scenes/geosphere.gltf");
	textures.skysphere = load_texture("textures/skysphere_rgba.ktx");
	models.scene       = load_model("scenes/geosphere.gltf");
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
	// Object rendering (into offscreen buffer)
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.models));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layouts.models,
	        1);

	// Pass object offset and color via push constant
	VkPushConstantRange push_constant_range            = vkb::initializers::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(push_const_block), 0);
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.models));

	// Bloom filter
	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	};

	descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.bloom_filter));

	pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layouts.bloom_filter, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.bloom_filter));

	// G-Buffer composition
	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	};

	descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.composition));

	pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layouts.composition, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.composition));
}

void FragmentShadingRate::setup_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layouts.models,
	        1);

	// Sphere model object descriptor set
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.sphere));

	VkDescriptorBufferInfo            matrix_buffer_descriptor     = create_descriptor(*uniform_buffers.matrices);
	VkDescriptorImageInfo             environment_image_descriptor = create_descriptor(textures.skysphere);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets        = {
        vkb::initializers::write_descriptor_set(descriptor_sets.sphere, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.sphere, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor),
    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);

	// Sky sphere descriptor set
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.skysphere));

	matrix_buffer_descriptor     = create_descriptor(*uniform_buffers.matrices);
	environment_image_descriptor = create_descriptor(textures.skysphere);
	write_descriptor_sets        = {
        vkb::initializers::write_descriptor_set(descriptor_sets.skysphere, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.skysphere, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor),
    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);

	// Bloom filter
	alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layouts.bloom_filter, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.bloom_filter));

	std::vector<VkDescriptorImageInfo> color_descriptors = {
	    vkb::initializers::descriptor_image_info(offscreen.sampler, offscreen.color[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	    vkb::initializers::descriptor_image_info(offscreen.sampler, offscreen.color[1].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	};

	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_sets.bloom_filter, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &color_descriptors[0]),
	    vkb::initializers::write_descriptor_set(descriptor_sets.bloom_filter, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &color_descriptors[1]),
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);

	// Composition descriptor set
	alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layouts.composition, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.composition));

	color_descriptors = {
	    vkb::initializers::descriptor_image_info(offscreen.sampler, offscreen.color[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	    vkb::initializers::descriptor_image_info(offscreen.sampler, filter_pass.color[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	};

	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_sets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &color_descriptors[0]),
	    vkb::initializers::write_descriptor_set(descriptor_sets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &color_descriptors[1]),
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
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
	        pipeline_layouts.models,
	        render_pass,
	        0);

	std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_states = {
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
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

	// Full screen pipelines

	// Empty vertex input state, full screen triangles are generated by the vertex shader
	VkPipelineVertexInputStateCreateInfo empty_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	pipeline_create_info.pVertexInputState                 = &empty_input_state;

	// Final fullscreen composition pass pipeline
	shader_stages[0]                  = load_shader("debug_utils/composition.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]                  = load_shader("debug_utils/composition.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	pipeline_create_info.layout       = pipeline_layouts.composition;
	pipeline_create_info.renderPass   = render_pass;
	rasterization_state.cullMode      = VK_CULL_MODE_FRONT_BIT;
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments    = blend_attachment_states.data();
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.composition));

	// Bloom pass
	shader_stages[0]                           = load_shader("debug_utils/bloom.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]                           = load_shader("debug_utils/bloom.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	color_blend_state.pAttachments             = &blend_attachment_state;
	blend_attachment_state.colorWriteMask      = 0xF;
	blend_attachment_state.blendEnable         = VK_TRUE;
	blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;

	// Set constant parameters via specialization constants
	VkSpecializationInfo                    specialization_info;
	std::array<VkSpecializationMapEntry, 1> specialization_map_entries;
	specialization_map_entries[0]        = vkb::initializers::specialization_map_entry(0, 0, sizeof(uint32_t));
	uint32_t dir                         = 1;
	specialization_info                  = vkb::initializers::specialization_info(1, specialization_map_entries.data(), sizeof(dir), &dir);
	shader_stages[1].pSpecializationInfo = &specialization_info;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.bloom[0]));

	// Second blur pass (into separate framebuffer)
	pipeline_create_info.renderPass = filter_pass.render_pass;
	dir                             = 0;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.bloom[1]));
	shader_stages[1].pSpecializationInfo = nullptr;

	// Object rendering pipelines
	rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;

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

	pipeline_create_info.pVertexInputState = &vertex_input_state;

	// skysphere pipeline (background cube)
	blend_attachment_state.blendEnable = VK_FALSE;
	pipeline_create_info.layout        = pipeline_layouts.models;
	pipeline_create_info.renderPass    = offscreen.render_pass;
	color_blend_state.attachmentCount  = 2;
	color_blend_state.pAttachments     = blend_attachment_states.data();

	shader_stages[0] = load_shader("debug_utils/gbuffer.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("debug_utils/gbuffer.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.skysphere));

	// Enable depth test and write
	depth_stencil_state.depthWriteEnable = VK_TRUE;
	depth_stencil_state.depthTestEnable  = VK_TRUE;
	// Flip cull mode
	rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.sphere));
}

// Prepare and initialize uniform buffer containing shader uniforms
void FragmentShadingRate::prepare_uniform_buffers()
{
	// Matrices vertex shader uniform buffer
	uniform_buffers.matrices = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                               sizeof(ubo_vs),
	                                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                               VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void FragmentShadingRate::update_uniform_buffers()
{
	ubo_vs.projection          = camera.matrices.perspective;
	ubo_vs.modelview           = camera.matrices.view * glm::mat4(1.0f);
	ubo_vs.skysphere_modelview = camera.matrices.view;
	uniform_buffers.matrices->convert_and_update(ubo_vs);
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

	// Query the fragment shading rate properties of the current implementation, we will need them later on
	physical_device_fragment_shading_rate_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 device_properties{};
	device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	device_properties.pNext = &physical_device_fragment_shading_rate_properties;
	vkGetPhysicalDeviceProperties2(get_device().get_gpu().get_handle(), &device_properties);

	// Get all available shading rates
	uint32_t fragment_shading_rate_count;
	vkGetPhysicalDeviceFragmentShadingRatesKHR(get_device().get_gpu().get_handle(), &fragment_shading_rate_count, nullptr);
	if (fragment_shading_rate_count > 0)
	{
		fragment_shading_rates.resize(fragment_shading_rate_count);
		vkGetPhysicalDeviceFragmentShadingRatesKHR(get_device().get_gpu().get_handle(), &fragment_shading_rate_count, fragment_shading_rates.data());
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -6.0f));
	camera.set_rotation(glm::vec3(0.0f, 180.0f, 0.0f));

	// Note: Using Revsered depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, (float) width / (float) height, 256.0f, 0.1f);

	load_assets();
	create_shading_rate_attachment();
	prepare_uniform_buffers();
	prepare_offscreen_buffer();
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
		if (drawer.checkbox("Bloom", &bloom))
		{
			build_command_buffers();
		}
		if (drawer.checkbox("skysphere", &display_skysphere))
		{
			build_command_buffers();
		}
	}
}

void FragmentShadingRate::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
}

std::unique_ptr<vkb::VulkanSample> create_fragment_shading_rate()
{
	return std::make_unique<FragmentShadingRate>();
}