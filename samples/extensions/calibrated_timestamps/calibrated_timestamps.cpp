/* Copyright (c) 2023, Holochip Corporation
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

#include "calibrated_timestamps.h"

std::string time_domain_to_string(VkTimeDomainEXT input_time_domain)
{
	switch (input_time_domain)
	{
		case VK_TIME_DOMAIN_DEVICE_EXT:
			return "device time domain";
		case VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT:
			return "clock monotonic time domain";
		case VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT:
			return "clock monotonic raw time domain";
		case VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT:
			return "query performance time domain";
		default:
			return "unknown time domain";
	}
}

CalibratedTimestamps::CalibratedTimestamps() :
    is_time_domain_init(false)
{
	title = "Calibrated Timestamps";

	// Add instance extensions required for calibrated timestamps
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	// NOTICE THAT: calibrated timestamps is a DEVICE extension!
	add_device_extension(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME);
}

CalibratedTimestamps::~CalibratedTimestamps()
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

		vkDestroySampler(get_device().get_handle(), textures.environment_map.sampler, nullptr);
	}
}

void CalibratedTimestamps::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
}

void CalibratedTimestamps::build_command_buffers()
{
	timestamps_begin("Build Command Buffers");

	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	std::array<VkClearValue, 3> offscreen_clear_values{};
	VkClearValue                filter_clear_value{};
	std::array<VkClearValue, 2> bloom_clear_values{};

	VkRenderPassBeginInfo offscreen_render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	offscreen_render_pass_begin_info.renderPass               = offscreen.render_pass;
	offscreen_render_pass_begin_info.framebuffer              = offscreen.framebuffer;
	offscreen_render_pass_begin_info.renderArea.extent.width  = offscreen.width;
	offscreen_render_pass_begin_info.renderArea.extent.height = offscreen.height;
	offscreen_render_pass_begin_info.clearValueCount          = static_cast<uint32_t>(offscreen_clear_values.size());
	offscreen_render_pass_begin_info.pClearValues             = offscreen_clear_values.data();
	VkRenderPassBeginInfo filter_render_pass_begin_info       = vkb::initializers::render_pass_begin_info();
	filter_render_pass_begin_info.framebuffer                 = filter_pass.framebuffer;
	filter_render_pass_begin_info.renderPass                  = filter_pass.render_pass;
	filter_render_pass_begin_info.clearValueCount             = 1;
	filter_render_pass_begin_info.renderArea.extent.width     = filter_pass.width;
	filter_render_pass_begin_info.renderArea.extent.height    = filter_pass.height;
	filter_render_pass_begin_info.pClearValues                = &filter_clear_value;
	VkRenderPassBeginInfo bloom_render_pass_begin_info        = vkb::initializers::render_pass_begin_info();
	bloom_render_pass_begin_info.renderPass                   = render_pass;
	bloom_render_pass_begin_info.clearValueCount              = static_cast<uint32_t>(bloom_clear_values.size());
	bloom_render_pass_begin_info.renderArea.extent.width      = width;
	bloom_render_pass_begin_info.renderArea.extent.height     = height;
	bloom_render_pass_begin_info.pClearValues                 = bloom_clear_values.data();

	VkViewport offscreen_viewport = vkb::initializers::viewport(static_cast<float>(offscreen.width), static_cast<float>(offscreen.height), 0.0f, 1.0f);
	VkViewport filter_viewport    = vkb::initializers::viewport(static_cast<float>(filter_pass.width), static_cast<float>(filter_pass.height), 0.0f, 1.0f);
	VkViewport bloom_viewport     = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);

	VkRect2D offscreen_scissor = vkb::initializers::rect2D(offscreen.width, offscreen.height, 0, 0);
	VkRect2D filter_scissor    = vkb::initializers::rect2D(filter_pass.width, filter_pass.height, 0, 0);
	VkRect2D bloom_scissor     = vkb::initializers::rect2D(static_cast<int>(width), static_cast<int>(height), 0, 0);

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));
		{
			vkCmdBeginRenderPass(draw_cmd_buffers[i], &offscreen_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &offscreen_viewport);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &offscreen_scissor);

			if (display_skybox)
			{
				vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skybox);
				vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.models, 0, 1, &descriptor_sets.skybox, 0, nullptr);
				draw_model(models.skybox, draw_cmd_buffers[i]);
			}
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.reflect);
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.models, 0, 1, &descriptor_sets.object, 0, nullptr);
			draw_model(models.objects[models.object_index], draw_cmd_buffers[i]);
			vkCmdEndRenderPass(draw_cmd_buffers[i]);
		}

		if (bloom)
		{
			vkCmdBeginRenderPass(draw_cmd_buffers[i], &filter_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &filter_viewport);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &filter_scissor);
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.bloom_filter, 0, 1, &descriptor_sets.bloom_filter, 0, nullptr);
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.bloom[1]);
			vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);
			vkCmdEndRenderPass(draw_cmd_buffers[i]);
		}

		{
			bloom_render_pass_begin_info.framebuffer = framebuffers[i];

			vkCmdBeginRenderPass(draw_cmd_buffers[i], &bloom_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &bloom_viewport);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &bloom_scissor);
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.composition, 0, 1, &descriptor_sets.composition, 0, nullptr);
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.composition);
			vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

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

	timestamps_end("Build Command Buffers");
}

void CalibratedTimestamps::create_attachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment)
{
	VkImageAspectFlags aspect_mask = 0;

	attachment->format = format;

	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
		{
			aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
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

void CalibratedTimestamps::prepare_offscreen_buffer()
{
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
		offscreen.width  = static_cast<int32_t>(width);
		offscreen.height = static_cast<int32_t>(height);

		create_attachment(color_format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &offscreen.color[0]);
		create_attachment(color_format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &offscreen.color[1]);
		create_attachment(depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &offscreen.depth);

		std::array<VkAttachmentDescription, 3> attachment_descriptions = {};

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
		subpass.colorAttachmentCount    = static_cast<uint32_t>(color_references.size());
		subpass.pDepthStencilAttachment = &depth_reference;

		std::array<VkSubpassDependency, 2> dependencies{};

		dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass      = 0;
		dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass      = 0;
		dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo render_pass_create_info = {};
		render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_create_info.pAttachments           = attachment_descriptions.data();
		render_pass_create_info.attachmentCount        = static_cast<uint32_t>(attachment_descriptions.size());
		render_pass_create_info.subpassCount           = 1;
		render_pass_create_info.pSubpasses             = &subpass;
		render_pass_create_info.dependencyCount        = static_cast<uint32_t>(dependencies.size());
		render_pass_create_info.pDependencies          = dependencies.data();

		VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &offscreen.render_pass));

		std::array<VkImageView, 3> attachments{};
		attachments[0] = offscreen.color[0].view;
		attachments[1] = offscreen.color[1].view;
		attachments[2] = offscreen.depth.view;

		VkFramebufferCreateInfo framebuffer_create_info = {};
		framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.pNext                   = nullptr;
		framebuffer_create_info.renderPass              = offscreen.render_pass;
		framebuffer_create_info.pAttachments            = attachments.data();
		framebuffer_create_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
		framebuffer_create_info.width                   = offscreen.width;
		framebuffer_create_info.height                  = offscreen.height;
		framebuffer_create_info.layers                  = 1;
		VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_create_info, nullptr, &offscreen.framebuffer));

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

	{
		filter_pass.width  = static_cast<int32_t>(width);
		filter_pass.height = static_cast<int32_t>(height);

		create_attachment(color_format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &filter_pass.color[0]);

		VkAttachmentDescription attachment_description{};

		attachment_description.samples        = VK_SAMPLE_COUNT_1_BIT;
		attachment_description.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment_description.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment_description.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment_description.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachment_description.format         = filter_pass.color[0].format;

		std::vector<VkAttachmentReference> color_references;
		color_references.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments    = color_references.data();
		subpass.colorAttachmentCount = static_cast<uint32_t>(color_references.size());

		std::array<VkSubpassDependency, 2> dependencies{};

		dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass      = 0;
		dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass      = 0;
		dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo render_pass_create_info = {};
		render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_create_info.pAttachments           = &attachment_description;
		render_pass_create_info.attachmentCount        = 1;
		render_pass_create_info.subpassCount           = 1;
		render_pass_create_info.pSubpasses             = &subpass;
		render_pass_create_info.dependencyCount        = static_cast<uint32_t>(dependencies.size());
		render_pass_create_info.pDependencies          = dependencies.data();

		VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &filter_pass.render_pass));

		VkImageView attachment = filter_pass.color[0].view;

		VkFramebufferCreateInfo framebuffer_create_info = {};
		framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.pNext                   = nullptr;
		framebuffer_create_info.renderPass              = filter_pass.render_pass;
		framebuffer_create_info.pAttachments            = &attachment;
		framebuffer_create_info.attachmentCount         = 1;
		framebuffer_create_info.width                   = filter_pass.width;
		framebuffer_create_info.height                  = filter_pass.height;
		framebuffer_create_info.layers                  = 1;
		VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_create_info, nullptr, &filter_pass.framebuffer));

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

void CalibratedTimestamps::load_assets()
{
	models.skybox                      = load_model("scenes/cube.gltf");
	std::vector<std::string> filenames = {"geosphere.gltf", "teapot.gltf", "torusknot.gltf"};
	object_names                       = {"Sphere", "Teapot", "Torusknot"};
	for (const auto &file : filenames)
	{
		auto object = load_model("scenes/" + file);
		models.objects.emplace_back(std::move(object));
	}

	auto geosphere_matrix = glm::mat4(1.0f);
	auto teapot_matrix    = glm::mat4(1.0f);
	teapot_matrix         = glm::scale(teapot_matrix, glm::vec3(10.0f, 10.0f, 10.0f));
	teapot_matrix         = glm::rotate(teapot_matrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	auto torus_matrix     = glm::mat4(1.0f);
	models.transforms.push_back(geosphere_matrix);
	models.transforms.push_back(teapot_matrix);
	models.transforms.push_back(torus_matrix);

	textures.environment_map = load_texture_cubemap("textures/uffizi_rgba16f_cube.ktx", vkb::sg::Image::Color);
}

void CalibratedTimestamps::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6)};
	uint32_t                   num_descriptor_sets = 4;
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void CalibratedTimestamps::setup_descriptor_set_layout()
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

	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	};

	descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.bloom_filter));

	pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layouts.bloom_filter, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.bloom_filter));

	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	};

	descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.composition));

	pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layouts.composition, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.composition));
}

void CalibratedTimestamps::setup_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layouts.models, 1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.object));

	VkDescriptorBufferInfo            matrix_buffer_descriptor     = create_descriptor(*uniform_buffers.matrices);
	VkDescriptorImageInfo             environment_image_descriptor = create_descriptor(textures.environment_map);
	VkDescriptorBufferInfo            params_buffer_descriptor     = create_descriptor(*uniform_buffers.params);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets        = {
        vkb::initializers::write_descriptor_set(descriptor_sets.object, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.object, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.object, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &params_buffer_descriptor),
    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.skybox));

	matrix_buffer_descriptor     = create_descriptor(*uniform_buffers.matrices);
	environment_image_descriptor = create_descriptor(textures.environment_map);
	params_buffer_descriptor     = create_descriptor(*uniform_buffers.params);
	write_descriptor_sets        = {
        vkb::initializers::write_descriptor_set(descriptor_sets.skybox, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.skybox, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.skybox, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &params_buffer_descriptor),
    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

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
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

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
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void CalibratedTimestamps::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

	VkPipelineColorBlendAttachmentState blend_attachment_state = vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector<VkDynamicState>      dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state         = vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables.data(), static_cast<uint32_t>(dynamic_state_enables.size()), 0);

	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layouts.models, render_pass, 0);

	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages    = shader_stages.data();

	VkSpecializationInfo                    specialization_info;
	std::array<VkSpecializationMapEntry, 1> specialization_map_entries{};

	VkPipelineVertexInputStateCreateInfo empty_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	pipeline_create_info.pVertexInputState                 = &empty_input_state;

	shader_stages[0]                  = load_shader("hdr/composition.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]                  = load_shader("hdr/composition.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	pipeline_create_info.layout       = pipeline_layouts.composition;
	pipeline_create_info.renderPass   = render_pass;
	rasterization_state.cullMode      = VK_CULL_MODE_FRONT_BIT;
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments    = &blend_attachment_state;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.composition));

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

	specialization_map_entries[0]        = vkb::initializers::specialization_map_entry(0, 0, sizeof(uint32_t));
	uint32_t dir                         = 1;
	specialization_info                  = vkb::initializers::specialization_info(1, specialization_map_entries.data(), sizeof(dir), &dir);
	shader_stages[1].pSpecializationInfo = &specialization_info;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.bloom[0]));

	pipeline_create_info.renderPass = filter_pass.render_pass;
	dir                             = 0;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.bloom[1]));

	rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;

	std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_states = {
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	};

	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3)};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	pipeline_create_info.pVertexInputState = &vertex_input_state;

	blend_attachment_state.blendEnable = VK_FALSE;
	pipeline_create_info.layout        = pipeline_layouts.models;
	pipeline_create_info.renderPass    = offscreen.render_pass;
	color_blend_state.attachmentCount  = static_cast<uint32_t>(blend_attachment_states.size());
	color_blend_state.pAttachments     = blend_attachment_states.data();

	shader_stages[0] = load_shader("hdr/gbuffer.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("hdr/gbuffer.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	specialization_map_entries[0]        = vkb::initializers::specialization_map_entry(0, 0, sizeof(uint32_t));
	uint32_t shadertype                  = 0;
	specialization_info                  = vkb::initializers::specialization_info(1, specialization_map_entries.data(), sizeof(shadertype), &shadertype);
	shader_stages[0].pSpecializationInfo = &specialization_info;
	shader_stages[1].pSpecializationInfo = &specialization_info;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.skybox));

	shadertype = 1;

	depth_stencil_state.depthWriteEnable = VK_TRUE;
	depth_stencil_state.depthTestEnable  = VK_TRUE;
	rasterization_state.cullMode         = VK_CULL_MODE_FRONT_BIT;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.reflect));
}

void CalibratedTimestamps::prepare_uniform_buffers()
{
	uniform_buffers.matrices = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubo_vs), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	uniform_buffers.params   = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubo_params), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
	uniform_buffers.params->convert_and_update(ubo_params);
}

void CalibratedTimestamps::update_uniform_buffers()
{
	timestamps_begin("update ubo");
	ubo_vs.projection        = camera.matrices.perspective;
	ubo_vs.model_view        = camera.matrices.view * models.transforms[models.object_index];
	ubo_vs.skybox_model_view = camera.matrices.view;
	uniform_buffers.matrices->convert_and_update(ubo_vs);
	timestamps_end("update ubo");
}

void CalibratedTimestamps::draw()
{
	timestamps_begin("draw");
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
	timestamps_end("draw");
}

bool CalibratedTimestamps::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -4.0f));
	camera.set_rotation(glm::vec3(0.0f, 180.0f, 0.0f));
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	// Get the optimal time domain as soon as possible
	get_device_time_domain();

	// Preparations
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

void CalibratedTimestamps::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
	if (camera.updated)
		update_uniform_buffers();
}

void CalibratedTimestamps::get_time_domains()
{
	// Initialize time domain count:
	uint32_t time_domain_count = 0;
	// Update time domain count:
	VkResult result = vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(get_device().get_gpu().get_handle(), &time_domain_count, nullptr);

	if (result == VK_SUCCESS)
	{
		// Resize time domains vector:
		time_domains.resize(time_domain_count);
		// Update time_domain vector:
		result = vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(get_device().get_gpu().get_handle(), &time_domain_count, time_domains.data());
	}

	if (time_domain_count > 0)
	{
		for (VkTimeDomainEXT time_domain : time_domains)
		{
			// Initialize in-scope time stamp info variable:
			VkCalibratedTimestampInfoEXT timestamp_info{};

			// Configure timestamp info variable:
			timestamp_info.sType      = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT;
			timestamp_info.pNext      = nullptr;
			timestamp_info.timeDomain = time_domain;

			// Push-back timestamp info to timestamps info vector:
			timestamps_info.push_back(timestamp_info);
		}

		// Resize time stamps vector
		timestamps.resize(time_domain_count);
		// Resize max deviations vector
		max_deviations.resize(time_domain_count);
	}

	is_time_domain_init = ((result == VK_SUCCESS) && (time_domain_count > 0));
}

VkResult CalibratedTimestamps::get_timestamps()
{
	// Ensures that time domain exists
	if (is_time_domain_init)
	{
		// Get calibrated timestamps:
		return vkGetCalibratedTimestampsEXT(get_device().get_handle(), static_cast<uint32_t>(time_domains.size()), timestamps_info.data(), timestamps.data(), max_deviations.data());
	}
	return VK_ERROR_UNKNOWN;
}

void CalibratedTimestamps::get_device_time_domain()
{
	get_time_domains();

	if (is_time_domain_init && (time_domains.size() > 1))
	{
		auto iterator_device = std::find(time_domains.begin(), time_domains.end(), VK_TIME_DOMAIN_DEVICE_EXT);

		int device_index = static_cast<int>(iterator_device - time_domains.begin());

		selected_time_domain.index         = device_index;
		selected_time_domain.timeDomainEXT = time_domains[device_index];
	}
}

void CalibratedTimestamps::timestamps_begin(const std::string &input_tag)
{
	// Now to get timestamps
	if (get_timestamps() == VK_SUCCESS)
	{
		if (!input_tag.empty())
		{
			auto pos = std::remove_if(delta_timestamps.begin(), delta_timestamps.end(), [input_tag](auto ts) { return ts.tag == input_tag; });
			if (pos != delta_timestamps.end())
				delta_timestamps.erase(pos);
		}
		// Create a local delta_timestamp to push back to the vector delta_timestamps
		DeltaTimestamp delta_timestamp{};

		if (!input_tag.empty())
		{
			delta_timestamp.tag = input_tag;
		}
		delta_timestamp.begin = timestamps[selected_time_domain.index];

		// Push back this partially filled element to the vector
		delta_timestamps.push_back(delta_timestamp);
	}
}

void CalibratedTimestamps::timestamps_end(const std::string &input_tag)
{
	if (delta_timestamps.empty())
	{
		LOGE("Timestamps begin-to-end Fatal Error: Timestamps end is not tagged the same as its begin!\n")
		return;        // exits the function here, further calculation is meaningless
	}

	auto result = get_timestamps();
	if (result == VK_SUCCESS)
	{
		for (auto ts : delta_timestamps)
		{
			if (ts.tag == input_tag)
			{
				// Add this data to the last term of delta_timestamps vector
				ts.end   = timestamps[selected_time_domain.index];
				ts.delta = ts.end - ts.begin;
				return;
			}
		}
		LOGE("timestamps_end called without a found corresponding begin timestamp.\n")
		return;
	}
	LOGE("get_timestamps failed with %d", result);
}

void CalibratedTimestamps::on_update_ui_overlay(vkb::Drawer &drawer)
{
	// Timestamps period extracted in runtime
	float timestamp_period = device->get_gpu().get_properties().limits.timestampPeriod;
	drawer.text("Timestamps Period:\n %.1f Nanoseconds", timestamp_period);

	// Adjustment Handles:
	if (drawer.header("Settings"))
	{
		if (drawer.combo_box("Object type", &models.object_index, object_names))
		{
			update_uniform_buffers();
			build_command_buffers();
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

	if (!delta_timestamps.empty())
	{
		drawer.text("Time Domain Selected:\n %s", time_domain_to_string(selected_time_domain.timeDomainEXT).c_str());

		for (const auto &delta_timestamp : delta_timestamps)
		{
			drawer.text("%s:\n %.1f Microseconds", delta_timestamp.tag.c_str(), static_cast<float>(delta_timestamp.delta) * timestamp_period * 0.001f);
		}
	}
}

bool CalibratedTimestamps::resize(uint32_t width, uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

std::unique_ptr<vkb::Application> create_calibrated_timestamps()
{
	return std::make_unique<CalibratedTimestamps>();
}
