/* Copyright (c) 2019-2020, Sascha Willems
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
 * Conservative rasterization
 *
 * Note: Requires a device that supports the VK_EXT_conservative_rasterization extension
 *
 * Uses an offscreen buffer with lower resolution to demonstrate the effect of conservative rasterization
 */

#include "conservative_rasterization.h"

#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM
#define ZOOM_FACTOR 16

ConservativeRasterization::ConservativeRasterization()
: pipelines()
, pipeline_layouts()
, descriptor_set_layouts()
, descriptor_sets()
, offscreen_pass()
{
	title = "Conservative rasterization";

	// Reading device properties of conservative rasterization requires VK_KHR_get_physical_device_properties2 to be enabled
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	// Enable extension required for conservative rasterization
	add_device_extension(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);
}

ConservativeRasterization::~ConservativeRasterization()
{
	if (device)
	{
		vkDestroyImageView(device->get_handle(), offscreen_pass.color.view, nullptr);
		vkDestroyImage(device->get_handle(), offscreen_pass.color.image, nullptr);
		vkFreeMemory(device->get_handle(), offscreen_pass.color.mem, nullptr);
		vkDestroyImageView(device->get_handle(), offscreen_pass.depth.view, nullptr);
		vkDestroyImage(device->get_handle(), offscreen_pass.depth.image, nullptr);
		vkFreeMemory(device->get_handle(), offscreen_pass.depth.mem, nullptr);

		vkDestroyRenderPass(device->get_handle(), offscreen_pass.render_pass, nullptr);
		vkDestroySampler(device->get_handle(), offscreen_pass.sampler, nullptr);
		vkDestroyFramebuffer(device->get_handle(), offscreen_pass.framebuffer, nullptr);

		vkDestroyPipeline(device->get_handle(), pipelines.triangle, nullptr);
		vkDestroyPipeline(device->get_handle(), pipelines.triangle_overlay, nullptr);
		vkDestroyPipeline(device->get_handle(), pipelines.triangle_conservative_raster, nullptr);
		vkDestroyPipeline(device->get_handle(), pipelines.fullscreen, nullptr);

		vkDestroyPipelineLayout(device->get_handle(), pipeline_layouts.fullscreen, nullptr);
		vkDestroyPipelineLayout(device->get_handle(), pipeline_layouts.scene, nullptr);

		vkDestroyDescriptorSetLayout(device->get_handle(), descriptor_set_layouts.scene, nullptr);
		vkDestroyDescriptorSetLayout(device->get_handle(), descriptor_set_layouts.fullscreen, nullptr);
	}

	uniform_buffers.scene.reset();
	triangle.vertices.reset();
	triangle.indices.reset();
}

void ConservativeRasterization::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	gpu.get_mutable_requested_features().fillModeNonSolid = gpu.get_features().fillModeNonSolid;
	gpu.get_mutable_requested_features().wideLines        = gpu.get_features().wideLines;
}

// Setup offscreen framebuffer, attachments and render passes for lower resolution rendering of the scene
void ConservativeRasterization::prepare_offscreen()
{
	offscreen_pass.width  = static_cast<int>(width) / ZOOM_FACTOR;
	offscreen_pass.height = static_cast<int>(height) / ZOOM_FACTOR;

	// Find a suitable depth format
	VkFormat framebuffer_depth_format = vkb::get_suitable_depth_format(get_device().get_gpu().get_handle());

	// Color attachment
	VkImageCreateInfo image = vkb::initializers::image_create_info();
	image.imageType         = VK_IMAGE_TYPE_2D;
	image.format            = FB_COLOR_FORMAT;
	image.extent.width      = offscreen_pass.width;
	image.extent.height     = offscreen_pass.height;
	image.extent.depth      = 1;
	image.mipLevels         = 1;
	image.arrayLayers       = 1;
	image.samples           = VK_SAMPLE_COUNT_1_BIT;
	image.tiling            = VK_IMAGE_TILING_OPTIMAL;
	// We will sample directly from the color attachment
	image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	VkMemoryAllocateInfo memory_allocation_info = vkb::initializers::memory_allocate_info();
	VkMemoryRequirements memory_requirements;

	VK_CHECK(vkCreateImage(get_device().get_handle(), &image, nullptr, &offscreen_pass.color.image));
	vkGetImageMemoryRequirements(get_device().get_handle(), offscreen_pass.color.image, &memory_requirements);
	memory_allocation_info.allocationSize  = memory_requirements.size;
	memory_allocation_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocation_info, nullptr, &offscreen_pass.color.mem));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), offscreen_pass.color.image, offscreen_pass.color.mem, 0));

	VkImageViewCreateInfo color_image_view           = vkb::initializers::image_view_create_info();
	color_image_view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	color_image_view.format                          = FB_COLOR_FORMAT;
	color_image_view.subresourceRange                = {};
	color_image_view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	color_image_view.subresourceRange.baseMipLevel   = 0;
	color_image_view.subresourceRange.levelCount     = 1;
	color_image_view.subresourceRange.baseArrayLayer = 0;
	color_image_view.subresourceRange.layerCount     = 1;
	color_image_view.image                           = offscreen_pass.color.image;
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &color_image_view, nullptr, &offscreen_pass.color.view));

	// Create sampler to sample from the attachment in the fragment shader
	VkSamplerCreateInfo sampler_info = vkb::initializers::sampler_create_info();
	sampler_info.magFilter           = VK_FILTER_NEAREST;
	sampler_info.minFilter           = VK_FILTER_NEAREST;
	sampler_info.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeV        = sampler_info.addressModeU;
	sampler_info.addressModeW        = sampler_info.addressModeU;
	sampler_info.mipLodBias          = 0.0f;
	sampler_info.maxAnisotropy       = 1.0f;
	sampler_info.minLod              = 0.0f;
	sampler_info.maxLod              = 1.0f;
	sampler_info.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_info, nullptr, &offscreen_pass.sampler));

	// Depth attachment
	image.format = framebuffer_depth_format;
	image.usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VK_CHECK(vkCreateImage(get_device().get_handle(), &image, nullptr, &offscreen_pass.depth.image));
	vkGetImageMemoryRequirements(get_device().get_handle(), offscreen_pass.depth.image, &memory_requirements);
	memory_allocation_info.allocationSize  = memory_requirements.size;
	memory_allocation_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocation_info, nullptr, &offscreen_pass.depth.mem));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), offscreen_pass.depth.image, offscreen_pass.depth.mem, 0));

	// The depth format we get for the current device may not include a stencil part, which affects the aspect mask used by the image view
	const VkImageAspectFlags aspect_mask = vkb::is_depth_only_format(framebuffer_depth_format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

	VkImageViewCreateInfo depth_stencil_view           = vkb::initializers::image_view_create_info();
	depth_stencil_view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	depth_stencil_view.format                          = framebuffer_depth_format;
	depth_stencil_view.flags                           = 0;
	depth_stencil_view.subresourceRange                = {};
	depth_stencil_view.subresourceRange.aspectMask     = aspect_mask;
	depth_stencil_view.subresourceRange.baseMipLevel   = 0;
	depth_stencil_view.subresourceRange.levelCount     = 1;
	depth_stencil_view.subresourceRange.baseArrayLayer = 0;
	depth_stencil_view.subresourceRange.layerCount     = 1;
	depth_stencil_view.image                           = offscreen_pass.depth.image;
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &depth_stencil_view, nullptr, &offscreen_pass.depth.view));

	// Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering

	std::array<VkAttachmentDescription, 2> attachment_descriptions = {};
	// Color attachment
	attachment_descriptions[0].format         = FB_COLOR_FORMAT;
	attachment_descriptions[0].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachment_descriptions[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_descriptions[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_descriptions[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descriptions[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_descriptions[0].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	// Depth attachment
	attachment_descriptions[1].format         = framebuffer_depth_format;
	attachment_descriptions[1].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachment_descriptions[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_descriptions[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descriptions[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_descriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descriptions[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_descriptions[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_reference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
	VkAttachmentReference depth_reference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

	VkSubpassDescription subpass_description    = {};
	subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.colorAttachmentCount    = 1;
	subpass_description.pColorAttachments       = &color_reference;
	subpass_description.pDepthStencilAttachment = &depth_reference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies{};

	dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass      = 0;
	dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass      = 0;
	dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create the actual renderpass
	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount        = static_cast<uint32_t>(attachment_descriptions.size());
	render_pass_create_info.pAttachments           = attachment_descriptions.data();
	render_pass_create_info.subpassCount           = 1;
	render_pass_create_info.pSubpasses             = &subpass_description;
	render_pass_create_info.dependencyCount        = static_cast<uint32_t>(dependencies.size());
	render_pass_create_info.pDependencies          = dependencies.data();

	VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &offscreen_pass.render_pass));

	VkImageView attachments[2];
	attachments[0] = offscreen_pass.color.view;
	attachments[1] = offscreen_pass.depth.view;

	VkFramebufferCreateInfo framebuffer_create_info = vkb::initializers::framebuffer_create_info();
	framebuffer_create_info.renderPass              = offscreen_pass.render_pass;
	framebuffer_create_info.attachmentCount         = 2;
	framebuffer_create_info.pAttachments            = attachments;
	framebuffer_create_info.width                   = offscreen_pass.width;
	framebuffer_create_info.height                  = offscreen_pass.height;
	framebuffer_create_info.layers                  = 1;

	VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_create_info, nullptr, &offscreen_pass.framebuffer));

	// Fill a descriptor for later use in a descriptor set
	offscreen_pass.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	offscreen_pass.descriptor.imageView   = offscreen_pass.color.view;
	offscreen_pass.descriptor.sampler     = offscreen_pass.sampler;
}

void ConservativeRasterization::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		// First render pass: Render a low res triangle to an offscreen framebuffer to use for visualization in second pass
		{
			VkClearValue clear_values[2];
			clear_values[0].color        = {{0.25f, 0.25f, 0.25f, 0.0f}};
			clear_values[1].depthStencil = {0.0f, 0};

			VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
			render_pass_begin_info.renderPass               = offscreen_pass.render_pass;
			render_pass_begin_info.framebuffer              = offscreen_pass.framebuffer;
			render_pass_begin_info.renderArea.extent.width  = offscreen_pass.width;
			render_pass_begin_info.renderArea.extent.height = offscreen_pass.height;
			render_pass_begin_info.clearValueCount          = 2;
			render_pass_begin_info.pClearValues             = clear_values;

			VkViewport viewport = vkb::initializers::viewport((float) offscreen_pass.width, (float) offscreen_pass.height, 0.0f, 1.0f);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkb::initializers::rect2D(offscreen_pass.width, offscreen_pass.height, 0, 0);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

			vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.scene, 0, 1, &descriptor_sets.scene, 0, nullptr);
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, conservative_raster_enabled ? pipelines.triangle_conservative_raster : pipelines.triangle);

			VkDeviceSize offsets[1] = {0};
			vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, triangle.vertices->get(), offsets);
			vkCmdBindIndexBuffer(draw_cmd_buffers[i], triangle.indices->get_handle(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);
			vkCmdDrawIndexed(draw_cmd_buffers[i], triangle.index_count, 1, 0, 0, 0);

			vkCmdEndRenderPass(draw_cmd_buffers[i]);
		}

		// Note: Explicit synchronization is not required between the render pass, as this is done implicit via sub pass dependencies

		// Second render pass: Render scene with conservative rasterization
		{
			VkClearValue clear_values[2];
			clear_values[0].color        = {{0.25f, 0.25f, 0.25f, 0.25f}};
			clear_values[1].depthStencil = {0.0f, 0};

			VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
			render_pass_begin_info.framebuffer              = framebuffers[i];
			render_pass_begin_info.renderPass               = render_pass;
			render_pass_begin_info.renderArea.offset.x      = 0;
			render_pass_begin_info.renderArea.offset.y      = 0;
			render_pass_begin_info.renderArea.extent.width  = width;
			render_pass_begin_info.renderArea.extent.height = height;
			render_pass_begin_info.clearValueCount          = 2;
			render_pass_begin_info.pClearValues             = clear_values;

			vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
			VkViewport viewport = vkb::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);
			VkRect2D scissor = vkb::initializers::rect2D(static_cast<int>(width), static_cast<int>(height), 0, 0);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

			// Low-res triangle from offscreen framebuffer
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.fullscreen);
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.fullscreen, 0, 1, &descriptor_sets.fullscreen, 0, nullptr);
			vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

			// Overlay actual triangle
			VkDeviceSize offsets[1] = {0};
			vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, triangle.vertices->get(), offsets);
			vkCmdBindIndexBuffer(draw_cmd_buffers[i], triangle.indices->get_handle(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.triangle_overlay);
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.scene, 0, 1, &descriptor_sets.scene, 0, nullptr);
			vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

			draw_ui(draw_cmd_buffers[i]);

			vkCmdEndRenderPass(draw_cmd_buffers[i]);
		}

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void ConservativeRasterization::load_assets()
{
	// Create a single triangle
	struct Vertex
	{
		float position[3];
		float color[3];
	};

	std::vector<Vertex> vertex_buffer = {
	    {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
	    {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
	    {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
	uint32_t              vertex_buffer_size = static_cast<uint32_t>(vertex_buffer.size()) * sizeof(Vertex);
	std::vector<uint32_t> index_buffer       = {0, 1, 2};
	triangle.index_count                     = static_cast<uint32_t>(index_buffer.size());
	uint32_t index_buffer_size               = triangle.index_count * sizeof(uint32_t);

	// Host visible source buffers (staging)
	vkb::core::Buffer vertex_staging_buffer{get_device(), vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY};
	vertex_staging_buffer.update(vertex_buffer.data(), vertex_buffer_size);

	vkb::core::Buffer index_staging_buffer{get_device(), index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY};
	index_staging_buffer.update(index_buffer.data(), index_buffer_size);

	// Device local destination buffers
	triangle.vertices = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                        vertex_buffer_size,
	                                                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                        VMA_MEMORY_USAGE_GPU_ONLY);

	triangle.indices = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                       index_buffer_size,
	                                                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                       VMA_MEMORY_USAGE_GPU_ONLY);

	// Copy from host to device
	get_device().copy_buffer(vertex_staging_buffer, *triangle.vertices, queue);
	get_device().copy_buffer(index_staging_buffer, *triangle.indices, queue);
}

void ConservativeRasterization::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)};
	VkDescriptorPoolCreateInfo descriptor_pool_info =
	    vkb::initializers::descriptor_pool_create_info(pool_sizes, 2);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_info, nullptr, &descriptor_pool));
}

void ConservativeRasterization::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings;
	VkDescriptorSetLayoutCreateInfo           descriptor_layout;
	VkPipelineLayoutCreateInfo                pipeline_layout_create_info;

	// Scene rendering
	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),                  // Binding 0: Vertex shader uniform buffer
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),        // Binding 1: Fragment shader image sampler
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2)                 // Binding 2: Fragment shader uniform buffer
	};
	descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layouts.scene));
	pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layouts.scene, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.scene));

	// Fullscreen pass
	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),                 // Binding 0: Vertex shader uniform buffer
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)        // Binding 1: Fragment shader image sampler
	};
	descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layouts.fullscreen));
	pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layouts.fullscreen, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.fullscreen));
}

void ConservativeRasterization::setup_descriptor_set()
{
	VkDescriptorSetAllocateInfo descriptor_set_allocate_info;

	// Scene rendering
	descriptor_set_allocate_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layouts.scene, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info, &descriptor_sets.scene));
	VkDescriptorBufferInfo            scene_buffer_descriptor         = create_descriptor(*uniform_buffers.scene);
	std::vector<VkWriteDescriptorSet> offscreen_write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_sets.scene, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &scene_buffer_descriptor),
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(offscreen_write_descriptor_sets.size()), offscreen_write_descriptor_sets.data(), 0, nullptr);

	// Fullscreen pass
	descriptor_set_allocate_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layouts.fullscreen, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info, &descriptor_sets.fullscreen));
	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_sets.fullscreen, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &offscreen_pass.descriptor),
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void ConservativeRasterization::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

	// Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector<VkDynamicState> dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables);

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(pipeline_layouts.fullscreen, render_pass, 0);

	// Conservative rasterization setup

	// Get device properties for conservative rasterization
	// Requires VK_KHR_get_physical_device_properties2 and manual function pointer creation
	auto _vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(instance->get_handle(), "vkGetPhysicalDeviceProperties2KHR"));
	assert(_vkGetPhysicalDeviceProperties2KHR);
	VkPhysicalDeviceProperties2KHR device_properties{};
	conservative_raster_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
	device_properties.sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
	device_properties.pNext              = &conservative_raster_properties;
	_vkGetPhysicalDeviceProperties2KHR(get_device().get_gpu().get_handle(), &device_properties);

	// Vertex bindings and attributes
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                        // Location 0: Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),        // Location 1: Color
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.stageCount          = vkb::to_u32(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();

	// Full screen pass
	shader_stages[0] = load_shader("conservative_rasterization/fullscreen.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("conservative_rasterization/fullscreen.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	// Empty vertex input state (full screen triangle generated in vertex shader)
	VkPipelineVertexInputStateCreateInfo empty_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	pipeline_create_info.pVertexInputState                 = &empty_input_state;
	pipeline_create_info.layout                            = pipeline_layouts.fullscreen;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.fullscreen));

	pipeline_create_info.pVertexInputState = &vertex_input_state;
	pipeline_create_info.layout            = pipeline_layouts.scene;

	// Original triangle outline
	// TODO: Check support for lines
	rasterization_state.lineWidth   = 2.0f;
	rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;
	shader_stages[0]                = load_shader("conservative_rasterization/triangle.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]                = load_shader("conservative_rasterization/triangleoverlay.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.triangle_overlay));

	pipeline_create_info.renderPass = offscreen_pass.render_pass;

	// Triangle rendering
	rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
	shader_stages[0]                = load_shader("conservative_rasterization/triangle.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]                = load_shader("conservative_rasterization/triangle.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Basic pipeline
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.triangle));

	// Pipeline with conservative rasterization enabled
	VkPipelineRasterizationConservativeStateCreateInfoEXT conservative_rasterization_state{};
	conservative_rasterization_state.sType                            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
	conservative_rasterization_state.conservativeRasterizationMode    = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
	conservative_rasterization_state.extraPrimitiveOverestimationSize = conservative_raster_properties.maxExtraPrimitiveOverestimationSize;

	// Conservative rasterization state has to be chained into the pipeline rasterization state create info structure
	rasterization_state.pNext = &conservative_rasterization_state;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.triangle_conservative_raster));
}

// Prepare and initialize uniform buffer containing shader uniforms
void ConservativeRasterization::prepare_uniform_buffers()
{
	uniform_buffers.scene = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                            sizeof(ubo_scene),
	                                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                            VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers_scene();
}

void ConservativeRasterization::update_uniform_buffers_scene()
{
	ubo_scene.projection = camera.matrices.perspective;
	ubo_scene.model      = camera.matrices.view;
	uniform_buffers.scene->convert_and_update(ubo_scene);
}

void ConservativeRasterization::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

bool ConservativeRasterization::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	// Note: Using Reversed depth-buffer for increased precision, so Z-near and Z-far are flipped
	camera.type = vkb::CameraType::LookAt;
	camera.set_perspective(60.0f, (float) width / (float) height, 512.0f, 0.1f);
	camera.set_rotation(glm::vec3(0.0f));
	camera.set_translation(glm::vec3(0.0f, 0.0f, -2.0f));

	load_assets();
	prepare_offscreen();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();
	prepared = true;
	return true;
}

void ConservativeRasterization::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
	if (camera.updated)
		update_uniform_buffers_scene();
}

void ConservativeRasterization::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (vkb::Drawer::header("Settings"))
	{
		if (drawer.checkbox("Conservative rasterization", &conservative_raster_enabled))
		{
			build_command_buffers();
		}
	}
	if (vkb::Drawer::header("Device properties"))
	{
		vkb::Drawer::text("maxExtraPrimitiveOverestimationSize: %f", conservative_raster_properties.maxExtraPrimitiveOverestimationSize);
		vkb::Drawer::text("extraPrimitiveOverestimationSizeGranularity: %f", conservative_raster_properties.extraPrimitiveOverestimationSizeGranularity);
		vkb::Drawer::text("primitiveUnderestimation:  %s", conservative_raster_properties.primitiveUnderestimation ? "yes" : "no");
		vkb::Drawer::text("conservativePointAndLineRasterization:  %s", conservative_raster_properties.conservativePointAndLineRasterization ? "yes" : "no");
		vkb::Drawer::text("degenerateTrianglesRasterized: %s", conservative_raster_properties.degenerateTrianglesRasterized ? "yes" : "no");
		vkb::Drawer::text("degenerateLinesRasterized: %s", conservative_raster_properties.degenerateLinesRasterized ? "yes" : "no");
		vkb::Drawer::text("fullyCoveredFragmentShaderInputVariable: %s", conservative_raster_properties.fullyCoveredFragmentShaderInputVariable ? "yes" : "no");
		vkb::Drawer::text("conservativeRasterizationPostDepthCoverage: %s", conservative_raster_properties.conservativeRasterizationPostDepthCoverage ? "yes" : "no");
	}
}

std::unique_ptr<vkb::VulkanSample> create_conservative_rasterization()
{
	return std::make_unique<ConservativeRasterization>();
}
