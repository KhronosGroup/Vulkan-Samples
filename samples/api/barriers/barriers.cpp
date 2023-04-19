/* Copyright (c) 2023, Mobica Limited
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
 * Barriers demonstration
 */

#include "barriers.h"

#include "scene_graph/components/sub_mesh.h"

Barriers::Barriers()
{
	// Dynamic Rendering is a Vulkan 1.2 extension
	set_api_version(VK_API_VERSION_1_2);
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

	title = "Barriers demonstration";
}

Barriers::~Barriers()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), pipelines.skybox, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.reflect, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.composition, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.bloom[0], nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.bloom[1], nullptr);

		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.models, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.composition, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.bloom_filter, nullptr);

		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.models, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.composition, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.bloom_filter, nullptr);

		vkDestroySampler(get_device().get_handle(), offscreen.sampler, nullptr);
		vkDestroySampler(get_device().get_handle(), filter_pass.sampler, nullptr);

		offscreen.depth.destroy(get_device().get_handle());
		offscreen.color[0].destroy(get_device().get_handle());
		offscreen.color[1].destroy(get_device().get_handle());

		filter_pass.color[0].destroy(get_device().get_handle());

		vkDestroySampler(get_device().get_handle(), textures.envmap.sampler, nullptr);
	}
}

void Barriers::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	auto &requested_dynamic_rendering            = gpu.request_extension_features<VkPhysicalDeviceDynamicRenderingFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR);
	requested_dynamic_rendering.dynamicRendering = VK_TRUE;

	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

void Barriers::conditional_image_memory_barrier(const std::string &title, const std::string &tooltip,
                                                VkCommandBuffer         command_buffer,
                                                VkImage                 image,
                                                VkAccessFlags           src_access_mask,
                                                VkAccessFlags           dst_access_mask,
                                                VkImageLayout           old_layout,
                                                VkImageLayout           new_layout,
                                                VkPipelineStageFlags    src_stage_mask,
                                                VkPipelineStageFlags    dst_stage_mask,
                                                VkImageSubresourceRange subresource_range)
{
	auto title_matches   = [&title](const TestBarrierInfo &info) { return info.title == title; };
	auto barrier_info_it = std::find_if(begin(test_barriers), end(test_barriers), title_matches);

	if (barrier_info_it == test_barriers.end())
	{
		TestBarrierInfo info = {title, tooltip, src_access_mask, dst_access_mask, old_layout, new_layout, src_stage_mask, dst_stage_mask};
		barrier_info_it      = test_barriers.insert(test_barriers.end(), info);
	}
	TestBarrierInfo info = *barrier_info_it;
	if (info.enable == false)
	{
		return;
	}
	if (info.enable_access_mask == false)
	{
		info.src_access_mask = 0;
		info.dst_access_mask = 0;
	}
	if (info.enable_stage_mask == false)
	{
		info.src_stage_mask = 0;
		info.dst_stage_mask = 0;
	}
	vkb::insert_image_memory_barrier(command_buffer, image, info.src_access_mask, info.dst_access_mask, info.old_layout, info.new_layout, info.src_stage_mask, info.dst_stage_mask, subresource_range);
}

void Barriers::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkImageSubresourceRange range{};
	range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel   = 0;
	range.levelCount     = VK_REMAINING_MIP_LEVELS;
	range.baseArrayLayer = 0;
	range.layerCount     = VK_REMAINING_ARRAY_LAYERS;

	VkImageSubresourceRange depth_range{range};
	depth_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		{
			/*
			    First pass: Render scene to offscreen framebuffer
			*/

			std::array<VkClearValue, 3> clear_values;
			clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
			clear_values[1].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
			clear_values[2].depthStencil = {0.0f, 0};

			conditional_image_memory_barrier("OfImg0 Init", "Offscreen image 0 initialization",
			                                 draw_cmd_buffers[i],
			                                 offscreen.color[0].image,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_IMAGE_LAYOUT_UNDEFINED,
			                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, range);

			conditional_image_memory_barrier("OfImg1 Init", "Offscreen image 1 initialization",
			                                 draw_cmd_buffers[i],
			                                 offscreen.color[1].image,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_IMAGE_LAYOUT_UNDEFINED,
			                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, range);

			conditional_image_memory_barrier("OfDepth Init", "Offscreen depth initialization",
			                                 draw_cmd_buffers[i],
			                                 offscreen.depth.image,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_IMAGE_LAYOUT_UNDEFINED,
			                                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, depth_range);

			VkRenderingAttachmentInfoKHR color_attachment_info[2];
			color_attachment_info[0]             = vkb::initializers::rendering_attachment_info();
			color_attachment_info[0].imageView   = offscreen.color[0].view;        // color_attachment.image_view;
			color_attachment_info[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			color_attachment_info[0].resolveMode = VK_RESOLVE_MODE_NONE;
			color_attachment_info[0].loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
			color_attachment_info[0].storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
			color_attachment_info[0].clearValue  = clear_values[0];

			color_attachment_info[1]             = vkb::initializers::rendering_attachment_info();
			color_attachment_info[1].imageView   = offscreen.color[1].view;        // color_attachment.image_view;
			color_attachment_info[1].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			color_attachment_info[1].resolveMode = VK_RESOLVE_MODE_NONE;
			color_attachment_info[1].loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
			color_attachment_info[1].storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
			color_attachment_info[1].clearValue  = clear_values[1];

			VkRenderingAttachmentInfoKHR depth_attachment_info = vkb::initializers::rendering_attachment_info();
			depth_attachment_info.imageView                    = offscreen.depth.view;
			depth_attachment_info.imageLayout                  = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			depth_attachment_info.resolveMode                  = VK_RESOLVE_MODE_NONE;
			depth_attachment_info.loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depth_attachment_info.storeOp                      = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depth_attachment_info.clearValue                   = clear_values[2];

			auto render_area             = VkRect2D{VkOffset2D{}, VkExtent2D{(uint32_t) offscreen.width, (uint32_t) offscreen.height}};
			auto render_info             = vkb::initializers::rendering_info(render_area, 2, color_attachment_info);
			render_info.layerCount       = 1;
			render_info.pDepthAttachment = &depth_attachment_info;
			if (!vkb::is_depth_only_format(depth_format))
			{
				render_info.pStencilAttachment = &depth_attachment_info;
			}

			vkCmdBeginRenderingKHR(draw_cmd_buffers[i], &render_info);

			VkViewport viewport = vkb::initializers::viewport(static_cast<float>(offscreen.width), static_cast<float>(offscreen.height), 0.0f, 1.0f);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkb::initializers::rect2D(offscreen.width, offscreen.height, 0, 0);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = {0};

			// Skybox
			if (display_skybox)
			{
				vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skybox);
				vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.models, 0, 1, &descriptor_sets.skybox, 0, NULL);

				draw_model(models.skybox, draw_cmd_buffers[i]);
			}

			// 3D object
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.reflect);
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.models, 0, 1, &descriptor_sets.object, 0, NULL);

			draw_model(models.objects[models.object_index], draw_cmd_buffers[i]);

			vkCmdEndRenderingKHR(draw_cmd_buffers[i]);

			conditional_image_memory_barrier("OfImg0 to RO", "Offscreen image 0 transition to read only",
			                                 draw_cmd_buffers[i],
			                                 offscreen.color[0].image,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			                                 VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, range);

			conditional_image_memory_barrier("OfImg1 to RO", "Offscreen image 1 transition to read only",
			                                 draw_cmd_buffers[i],
			                                 offscreen.color[1].image,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			                                 VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, range);
		}

		/*
		    Second render pass: First bloom pass
		*/
		if (bloom)
		{
			VkClearValue clear_values[2];
			clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
			clear_values[1].depthStencil = {0.0f, 0};

			conditional_image_memory_barrier("BlImg Init", "Bloom image initialization",
			                                 draw_cmd_buffers[i],
			                                 filter_pass.color[0].image,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_IMAGE_LAYOUT_UNDEFINED,
			                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, range);

			VkRenderingAttachmentInfoKHR color_attachment_info = vkb::initializers::rendering_attachment_info();
			color_attachment_info.imageView                    = filter_pass.color[0].view;
			color_attachment_info.imageLayout                  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			color_attachment_info.resolveMode                  = VK_RESOLVE_MODE_NONE;
			color_attachment_info.loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
			color_attachment_info.storeOp                      = VK_ATTACHMENT_STORE_OP_STORE;
			color_attachment_info.clearValue                   = clear_values[0];

			auto render_area       = VkRect2D{VkOffset2D{}, VkExtent2D{(uint32_t) filter_pass.width, (uint32_t) filter_pass.height}};
			auto render_info       = vkb::initializers::rendering_info(render_area, 1, &color_attachment_info);
			render_info.layerCount = 1;

			vkCmdBeginRenderingKHR(draw_cmd_buffers[i], &render_info);

			VkViewport viewport = vkb::initializers::viewport(static_cast<float>(filter_pass.width), static_cast<float>(filter_pass.height), 0.0f, 1.0f);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkb::initializers::rect2D(filter_pass.width, filter_pass.height, 0, 0);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.bloom_filter, 0, 1, &descriptor_sets.bloom_filter, 0, NULL);

			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.bloom[1]);
			vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

			vkCmdEndRenderingKHR(draw_cmd_buffers[i]);

			conditional_image_memory_barrier("BlImg to RO", "Bloom image transition to read only",
			                                 draw_cmd_buffers[i],
			                                 filter_pass.color[0].image,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			                                 VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, range);
		}

		/*
		    Third render pass: Scene rendering with applied second bloom pass (when enabled)
		*/
		{
			VkClearValue clear_values[2];
			clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
			clear_values[1].depthStencil = {0.0f, 0};

			// Final composition

			conditional_image_memory_barrier("Swapchain Init", "Swapchain image initialization",
			                                 draw_cmd_buffers[i],
			                                 swapchain_buffers[i].image,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			                                 VK_IMAGE_LAYOUT_UNDEFINED,
			                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, range);

			VkRenderingAttachmentInfoKHR color_attachment_info = vkb::initializers::rendering_attachment_info();
			color_attachment_info.imageView                    = swapchain_buffers[i].view;        // color_attachment.image_view;
			color_attachment_info.imageLayout                  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			color_attachment_info.resolveMode                  = VK_RESOLVE_MODE_NONE;
			color_attachment_info.loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
			color_attachment_info.storeOp                      = VK_ATTACHMENT_STORE_OP_STORE;
			color_attachment_info.clearValue                   = clear_values[0];

			VkRenderingAttachmentInfoKHR depth_attachment_info = vkb::initializers::rendering_attachment_info();
			depth_attachment_info.imageView                    = depth_stencil.view;
			depth_attachment_info.imageLayout                  = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			depth_attachment_info.resolveMode                  = VK_RESOLVE_MODE_NONE;
			depth_attachment_info.loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depth_attachment_info.storeOp                      = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depth_attachment_info.clearValue                   = clear_values[1];

			auto render_area             = VkRect2D{VkOffset2D{}, VkExtent2D{width, height}};
			auto render_info             = vkb::initializers::rendering_info(render_area, 1, &color_attachment_info);
			render_info.layerCount       = 1;
			render_info.pDepthAttachment = &depth_attachment_info;
			if (!vkb::is_depth_only_format(depth_format))
			{
				render_info.pStencilAttachment = &depth_attachment_info;
			}

			vkCmdBeginRenderingKHR(draw_cmd_buffers[i], &render_info);

			VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.composition, 0, 1, &descriptor_sets.composition, 0, NULL);

			// Scene
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.composition);
			vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

			// Bloom
			if (bloom)
			{
				vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.bloom[0]);
				vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);
			}

			draw_ui(draw_cmd_buffers[i]);

			vkCmdEndRenderingKHR(draw_cmd_buffers[i]);
		}

		conditional_image_memory_barrier("Swapchain to Present", "Swapchain image transition to present",
		                                 draw_cmd_buffers[i],
		                                 swapchain_buffers[i].image,
		                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
		                                 VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
		                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		                                 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, range);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void Barriers::create_attachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment)
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
void Barriers::prepare_offscreen_buffer()
{
	// We need to select a format that supports the color attachment blending flag, so we iterate over multiple formats to find one that supports this flag
	VkFormat color_format{VK_FORMAT_UNDEFINED};

	const std::vector<VkFormat> float_format_priority_list = {
	    VK_FORMAT_R32G32B32A32_SFLOAT,
	    VK_FORMAT_R16G16B16A16_SFLOAT};

	for (auto &format : float_format_priority_list)
	{
		const VkFormatProperties properties = get_device().get_gpu().get_format_properties(format);
		if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)
		{
			color_format = format;
			break;
		}
	}

	if (color_format == VK_FORMAT_UNDEFINED)
	{
		throw std::runtime_error("No suitable float format could be determined");
	}

	{
		offscreen.width  = width;
		offscreen.height = height;

		// Color attachments

		// We are using two 128-Bit RGBA floating point color buffers for this sample
		// In a performance or bandwith-limited scenario you should consider using a format with lower precision
		create_attachment(color_format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &offscreen.color[0]);
		create_attachment(color_format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &offscreen.color[1]);
		// Depth attachment
		create_attachment(depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &offscreen.depth);

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

		// Floating point color attachment
		create_attachment(color_format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &filter_pass.color[0]);

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

void Barriers::load_assets()
{
	// Models
	models.skybox                      = load_model("scenes/cube.gltf");
	std::vector<std::string> filenames = {"geosphere.gltf", "teapot.gltf", "torusknot.gltf"};
	object_names                       = {"Sphere", "Teapot", "Torusknot"};
	for (auto file : filenames)
	{
		auto object = load_model("scenes/" + file);
		models.objects.emplace_back(std::move(object));
	}

	// Transforms
	auto geosphere_matrix = glm::mat4(1.0f);
	auto teapot_matrix    = glm::mat4(1.0f);
	teapot_matrix         = glm::scale(teapot_matrix, glm::vec3(10.0f, 10.0f, 10.0f));
	teapot_matrix         = glm::rotate(teapot_matrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	auto torus_matrix     = glm::mat4(1.0f);
	models.transforms.push_back(geosphere_matrix);
	models.transforms.push_back(teapot_matrix);
	models.transforms.push_back(torus_matrix);

	// Load Barriers cube map
	textures.envmap = load_texture_cubemap("textures/uffizi_rgba16f_cube.ktx", vkb::sg::Image::Color);
}

void Barriers::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6)};
	uint32_t                   num_descriptor_sets = 4;
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void Barriers::setup_descriptor_set_layout()
{
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

void Barriers::setup_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layouts.models,
	        1);

	// 3D object descriptor set
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.object));

	VkDescriptorBufferInfo            matrix_buffer_descriptor     = create_descriptor(*uniform_buffers.matrices);
	VkDescriptorImageInfo             environment_image_descriptor = create_descriptor(textures.envmap);
	VkDescriptorBufferInfo            params_buffer_descriptor     = create_descriptor(*uniform_buffers.params);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets        = {
        vkb::initializers::write_descriptor_set(descriptor_sets.object, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.object, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.object, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &params_buffer_descriptor),
    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);

	// Sky box descriptor set
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.skybox));

	matrix_buffer_descriptor     = create_descriptor(*uniform_buffers.matrices);
	environment_image_descriptor = create_descriptor(textures.envmap);
	params_buffer_descriptor     = create_descriptor(*uniform_buffers.params);
	write_descriptor_sets        = {
        vkb::initializers::write_descriptor_set(descriptor_sets.skybox, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.skybox, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.skybox, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &params_buffer_descriptor),
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

bool Barriers::prepare_gui(vkb::Platform &platform)
{
	// Create graphics pipeline for dynamic rendering
	VkFormat color_rendering_format   = render_context->get_format();
	VkFormat stencil_rendering_format = vkb::is_depth_only_format(depth_format) ? VK_FORMAT_UNDEFINED : depth_format;

	VkPipelineRenderingCreateInfoKHR pipeline_create{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
	pipeline_create.pNext                   = VK_NULL_HANDLE;
	pipeline_create.colorAttachmentCount    = 1;
	pipeline_create.pColorAttachmentFormats = &color_rendering_format;
	pipeline_create.depthAttachmentFormat   = depth_format;
	pipeline_create.stencilAttachmentFormat = stencil_rendering_format;

	gui = std::make_unique<vkb::Gui>(*this, platform.get_window(), /*stats=*/nullptr, 15.0f, true);
	gui->prepare(pipeline_cache, VK_NULL_HANDLE, &pipeline_create,
	             {load_shader("uioverlay/uioverlay.vert", VK_SHADER_STAGE_VERTEX_BIT),
	              load_shader("uioverlay/uioverlay.frag", VK_SHADER_STAGE_FRAGMENT_BIT)});
	return true;
}

void Barriers::prepare_pipelines()
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
	        VK_NULL_HANDLE,
	        0);

	std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_states = {
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	};

	// Create graphics pipeline for dynamic rendering
	VkFormat color_rendering_format[2] = {};
	VkFormat stencil_rendering_format  = vkb::is_depth_only_format(depth_format) ? VK_FORMAT_UNDEFINED : depth_format;

	// Provide information for dynamic rendering
	VkPipelineRenderingCreateInfoKHR pipeline_create{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
	pipeline_create.pNext      = VK_NULL_HANDLE;
	pipeline_create_info.pNext = &pipeline_create;

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

	VkSpecializationInfo                    specialization_info;
	std::array<VkSpecializationMapEntry, 1> specialization_map_entries;

	// Full screen pipelines

	// Empty vertex input state, full screen triangles are generated by the vertex shader
	VkPipelineVertexInputStateCreateInfo empty_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	pipeline_create_info.pVertexInputState                 = &empty_input_state;

	// Final fullscreen composition pass pipeline
	shader_stages[0]                  = load_shader("hdr/composition.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]                  = load_shader("hdr/composition.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	pipeline_create_info.layout       = pipeline_layouts.composition;
	pipeline_create_info.renderPass   = VK_NULL_HANDLE;
	rasterization_state.cullMode      = VK_CULL_MODE_FRONT_BIT;
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments    = blend_attachment_states.data();

	color_rendering_format[0]               = render_context->get_format();
	pipeline_create.colorAttachmentCount    = 1;
	pipeline_create.pColorAttachmentFormats = color_rendering_format;
	pipeline_create.depthAttachmentFormat   = depth_format;
	pipeline_create.stencilAttachmentFormat = stencil_rendering_format;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.composition));

	// Bloom pass
	shader_stages[0]                           = load_shader("hdr/bloom.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]                           = load_shader("hdr/bloom.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
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
	specialization_map_entries[0]        = vkb::initializers::specialization_map_entry(0, 0, sizeof(uint32_t));
	uint32_t dir                         = 1;
	specialization_info                  = vkb::initializers::specialization_info(1, specialization_map_entries.data(), sizeof(dir), &dir);
	shader_stages[1].pSpecializationInfo = &specialization_info;

	color_rendering_format[0]               = render_context->get_format();
	pipeline_create.colorAttachmentCount    = 1;
	pipeline_create.pColorAttachmentFormats = color_rendering_format;
	pipeline_create.depthAttachmentFormat   = depth_format;
	pipeline_create.stencilAttachmentFormat = stencil_rendering_format;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.bloom[0]));

	// Second blur pass (into separate framebuffer)
	pipeline_create_info.renderPass = VK_NULL_HANDLE;
	dir                             = 0;

	color_rendering_format[0]               = offscreen.color[0].format;
	pipeline_create.colorAttachmentCount    = 1;
	pipeline_create.pColorAttachmentFormats = color_rendering_format;
	pipeline_create.depthAttachmentFormat   = VK_FORMAT_UNDEFINED;
	pipeline_create.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.bloom[1]));

	// Object rendering pipelines
	rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;

	// Vertex bindings an attributes for model rendering
	// Binding description
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	// Attribute descriptions
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                       // Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3)        // Normal
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	pipeline_create_info.pVertexInputState = &vertex_input_state;

	// Skybox pipeline (background cube)
	blend_attachment_state.blendEnable = VK_FALSE;
	pipeline_create_info.layout        = pipeline_layouts.models;
	color_blend_state.attachmentCount  = 2;
	color_blend_state.pAttachments     = blend_attachment_states.data();

	shader_stages[0] = load_shader("hdr/gbuffer.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("hdr/gbuffer.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Set constant parameters via specialization constants
	specialization_map_entries[0]        = vkb::initializers::specialization_map_entry(0, 0, sizeof(uint32_t));
	uint32_t shadertype                  = 0;
	specialization_info                  = vkb::initializers::specialization_info(1, specialization_map_entries.data(), sizeof(shadertype), &shadertype);
	shader_stages[0].pSpecializationInfo = &specialization_info;
	shader_stages[1].pSpecializationInfo = &specialization_info;

	color_rendering_format[0]               = offscreen.color[0].format;
	color_rendering_format[1]               = offscreen.color[1].format;
	pipeline_create.colorAttachmentCount    = 2;
	pipeline_create.pColorAttachmentFormats = color_rendering_format;
	pipeline_create.depthAttachmentFormat   = depth_format;
	pipeline_create.stencilAttachmentFormat = stencil_rendering_format;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.skybox));

	// Object rendering pipeline
	shadertype = 1;

	// Enable depth test and write
	depth_stencil_state.depthWriteEnable = VK_TRUE;
	depth_stencil_state.depthTestEnable  = VK_TRUE;
	// Flip cull mode
	rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;

	color_rendering_format[0]               = offscreen.color[0].format;
	color_rendering_format[1]               = offscreen.color[1].format;
	pipeline_create.colorAttachmentCount    = 2;
	pipeline_create.pColorAttachmentFormats = color_rendering_format;
	pipeline_create.depthAttachmentFormat   = depth_format;
	pipeline_create.stencilAttachmentFormat = stencil_rendering_format;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.reflect));
}

// Prepare and initialize uniform buffer containing shader uniforms
void Barriers::prepare_uniform_buffers()
{
	// Matrices vertex shader uniform buffer
	uniform_buffers.matrices = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                               sizeof(ubo_vs),
	                                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                               VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Params
	uniform_buffers.params = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                             sizeof(ubo_params),
	                                                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                             VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
	update_params();
}

void Barriers::update_uniform_buffers()
{
	ubo_vs.projection       = camera.matrices.perspective;
	ubo_vs.modelview        = camera.matrices.view * models.transforms[models.object_index];
	ubo_vs.skybox_modelview = camera.matrices.view;
	uniform_buffers.matrices->convert_and_update(ubo_vs);
}

void Barriers::update_params()
{
	uniform_buffers.params->convert_and_update(ubo_params);
}

void Barriers::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

bool Barriers::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -4.0f));
	camera.set_rotation(glm::vec3(0.0f, 180.0f, 0.0f));

	// Note: Using Revsered depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	load_assets();
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

void Barriers::render(float delta_time)
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

void Barriers::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.combo_box("Object type", &models.object_index, object_names))
		{
			update_uniform_buffers();
			build_command_buffers();
		}
		if (drawer.input_float("Exposure", &ubo_params.exposure, 0.025f, 3))
		{
			update_params();
		}
		if (drawer.checkbox("Bloom", &bloom))
		{
			build_command_buffers();
		}
		if (drawer.checkbox("Skybox", &display_skybox))
		{
			build_command_buffers();
		}
	}
	if (drawer.header("Barriers (enable, stage, access)"))
	{
		bool rebuild = false;

		ImGui::Columns(2);
		for (auto &data : test_barriers)
		{
			ImGui::PushID(data.title.c_str());

			drawer.text(data.title.c_str());
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("%s", data.tooltip.c_str());
			}

			ImGui::NextColumn();
			rebuild |= drawer.checkbox("##0", &data.enable);
			if (data.enable)
			{
				ImGui::SameLine();
				rebuild |= drawer.checkbox("##1", &data.enable_stage_mask);
				ImGui::SameLine();
				rebuild |= drawer.checkbox("##2", &data.enable_access_mask);
			}

			ImGui::NextColumn();
			ImGui::PopID();
		}
		ImGui::Columns(1);

		if (rebuild)
		{
			build_command_buffers();
		}
	}
}

bool Barriers::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

std::unique_ptr<vkb::Application> create_barriers()
{
	return std::make_unique<Barriers>();
}
