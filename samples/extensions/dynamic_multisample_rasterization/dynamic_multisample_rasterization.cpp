/* Copyright (c) 2024, Mobica Limited
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

#include "dynamic_multisample_rasterization.h"

#include "gltf_loader.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/sub_mesh.h"

DynamicMultisampleRasterization::DynamicMultisampleRasterization()
{
	title = "DynamicState3 Multisample Rasterization";

	set_api_version(VK_API_VERSION_1_2);
	add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
	add_device_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
}

DynamicMultisampleRasterization::~DynamicMultisampleRasterization()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipeline_inversed_rasterizer, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);

		vkDestroyPipeline(get_device().get_handle(), pipeline_gui, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout_gui, nullptr);
		vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool_gui, VK_NULL_HANDLE);
		destroy_image_data(depth_stencil);
		destroy_image_data(color_attachment);
	}
}

void DynamicMultisampleRasterization::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT requested_feature                                                                                        = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT};
	requested_feature.extendedDynamicState3RasterizationSamples                                                                                               = VK_TRUE;
	gpu.request_extension_features<VkPhysicalDeviceExtendedDynamicState3FeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT) = requested_feature;

	// Dynamic Rendering
	auto &requested_dynamic_rendering            = gpu.request_extension_features<VkPhysicalDeviceDynamicRenderingFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR);
	requested_dynamic_rendering.dynamicRendering = VK_TRUE;
}

const std::string to_string(VkSampleCountFlagBits count)
{
	switch (count)
	{
		case VK_SAMPLE_COUNT_1_BIT:
			return "No MSAA";
		case VK_SAMPLE_COUNT_2_BIT:
			return "2X MSAA";
		case VK_SAMPLE_COUNT_4_BIT:
			return "4X MSAA";
		case VK_SAMPLE_COUNT_8_BIT:
			return "8X MSAA";
		case VK_SAMPLE_COUNT_16_BIT:
			return "16X MSAA";
		case VK_SAMPLE_COUNT_32_BIT:
			return "32X MSAA";
		case VK_SAMPLE_COUNT_64_BIT:
			return "64X MSAA";
		default:
			return "Unknown";
	}
}

void DynamicMultisampleRasterization::prepare_supported_sample_count_list()
{
	if (sample_count_prepared)
		return;

	VkPhysicalDeviceProperties gpu_properties;
	vkGetPhysicalDeviceProperties(get_device().get_gpu().get_handle(), &gpu_properties);

	VkSampleCountFlags supported_by_depth_and_color = gpu_properties.limits.framebufferColorSampleCounts & gpu_properties.limits.framebufferDepthSampleCounts;

	// All possible sample counts are listed here from most to least preferred as default
	// On Mali GPUs 4X MSAA is recommended as best performance/quality trade-off
	std::vector<VkSampleCountFlagBits> counts = {VK_SAMPLE_COUNT_4_BIT, VK_SAMPLE_COUNT_2_BIT, VK_SAMPLE_COUNT_8_BIT,
	                                             VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_64_BIT,
	                                             VK_SAMPLE_COUNT_1_BIT};

	std::copy_if(counts.begin(),
	             counts.end(),
	             std::back_inserter(supported_sample_count_list),
	             [&supported_by_depth_and_color](auto count) { return supported_by_depth_and_color & count; });
	std::transform(supported_sample_count_list.begin(),
	               supported_sample_count_list.end(),
	               std::back_inserter(gui_settings.sample_counts),
	               [](auto count) { return to_string(count); });
	if (!supported_sample_count_list.empty())
	{
		sample_count = supported_sample_count_list.front();
	}

	sample_count_prepared = true;
}

bool DynamicMultisampleRasterization::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(1.9f, 10.f, -18.f));
	camera.set_rotation(glm::vec3(0.f, -40.f, 0.f));
	camera.rotation_speed = 0.01f;

	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);
	load_assets();

	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_sets();

	update_resources();

	prepared = true;
	return true;
}

void DynamicMultisampleRasterization::draw_node(VkCommandBuffer &draw_cmd_buffer, SceneNode &node)
{
	assert(node.sub_mesh->vertex_buffers.count("position") == 1);
	assert(node.sub_mesh->vertex_buffers.count("normal") == 1);
	assert(node.sub_mesh->vertex_buffers.count("texcoord_0") == 1);

	const auto &vertex_buffer_pos    = node.sub_mesh->vertex_buffers.at("position");
	const auto &vertex_buffer_normal = node.sub_mesh->vertex_buffers.at("normal");
	const auto &vertex_buffer_uv     = node.sub_mesh->vertex_buffers.at("texcoord_0");
	auto       &index_buffer         = node.sub_mesh->index_buffer;

	// Pass data for the current node via push commands
	auto node_material = dynamic_cast<const vkb::sg::PBRMaterial *>(node.sub_mesh->get_material());
	assert(node_material);

	push_const_block.model_matrix = node.node->get_transform().get_world_matrix();

	push_const_block.base_color_factor    = node_material->base_color_factor;
	push_const_block.metallic_factor      = node_material->metallic_factor;
	push_const_block.roughness_factor     = node_material->roughness_factor;
	push_const_block.base_texture_index   = -1;
	push_const_block.normal_texture_index = -1;
	push_const_block.pbr_texture_index    = -1;

	auto base_color_texture = node_material->textures.find("base_color_texture");

	if (base_color_texture != node_material->textures.end())
	{
		push_const_block.base_texture_index = name_to_texture_id.at(base_color_texture->second->get_name());
	}

	auto normal_texture = node_material->textures.find("normal_texture");

	if (normal_texture != node_material->textures.end())
	{
		push_const_block.normal_texture_index = name_to_texture_id.at(normal_texture->second->get_name());
	}

	auto metallic_roughness_texture = node_material->textures.find("metallic_roughness_texture");

	if (metallic_roughness_texture != node_material->textures.end())
	{
		push_const_block.pbr_texture_index = name_to_texture_id.at(metallic_roughness_texture->second->get_name());
	}

	vkCmdPushConstants(draw_cmd_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_const_block), &push_const_block);

	VkDeviceSize offsets[1] = {0};
	vkCmdBindVertexBuffers(draw_cmd_buffer, 0, 1, vertex_buffer_pos.get(), offsets);
	vkCmdBindVertexBuffers(draw_cmd_buffer, 1, 1, vertex_buffer_normal.get(), offsets);
	vkCmdBindVertexBuffers(draw_cmd_buffer, 2, 1, vertex_buffer_uv.get(), offsets);
	vkCmdBindIndexBuffer(draw_cmd_buffer, index_buffer->get_handle(), 0, node.sub_mesh->index_type);

	vkCmdDraw(draw_cmd_buffer, node.sub_mesh->vertex_indices, 1, 0, 0);
}

void DynamicMultisampleRasterization::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	std::vector<VkClearValue> clear_values(2);
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	std::vector<VkRenderingAttachmentInfoKHR> attachments(2);
	attachments_setup(attachments, clear_values);

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

		if (sample_count != VK_SAMPLE_COUNT_1_BIT)
		{
			attachments[0].resolveImageView = swapchain_buffers[i].view;
		}
		else
		{
			attachments[0].imageView = swapchain_buffers[i].view;
		}

		VkMultisampledRenderToSingleSampledInfoEXT multisampled_render;
		if (sample_count != VK_SAMPLE_COUNT_1_BIT)
		{
			multisampled_render.multisampledRenderToSingleSampledEnable = VK_TRUE;
			multisampled_render.sType                                   = VK_STRUCTURE_TYPE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_INFO_EXT;
			multisampled_render.rasterizationSamples                    = sample_count;
			multisampled_render.pNext                                   = nullptr;
		}

		auto render_area             = VkRect2D{VkOffset2D{}, VkExtent2D{width, height}};
		auto render_info             = vkb::initializers::rendering_info(render_area, 1, &attachments[0]);
		render_info.layerCount       = 1;
		render_info.pDepthAttachment = &attachments[1];
		render_info.pNext            = (sample_count != VK_SAMPLE_COUNT_1_BIT) ? &multisampled_render : VK_NULL_HANDLE;

		vkb::image_layout_transition(draw_cmd_buffers[i],
		                             swapchain_buffers[i].image,
		                             VK_IMAGE_LAYOUT_UNDEFINED,
		                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		                             range);

		vkb::image_layout_transition(draw_cmd_buffers[i],
		                             depth_stencil.image,
		                             VK_IMAGE_LAYOUT_UNDEFINED,
		                             VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		                             depth_range);

		vkCmdBeginRenderingKHR(draw_cmd_buffers[i], &render_info);
		vkCmdSetRasterizationSamplesEXT(draw_cmd_buffers[i], sample_count);        // VK_EXT_extended_dynamic_state3

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		for (auto &node : scene_nodes_opaque)
		{
			draw_node(draw_cmd_buffers[i], node);
		}

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_inversed_rasterizer);

		for (auto &node : scene_nodes_opaque_flipped)
		{
			draw_node(draw_cmd_buffers[i], node);
		}

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		for (auto &node : scene_nodes_transparent)
		{
			draw_node(draw_cmd_buffers[i], node);
		}

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_inversed_rasterizer);

		for (auto &node : scene_nodes_transparent_flipped)
		{
			draw_node(draw_cmd_buffers[i], node);
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderingKHR(draw_cmd_buffers[i]);

		vkb::image_layout_transition(draw_cmd_buffers[i],
		                             swapchain_buffers[i].image,
		                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		                             range);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void DynamicMultisampleRasterization::draw_ui(VkCommandBuffer &cmd_buffer)
{
	if (gui)
	{
		gui->draw(cmd_buffer, pipeline_gui, pipeline_layout_gui, descriptor_set_gui);
	}
}

void DynamicMultisampleRasterization::load_assets()
{
	vkb::GLTFLoader loader{get_device()};
	scene = loader.read_scene_from_file("scenes/space_module/SpaceModule.gltf");
	assert(scene);
	// Store all scene nodes in separate vectors for easier rendering
	for (auto &mesh : scene->get_components<vkb::sg::Mesh>())
	{
		for (auto &node : mesh->get_nodes())
		{
			for (auto &sub_mesh : mesh->get_submeshes())
			{
				auto &scale = node->get_transform().get_scale();

				bool flipped     = scale.x * scale.y * scale.z < 0;
				bool transparent = sub_mesh->get_material()->alpha_mode == vkb::sg::AlphaMode::Blend;

				if (transparent)        // transparent material
				{
					if (flipped)
						scene_nodes_transparent_flipped.push_back({node, sub_mesh});
					else
						scene_nodes_transparent.push_back({node, sub_mesh});
				}
				else        // opaque material
				{
					if (flipped)
						scene_nodes_opaque_flipped.push_back({node, sub_mesh});
					else
						scene_nodes_opaque.push_back({node, sub_mesh});
				}
			}
		}
	}

	auto textures = scene->get_components<vkb::sg::Texture>();

	for (auto texture : textures)
	{
		const auto           &name  = texture->get_name();
		auto                  image = texture->get_image();
		VkDescriptorImageInfo imageInfo;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView   = image->get_vk_image_view().get_handle();
		imageInfo.sampler     = texture->get_sampler()->vk_sampler.get_handle();

		image_infos.push_back(imageInfo);
		name_to_texture_id.emplace(name, static_cast<int32_t>(image_infos.size()) - 1);
	}
}

void DynamicMultisampleRasterization::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(image_infos.size()))};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), 2);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void DynamicMultisampleRasterization::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, static_cast<uint32_t>(image_infos.size())),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layout,
	        1);

	// Pass scene node information via push constants
	VkPushConstantRange push_constant_range            = vkb::initializers::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, sizeof(push_const_block), 0);
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void DynamicMultisampleRasterization::setup_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo matrix_buffer_descriptor = create_descriptor(*uniform_buffer);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, image_infos.data(), image_infos.size())};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void DynamicMultisampleRasterization::attachments_setup(std::vector<VkRenderingAttachmentInfoKHR> &attachments, std::vector<VkClearValue> &clear_values)
{
	prepare_supported_sample_count_list();
	destroy_image_data(color_attachment);
	destroy_image_data(depth_stencil);
	setup_color_attachment();
	setup_depth_stencil();

	attachments[0].sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachments[0].imageView        = color_attachment.view;
	attachments[0].imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].clearValue       = clear_values[0];
	attachments[0].pNext            = VK_NULL_HANDLE;
	attachments[0].resolveImageView = VK_NULL_HANDLE;

	if (sample_count != VK_SAMPLE_COUNT_1_BIT)
	{
		attachments[0].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].resolveMode        = VK_RESOLVE_MODE_AVERAGE_BIT;
	}
	else
	{
		attachments[0].resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].resolveMode        = VK_RESOLVE_MODE_NONE;
	}

	// Depth attachment
	attachments[1].sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachments[1].imageView   = depth_stencil.view;
	attachments[1].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].clearValue  = clear_values[1];
	attachments[1].pNext       = VK_NULL_HANDLE;
}

// Create attachment that will be used in a dynamic rendering.
void DynamicMultisampleRasterization::setup_color_attachment()
{
	VkImageCreateInfo image_create_info{};
	image_create_info.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType   = VK_IMAGE_TYPE_2D;
	image_create_info.format      = render_context->get_format();
	image_create_info.extent      = {get_render_context().get_surface_extent().width, get_render_context().get_surface_extent().height, 1};
	image_create_info.mipLevels   = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples     = sample_count;
	image_create_info.tiling      = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.usage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

	VK_CHECK(vkCreateImage(device->get_handle(), &image_create_info, nullptr, &color_attachment.image));
	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device->get_handle(), color_attachment.image, &memReqs);

	VkMemoryAllocateInfo memory_allocation{};
	memory_allocation.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocation.allocationSize  = memReqs.size;
	memory_allocation.memoryTypeIndex = device->get_memory_type(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(device->get_handle(), &memory_allocation, nullptr, &color_attachment.mem));
	VK_CHECK(vkBindImageMemory(device->get_handle(), color_attachment.image, color_attachment.mem, 0));

	VkImageViewCreateInfo image_view_create_info{};
	image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.image                           = color_attachment.image;
	image_view_create_info.format                          = render_context->get_format();
	image_view_create_info.subresourceRange.baseMipLevel   = 0;
	image_view_create_info.subresourceRange.levelCount     = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount     = 1;
	image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
	if (depth_format >= VK_FORMAT_D16_UNORM_S8_UINT)
	{
		image_view_create_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	VK_CHECK(vkCreateImageView(device->get_handle(), &image_view_create_info, nullptr, &color_attachment.view));
}

void DynamicMultisampleRasterization::setup_depth_stencil()
{
	VkImageCreateInfo image_create_info{};
	image_create_info.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType   = VK_IMAGE_TYPE_2D;
	image_create_info.format      = depth_format;
	image_create_info.extent      = {get_render_context().get_surface_extent().width, get_render_context().get_surface_extent().height, 1};
	image_create_info.mipLevels   = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples     = sample_count;
	image_create_info.tiling      = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	VK_CHECK(vkCreateImage(device->get_handle(), &image_create_info, nullptr, &depth_stencil.image));
	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device->get_handle(), depth_stencil.image, &memReqs);

	VkMemoryAllocateInfo memory_allocation{};
	memory_allocation.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocation.allocationSize  = memReqs.size;
	memory_allocation.memoryTypeIndex = device->get_memory_type(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(device->get_handle(), &memory_allocation, nullptr, &depth_stencil.mem));
	VK_CHECK(vkBindImageMemory(device->get_handle(), depth_stencil.image, depth_stencil.mem, 0));

	VkImageViewCreateInfo image_view_create_info{};
	image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.image                           = depth_stencil.image;
	image_view_create_info.format                          = depth_format;
	image_view_create_info.subresourceRange.baseMipLevel   = 0;
	image_view_create_info.subresourceRange.levelCount     = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount     = 1;
	image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
	// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
	if (depth_format >= VK_FORMAT_D16_UNORM_S8_UINT)
	{
		image_view_create_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	VK_CHECK(vkCreateImageView(device->get_handle(), &image_view_create_info, nullptr, &depth_stencil.view));
}

void DynamicMultisampleRasterization::prepare_pipelines()
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
	        VK_COLOR_COMPONENT_R_BIT |
	            VK_COLOR_COMPONENT_G_BIT |
	            VK_COLOR_COMPONENT_B_BIT |
	            VK_COLOR_COMPONENT_A_BIT,
	        VK_TRUE);

	blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE,
	        VK_TRUE,
	        VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,        // disable multisampling during configuration
	        0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_RASTERIZATION_SAMPLES_EXT /* VK_EXT_extended_dynamic_state3 */
	};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(
	        pipeline_layout,
	        VK_NULL_HANDLE,
	        0);

	std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_states = {
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE)};

	// Create graphics pipeline for dynamic rendering
	VkFormat color_rendering_format = render_context->get_format();

	// Provide information for dynamic rendering
	VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
	pipeline_rendering_create_info.pNext                   = VK_NULL_HANDLE;
	pipeline_rendering_create_info.colorAttachmentCount    = 1;
	pipeline_rendering_create_info.pColorAttachmentFormats = &color_rendering_format;
	pipeline_rendering_create_info.depthAttachmentFormat   = depth_format;

	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.layout              = pipeline_layout;
	pipeline_create_info.pNext               = &pipeline_rendering_create_info;

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages    = shader_stages.data();

	// Vertex bindings an attributes for model rendering
	// Binding description, we use separate buffers for the vertex attributes
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(2, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	// Attribute descriptions
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Position
	    vkb::initializers::vertex_input_attribute_description(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),        // Normal
	    vkb::initializers::vertex_input_attribute_description(2, 2, VK_FORMAT_R32G32_SFLOAT, 0),           // TexCoord
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	pipeline_create_info.pVertexInputState = &vertex_input_state;

	shader_stages[0] = load_shader("dynamic_multisample_rasterization/model.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("dynamic_multisample_rasterization/model.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));

	// Add another pipeline since parts of the scene have to be rendered using VK_FRONT_FACE_CLOCKWISE
	VkPipelineRasterizationStateCreateInfo rasterization_state_inversed_rasterizer =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_BACK_BIT,
	        VK_FRONT_FACE_CLOCKWISE,
	        0);

	pipeline_create_info.pRasterizationState = &rasterization_state_inversed_rasterizer;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline_inversed_rasterizer));
}

void DynamicMultisampleRasterization::prepare_gui_pipeline()
{
	// Descriptor pool
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};
	VkDescriptorPoolCreateInfo descriptorPoolInfo = vkb::initializers::descriptor_pool_create_info(pool_sizes, 2);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptorPoolInfo, nullptr, &descriptor_pool_gui));

	// Descriptor set layout
	std::vector<VkDescriptorSetLayoutBinding> layout_bindings_gui = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	};
	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(layout_bindings_gui);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout_gui));

	// Descriptor set
	VkDescriptorSetAllocateInfo descriptor_allocation = vkb::initializers::descriptor_set_allocate_info(descriptor_pool_gui, &descriptor_set_layout_gui, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_allocation, &descriptor_set_gui));
	VkDescriptorImageInfo font_descriptor = vkb::initializers::descriptor_image_info(
	    gui->get_sampler(),
	    gui->get_font_image_view(),
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_set_gui, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &font_descriptor)};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

	// Setup graphics pipeline for UI rendering
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	// Enable blending
	VkPipelineColorBlendAttachmentState blend_attachment_state{};
	blend_attachment_state.blendEnable         = VK_TRUE;
	blend_attachment_state.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_RASTERIZATION_SAMPLES_EXT /* VK_EXT_extended_dynamic_state3 */
	};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables);

	std::vector<vkb::ShaderModule *> shader_modules;

	vkb::ShaderSource vert_shader("uioverlay/uioverlay.vert");
	vkb::ShaderSource frag_shader("uioverlay/uioverlay.frag");

	shader_modules.push_back(&get_device().get_resource_cache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, vert_shader, {}));
	shader_modules.push_back(&get_device().get_resource_cache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader, {}));

	pipeline_layout_gui = get_device().get_resource_cache().request_pipeline_layout(shader_modules).get_handle();

	// Create graphics pipeline for dynamic rendering
	VkFormat color_rendering_format = render_context->get_format();

	// Provide information for dynamic rendering
	VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
	pipeline_rendering_create_info.pNext                   = VK_NULL_HANDLE;
	pipeline_rendering_create_info.colorAttachmentCount    = 1;
	pipeline_rendering_create_info.pColorAttachmentFormats = &color_rendering_format;
	pipeline_rendering_create_info.depthAttachmentFormat   = depth_format;
	if (!vkb::is_depth_only_format(depth_format))
	{
		pipeline_rendering_create_info.stencilAttachmentFormat = depth_format;
	}

	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout_gui, VK_NULL_HANDLE);

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

	shader_stages[0] = load_shader(vert_shader.get_filename(), VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader(frag_shader.get_filename(), VK_SHADER_STAGE_FRAGMENT_BIT);

	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages    = shader_stages.data();

	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.stageCount          = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();
	pipeline_create_info.pNext               = &pipeline_rendering_create_info;

	// Vertex bindings an attributes based on ImGui vertex definition
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),         // Location 0: Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),          // Location 1: UV
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)),        // Location 0: Color
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state_create_info.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state_create_info.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state_create_info.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state_create_info.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline_gui));
}

// Prepare and initialize uniform buffer containing shader uniforms
void DynamicMultisampleRasterization::prepare_uniform_buffers()
{
	// Matrices vertex shader uniform buffer
	uniform_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                     sizeof(uniform_data),
	                                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                     VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void DynamicMultisampleRasterization::prepare_gui()
{
	gui = std::make_unique<vkb::Gui>(*this, *window, /*stats=*/nullptr, 15.0f, true);

	prepare_gui_pipeline();

	// No need to call gui->prepare because the pipeline has been created above
}

void DynamicMultisampleRasterization::update_uniform_buffers()
{
	uniform_data.projection = camera.matrices.perspective;
	// Scale the view matrix as the model is pretty large, and also flip it upside down
	uniform_data.view = glm::scale(camera.matrices.view, glm::vec3(0.1f, -0.1f, 0.1f));
	uniform_buffer->convert_and_update(uniform_data);
}

void DynamicMultisampleRasterization::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void DynamicMultisampleRasterization::render(float delta_time)
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

void DynamicMultisampleRasterization::destroy_image_data(ImageData &image_data)
{
	if (image_data.image != nullptr)
	{
		vkDestroyImageView(get_device().get_handle(), image_data.view, nullptr);
		vkDestroyImage(get_device().get_handle(), image_data.image, nullptr);
		vkFreeMemory(get_device().get_handle(), image_data.mem, nullptr);
		image_data.view  = VK_NULL_HANDLE;
		image_data.image = VK_NULL_HANDLE;
		image_data.mem   = VK_NULL_HANDLE;
	}
}

void DynamicMultisampleRasterization::update_resources()
{
	prepared = false;

	if (device)
	{
		device->wait_idle();
		rebuild_command_buffers();
	}

	prepared = true;
}

void DynamicMultisampleRasterization::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.combo_box("antialiasing", &gui_settings.sample_count_index, gui_settings.sample_counts))
		{
			sample_count = supported_sample_count_list[gui_settings.sample_count_index];

			update_resources();
		}
	}
}

bool DynamicMultisampleRasterization::resize(const uint32_t _width, const uint32_t _height)
{
	sample_count = VK_SAMPLE_COUNT_1_BIT;

	if (!ApiVulkanSample::resize(_width, _height))
		return false;

	sample_count = supported_sample_count_list[gui_settings.sample_count_index];

	prepared = false;

	update_resources();

	prepared = true;
	return true;
}

std::unique_ptr<vkb::VulkanSample> create_dynamic_multisample_rasterization()
{
	return std::make_unique<DynamicMultisampleRasterization>();
}