/* Copyright (c) 2024, Google
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

#include "oit_depth_peeling.h"
#include <algorithm>

OITDepthPeeling::OITDepthPeeling()
{
}

OITDepthPeeling::~OITDepthPeeling()
{
	if (!has_device())
	{
		return;
	}

	vkDestroyPipeline(get_device().get_handle(), background_pipeline, nullptr);
	vkDestroyPipeline(get_device().get_handle(), combine_pipeline, nullptr);
	vkDestroyPipelineLayout(get_device().get_handle(), combine_pipeline_layout, nullptr);
	vkDestroyPipeline(get_device().get_handle(), gather_pipeline, nullptr);
	vkDestroyPipeline(get_device().get_handle(), gather_first_pipeline, nullptr);
	vkDestroyPipelineLayout(get_device().get_handle(), gather_pipeline_layout, nullptr);

	vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, VK_NULL_HANDLE);

	destroy_sized_objects();

	scene_constants.reset();

	vkDestroyDescriptorSetLayout(get_device().get_handle(), gather_descriptor_set_layout, nullptr);
	vkDestroyDescriptorSetLayout(get_device().get_handle(), combine_descriptor_set_layout, nullptr);

	vkDestroySampler(get_device().get_handle(), point_sampler, VK_NULL_HANDLE);
	vkDestroySampler(get_device().get_handle(), background_texture.sampler, nullptr);
	object.reset();
}

bool OITDepthPeeling::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.0f, 0.0f, -4.0f});
	camera.set_rotation({0.0f, 0.0f, 0.0f});
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 16.0f, 0.1f);

	load_assets();
	create_samplers();
	create_constant_buffers();
	create_descriptors();
	create_sized_objects(width, height);
	create_pipelines();

	update_scene_constants();
	update_descriptors();
	build_command_buffers();

	prepared = true;
	return true;
}

bool OITDepthPeeling::resize(const uint32_t width, const uint32_t height)
{
	destroy_sized_objects();
	create_sized_objects(width, height);
	update_descriptors();
	ApiVulkanSample::resize(width, height);
	return true;
}

void OITDepthPeeling::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}

	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();

	if (camera_auto_rotation)
	{
		camera.rotate({delta_time * 5.0f, delta_time * 5.0f, 0.0f});
	}
	update_scene_constants();
}

void OITDepthPeeling::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
	else
	{
		throw std::runtime_error("This sample requires support for anisotropic sampling");
	}
}

void OITDepthPeeling::on_update_ui_overlay(vkb::Drawer &drawer)
{
	drawer.checkbox("Camera auto-rotation", &camera_auto_rotation);
	drawer.slider_float("Background grayscale", &background_grayscale, kBackgroundGrayscaleMin, kBackgroundGrayscaleMax);
	drawer.slider_float("Object opacity", &object_alpha, kObjectAlphaMin, kObjectAlphaMax);

	drawer.slider_int("Front layer index", &front_layer_index, 0, back_layer_index);
	drawer.slider_int("Back layer index", &back_layer_index, front_layer_index, kLayerMaxCount - 1);
}

void OITDepthPeeling::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));
		{
			// Gather passes
			// Each pass renders a single transparent layer into a layer texture.
			for (uint32_t l = 0; l <= back_layer_index; ++l)
			{
				// Two depth textures are used.
				// Their roles alternates for each pass.
				// The first depth texture is used for fixed-function depth test.
				// The second one is the result of the depth test from the previous gather pass.
				// It is bound as texture and read in the shader to discard fragments from the
				// previous layers.
				VkImageSubresourceRange depth_subresource_range = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
				vkb::image_layout_transition(
				    draw_cmd_buffers[i], depth_image[l % kDepthCount]->get_handle(),
				    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				    VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				    l <= 1 ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				    depth_subresource_range);
				if (l > 0)
				{
					vkb::image_layout_transition(
					    draw_cmd_buffers[i], depth_image[(l + 1) % kDepthCount]->get_handle(),
					    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
					    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
					    depth_subresource_range);
				}

				// Set one of the layer textures as color attachment, as the gather pass will render to it.
				VkImageSubresourceRange layer_subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
				vkb::image_layout_transition(
				    draw_cmd_buffers[i], layer_image[l]->get_handle(),
				    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				    VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				    layer_subresource_range);

				render_pass_begin_info.framebuffer = gather_framebuffer[l];
				render_pass_begin_info.renderPass  = gather_render_pass;
				vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
				{
					// Render the geometry into the layer texture.
					VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
					vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

					VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
					vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

					vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, gather_pipeline_layout, 0, 1, &gather_descriptor_set[l % kDepthCount], 0, NULL);

					vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, l == 0 ? gather_first_pipeline : gather_pipeline);
					draw_model(object, draw_cmd_buffers[i]);
				}
				vkCmdEndRenderPass(draw_cmd_buffers[i]);

				// Get the layer texture ready to be read by the combine pass.
				vkb::image_layout_transition(
				    draw_cmd_buffers[i], layer_image[l]->get_handle(),
				    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				    layer_subresource_range);
			}

			// Combine pass
			// This pass blends all the layers into the final transparent color.
			// The final color is then alpha blended into the background.
			render_pass_begin_info.framebuffer = framebuffers[i];
			render_pass_begin_info.renderPass  = render_pass;
			vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
			{
				VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
				vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

				VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
				vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

				vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, combine_pipeline_layout, 0, 1, &combine_descriptor_set, 0, NULL);

				vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, background_pipeline);
				vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

				vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, combine_pipeline);
				vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

				draw_ui(draw_cmd_buffers[i]);
			}
			vkCmdEndRenderPass(draw_cmd_buffers[i]);
		}
		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

////////////////////////////////////////////////////////////////////////////////

void OITDepthPeeling::create_sized_objects(const uint32_t width, const uint32_t height)
{
	create_images(width, height);
	create_gather_pass_objects(width, height);
}

void OITDepthPeeling::destroy_sized_objects()
{
	for (uint32_t i = 0; i < kDepthCount; ++i)
	{
		depth_image_view[i].reset();
		depth_image[i].reset();
	}
	for (uint32_t i = 0; i < kLayerMaxCount; ++i)
	{
		vkDestroyFramebuffer(get_device().get_handle(), gather_framebuffer[i], nullptr);
		layer_image_view[i].reset();
		layer_image[i].reset();
	}
	vkDestroyRenderPass(get_device().get_handle(), gather_render_pass, nullptr);
}

void OITDepthPeeling::create_gather_pass_objects(const uint32_t width, const uint32_t height)
{
	const VkAttachmentReference color_attachment_reference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
	const VkAttachmentReference depth_attachment_reference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

	VkSubpassDescription subpass    = {};
	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount    = 1;
	subpass.pColorAttachments       = &color_attachment_reference;
	subpass.pDepthStencilAttachment = &depth_attachment_reference;

	VkAttachmentDescription attachment_descriptions[2] = {};
	attachment_descriptions[0].format                  = VK_FORMAT_R8G8B8A8_UNORM;
	attachment_descriptions[0].samples                 = VK_SAMPLE_COUNT_1_BIT;
	attachment_descriptions[0].loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_descriptions[0].storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_descriptions[0].stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_descriptions[0].stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descriptions[0].initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_descriptions[0].finalLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment_descriptions[1].format                  = VK_FORMAT_D32_SFLOAT;
	attachment_descriptions[1].samples                 = VK_SAMPLE_COUNT_1_BIT;
	attachment_descriptions[1].loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_descriptions[1].storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_descriptions[1].stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_descriptions[1].stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descriptions[1].initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_descriptions[1].finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.subpassCount           = 1;
	render_pass_create_info.pSubpasses             = &subpass;
	render_pass_create_info.attachmentCount        = 2;
	render_pass_create_info.pAttachments           = attachment_descriptions;
	VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &gather_render_pass));

	VkFramebufferCreateInfo framebuffer_create_info = vkb::initializers::framebuffer_create_info();
	framebuffer_create_info.width                   = width;
	framebuffer_create_info.height                  = height;
	framebuffer_create_info.layers                  = 1;
	framebuffer_create_info.attachmentCount         = 2;
	for (uint32_t i = 0; i < kLayerMaxCount; ++i)
	{
		framebuffer_create_info.renderPass = gather_render_pass;
		const VkImageView attachments[]    = {
            layer_image_view[i]->get_handle(),
            depth_image_view[i % kDepthCount]->get_handle(),
        };
		framebuffer_create_info.pAttachments = attachments;
		VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_create_info, nullptr, gather_framebuffer + i));
	}
}

void OITDepthPeeling::create_images(const uint32_t width, const uint32_t height)
{
	const VkExtent3D image_extent = {width, height, 1};
	for (uint32_t i = 0; i < kLayerMaxCount; ++i)
	{
		layer_image[i]      = std::make_unique<vkb::core::Image>(get_device(), image_extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_SAMPLE_COUNT_1_BIT);
		layer_image_view[i] = std::make_unique<vkb::core::ImageView>(*layer_image[i], VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM);
	}

	for (uint32_t i = 0; i < kDepthCount; ++i)
	{
		depth_image[i]      = std::make_unique<vkb::core::Image>(get_device(), image_extent, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_SAMPLE_COUNT_1_BIT);
		depth_image_view[i] = std::make_unique<vkb::core::ImageView>(*depth_image[i], VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_D32_SFLOAT);
	}
}

////////////////////////////////////////////////////////////////////////////////

void OITDepthPeeling::load_assets()
{
	object             = load_model("scenes/torusknot.gltf");
	background_texture = load_texture("textures/vulkan_logo_full.ktx", vkb::sg::Image::Color);
}

void OITDepthPeeling::create_samplers()
{
	VkSamplerCreateInfo sampler_info = vkb::initializers::sampler_create_info();
	sampler_info.magFilter           = VK_FILTER_NEAREST;
	sampler_info.minFilter           = VK_FILTER_NEAREST;
	sampler_info.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.mipLodBias          = 0.0f;
	sampler_info.maxAnisotropy       = 1.0f;
	sampler_info.minLod              = 0.0f;
	sampler_info.maxLod              = 1.0f;
	sampler_info.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_info, nullptr, &point_sampler));
}

void OITDepthPeeling::create_constant_buffers()
{
	scene_constants = std::make_unique<vkb::core::BufferC>(get_device(), sizeof(SceneConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void OITDepthPeeling::create_descriptors()
{
	{
		std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
		};
		VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &gather_descriptor_set_layout));
	}

	{
		std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, kLayerMaxCount),
		};
		VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &combine_descriptor_set_layout));
	}

	{
		const uint32_t num_gather_pass_combined_image_sampler = kDepthCount;
		const uint32_t num_gather_pass_uniform_buffer         = kDepthCount;

		const uint32_t num_combine_pass_combined_image_sampler = kLayerMaxCount + 1;
		const uint32_t num_combine_pass_uniform_buffer         = 1;

		const uint32_t                    num_uniform_buffer_descriptors         = num_gather_pass_uniform_buffer + num_combine_pass_uniform_buffer;
		const uint32_t                    num_combined_image_sampler_descriptors = num_gather_pass_combined_image_sampler + num_combine_pass_combined_image_sampler;
		std::vector<VkDescriptorPoolSize> pool_sizes                             = {
            vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, num_uniform_buffer_descriptors),
            vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, num_combined_image_sampler_descriptors),
        };
		const uint32_t             num_gather_descriptor_sets  = 2;
		const uint32_t             num_combine_descriptor_sets = 1;
		const uint32_t             num_descriptor_sets         = num_gather_descriptor_sets + num_combine_descriptor_sets;
		VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
		VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
	}

	{
		const VkDescriptorSetLayout layouts[kDepthCount] = {gather_descriptor_set_layout, gather_descriptor_set_layout};
		VkDescriptorSetAllocateInfo alloc_info           = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, layouts, kDepthCount);
		VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, gather_descriptor_set));
	}

	{
		VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &combine_descriptor_set_layout, 1);
		VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &combine_descriptor_set));
	}
}

void OITDepthPeeling::update_descriptors()
{
	VkDescriptorBufferInfo scene_constants_descriptor = create_descriptor(*scene_constants);

	for (uint32_t i = 0; i < kDepthCount; ++i)
	{
		VkDescriptorImageInfo depth_texture_descriptor = {};
		depth_texture_descriptor.sampler               = point_sampler;
		depth_texture_descriptor.imageView             = depth_image_view[(i + 1) % kDepthCount]->get_handle();
		depth_texture_descriptor.imageLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
		    vkb::initializers::write_descriptor_set(gather_descriptor_set[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &scene_constants_descriptor),
		    vkb::initializers::write_descriptor_set(gather_descriptor_set[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &depth_texture_descriptor),
		};
		vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
	}

	{
		std::vector<VkWriteDescriptorSet> write_descriptor_sets(3);

		write_descriptor_sets[0] = vkb::initializers::write_descriptor_set(combine_descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &scene_constants_descriptor);

		VkDescriptorImageInfo background_texture_descriptor = create_descriptor(background_texture);
		write_descriptor_sets[1]                            = vkb::initializers::write_descriptor_set(combine_descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &background_texture_descriptor);

		VkDescriptorImageInfo layer_texture_descriptor[kLayerMaxCount];
		for (uint32_t i = 0; i < kLayerMaxCount; ++i)
		{
			layer_texture_descriptor[i].sampler     = point_sampler;
			layer_texture_descriptor[i].imageView   = layer_image_view[i]->get_handle();
			layer_texture_descriptor[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		write_descriptor_sets[2] = vkb::initializers::write_descriptor_set(combine_descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, layer_texture_descriptor, kLayerMaxCount);

		vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
	}
}

void OITDepthPeeling::create_pipelines()
{
	{
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&gather_descriptor_set_layout, 1);
		VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &gather_pipeline_layout));
	}
	{
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&combine_descriptor_set_layout, 1);
		VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &combine_pipeline_layout));
	}

	{
		VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();

		VkPipelineInputAssemblyStateCreateInfo input_assembly_state = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

		VkPipelineRasterizationStateCreateInfo rasterization_state = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

		VkPipelineColorBlendAttachmentState blend_attachment_state = vkb::initializers::pipeline_color_blend_attachment_state(0xF, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo color_blend_state      = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

		VkPipelineMultisampleStateCreateInfo multisample_state = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

		VkPipelineViewportStateCreateInfo viewport_state = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

		VkPipelineDepthStencilStateCreateInfo depth_stencil_state = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER);

		std::vector<VkDynamicState>      dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		VkPipelineDynamicStateCreateInfo dynamicState          = vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables.data(), static_cast<uint32_t>(dynamic_state_enables.size()), 0);

		std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

		VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(gather_pipeline_layout, gather_render_pass, 0);
		pipeline_create_info.pVertexInputState            = &vertex_input_state;
		pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
		pipeline_create_info.pRasterizationState          = &rasterization_state;
		pipeline_create_info.pColorBlendState             = &color_blend_state;
		pipeline_create_info.pMultisampleState            = &multisample_state;
		pipeline_create_info.pViewportState               = &viewport_state;
		pipeline_create_info.pDepthStencilState           = &depth_stencil_state;
		pipeline_create_info.pDynamicState                = &dynamicState;
		pipeline_create_info.stageCount                   = static_cast<uint32_t>(shader_stages.size());
		pipeline_create_info.pStages                      = shader_stages.data();

		{
			const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
			    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
			};
			const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
			    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),
			    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)),
			};
			vertex_input_state.vertexBindingDescriptionCount   = static_cast<uint32_t>(vertex_input_bindings.size());
			vertex_input_state.pVertexBindingDescriptions      = vertex_input_bindings.data();
			vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes.size());
			vertex_input_state.pVertexAttributeDescriptions    = vertex_input_attributes.data();

			shader_stages[0] = load_shader("oit_depth_peeling/gather.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shader_stages[1] = load_shader("oit_depth_peeling/gather_first.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

			VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &gather_first_pipeline));

			shader_stages[1] = load_shader("oit_depth_peeling/gather.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &gather_pipeline));
		}

		pipeline_create_info.layout = combine_pipeline_layout;

		vertex_input_state.vertexBindingDescriptionCount   = 0;
		vertex_input_state.pVertexBindingDescriptions      = nullptr;
		vertex_input_state.vertexAttributeDescriptionCount = 0;
		vertex_input_state.pVertexAttributeDescriptions    = nullptr;

		depth_stencil_state = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_GREATER);

		pipeline_create_info.renderPass = render_pass;

		{
			shader_stages[0] = load_shader("oit_depth_peeling/fullscreen.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shader_stages[1] = load_shader("oit_depth_peeling/background.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

			VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &background_pipeline));
		}

		{
			blend_attachment_state.blendEnable         = VK_TRUE;
			blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
			blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;

			shader_stages[0] = load_shader("oit_depth_peeling/fullscreen.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shader_stages[1] = load_shader("oit_depth_peeling/combine.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

			VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &combine_pipeline));
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void OITDepthPeeling::update_scene_constants()
{
	SceneConstants constants        = {};
	constants.model_view_projection = camera.matrices.perspective * camera.matrices.view * glm::scale(glm::mat4(1.0f), glm::vec3(0.08));
	constants.background_grayscale  = background_grayscale;
	constants.object_alpha          = object_alpha;
	constants.front_layer_index     = front_layer_index;
	constants.back_layer_index      = back_layer_index;
	scene_constants->convert_and_update(constants);
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<vkb::VulkanSampleC> create_oit_depth_peeling()
{
	return std::make_unique<OITDepthPeeling>();
}
