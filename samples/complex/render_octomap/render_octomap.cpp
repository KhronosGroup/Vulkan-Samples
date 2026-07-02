/* Copyright (c) 2024-2026, Holochip Inc.
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

#include "render_octomap.h"
#include "core/util/logging.hpp"
#include "filesystem/legacy.h"
#include "gltf_loader.h"
#include "octomap/octomap.h"
#include "platform/input_events.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/sub_mesh.h"
#include "scene_graph/scene.h"

#include <algorithm>
#include <filesystem>
#include <tiny_gltf.h>

// KHR_gaussian_splatting extension name
#define GLTF_KHR_GAUSSIAN_SPLATTING_EXTENSION "KHR_gaussian_splatting"

RenderOctomap::RenderOctomap()
{
	title = "Octomap Viewer";
	map   = new octomap::OcTree(0.1f);
}
RenderOctomap::~RenderOctomap()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		vkDestroyPipeline(get_device().get_handle(), gltf_pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), gltf_pipeline_layout, nullptr);
		vkDestroyPipeline(get_device().get_handle(), splat_pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), splat_pipeline_layout, nullptr);
		vkDestroyDescriptorPool(get_device().get_handle(), splat_descriptor_pool, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), splat_descriptor_set_layout, nullptr);
		vkDestroyPipelineCache(get_device().get_handle(), pipeline_cache, nullptr);
		vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, nullptr);
	}
	delete map;
	map = nullptr;
}
// Standard HSV→RGB conversion (h in [0,1], s and v in [0,1]).
static glm::vec3 hsv_to_rgb(float h, float s, float v)
{
	h -= std::floor(h);
	h *= 6.0f;
	int   i = static_cast<int>(std::floor(h));
	float f = h - static_cast<float>(i);
	if (!(i & 1))
	{
		f = 1.0f - f;
	}
	float m = v * (1.0f - s);
	float n = v * (1.0f - s * f);
	switch (i)
	{
		case 6:
		case 0:
			return {v, n, m};
		case 1:
			return {n, v, m};
		case 2:
			return {m, v, n};
		case 3:
			return {m, n, v};
		case 4:
			return {n, m, v};
		case 5:
			return {v, m, n};
		default:
			return {1.0f, 0.5f, 0.5f};
	}
}

void RenderOctomap::build_cubes()
{
	const octomap::OcTree *tree = map;
	if (tree->size() == 0)
	{
		return;
	}
	// Only rebuild the instance buffer once the map has grown by at least 5%
	// since the last build, so we don't rebuild every frame for tiny updates.
	float next_build_size = static_cast<float>(last_map_build_size) * 1.05f;
	if (static_cast<float>(tree->size()) < next_build_size)
	{
		return;
	}

	double min_x, min_y, min_z, max_x, max_y, max_z;
	tree->getMetricMin(min_x, min_y, min_z);
	tree->getMetricMax(max_x, max_y, max_z);

	// set min/max Z for color height map
	z_min = static_cast<float>(min_z);
	z_max = static_cast<float>(max_z);

	// Hue uses 80% of the [0,1] range so high-altitude voxels don't wrap back to red.
	const float z_range = z_max - z_min;

	instances.clear();
	for (auto it = tree->begin_tree(max_tree_depth), end = tree->end_tree(); it != end; ++it)
	{
		if (it.isLeaf() && tree->isNodeOccupied(*it))
		{
			glm::vec3 coords = {it.getCoordinate().x(), it.getCoordinate().y(), it.getCoordinate().z()};
			coords.y *= -1;
			InstanceData instance;
			instance.pos[0] = coords[0];
			instance.pos[1] = coords[1];
			instance.pos[2] = coords[2];

			float h;
			if (z_range <= 0.0f)
			{
				h = 0.5f;
			}
			else
			{
				h = (1.0f - std::clamp((coords[2] - z_min) / z_range, 0.0f, 1.0f)) * 0.8f;
			}

			glm::vec3 rgb   = hsv_to_rgb(h, 1.0f, 1.0f);
			instance.col[0] = rgb.r;
			instance.col[1] = rgb.g;
			instance.col[2] = rgb.b;
			instance.col[3] = 1.0f;
			instance.scale  = static_cast<float>(it.getSize());
			instances.push_back(instance);
		}
	}        // end for all voxels
	// Create buffers
	if (!instances.empty())
	{
		auto staging    = vkb::core::BufferC::create_staging_buffer(get_device(), instances);
		instance_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
		                                                       instances.size() * sizeof(InstanceData),
		                                                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		                                                       VMA_MEMORY_USAGE_GPU_ONLY);
		with_command_buffer([&](VkCommandBuffer cmd) {
			VkBufferCopy copy{};
			copy.size = instances.size() * sizeof(InstanceData);
			vkCmdCopyBuffer(cmd, staging.get_handle(), instance_buffer->get_handle(), 1, &copy);
		});
	}

	last_build_time     = std::chrono::system_clock::now();
	last_map_build_size = tree->size();
}

void RenderOctomap::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clear_values[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		// Set target frame buffer
		render_pass_begin_info.framebuffer = framebuffers[i];
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);
		VkRect2D scissor_rect = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor_rect);

		VkDeviceSize offsets[1] = {0};
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
		                        &descriptor_set, 0, nullptr);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, &vertex_buffer->get_handle(), offsets);
		// instance_buffer is only allocated once at least one occupied voxel has been
		// found (see build_cubes()); skip the draw until then (e.g. map failed to load).
		if (instance_buffer && index_buffer)
		{
			vkCmdBindVertexBuffers(draw_cmd_buffers[i], 1, 1, &instance_buffer->get_handle(), offsets);
			vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(draw_cmd_buffers[i], index_count, static_cast<uint32_t>(instances.size()), 0, 0, 0);
		}

		// Draw the framework's ImGui overlay (view-mode buttons) on top.
		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}
bool RenderOctomap::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}
	// Enable framework WASD movement (implemented only for `CameraType::FirstPerson`).
	camera.type = vkb::CameraType::FirstPerson;
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 256.0f);
	camera.set_rotation({0.0f, 0.0f, 0.0f});
	camera.set_translation({0.0f, 0.0f, -1.0f});
	std::string octomap_path = vkb::fs::path::get(vkb::fs::path::Type::Assets, "scenes/octmap_and_splats/octMap.bin");
	map->readBinary(octomap_path);
	build_cubes();
	create_octomap_pipeline(render_pass);
	build_command_buffers();
	prepared = true;
	return true;
}

void RenderOctomap::create_octomap_pipeline(VkRenderPass renderPass)
{
	setup_vertex_descriptions();
	prepare_ubo();
	auto input_assembly_state   = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	auto raster_state           = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	auto blend_attachment_state = vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
	auto color_blend_state      = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);
	auto depth_stencil_state    = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	auto viewport_state         = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);
	auto multisample_state      = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	// Only VIEWPORT/SCISSOR are used by this pipeline; polygon mode is FILL, so there is
	// no line width to set dynamically.
	std::vector dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};

	auto dynamic_state = vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables.data(), dynamic_state_enables.size(), 0);

	VkPipelineCacheCreateInfo pipeline_cache_create_info = {};
	pipeline_cache_create_info.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VK_CHECK(vkCreatePipelineCache(get_device().get_handle(), &pipeline_cache_create_info, nullptr, &pipeline_cache));

	// Rendering pipeline
	std::vector pool_sizes = {
	    // Graphics pipelines uniform buffers
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2)};
	VkDescriptorPoolCreateInfo descriptor_pool_info = vkb::initializers::descriptor_pool_create_info(pool_sizes, 3);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_info, nullptr, &descriptor_pool));

	std::vector set_layout_bindings = {
	    // Binding 0: Vertex shader uniform buffer
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)};

	auto descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layout));

	auto pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));

	// Load shaders
	std::vector shader_stages = {
	    load_shader("render_octomap", "render.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
	    load_shader("render_octomap", "render.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

	auto pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout, renderPass, 0);

	pipeline_create_info.pVertexInputState   = &vertices.input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &raster_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.stageCount          = shader_stages.size();
	pipeline_create_info.pStages             = shader_stages.data();
	pipeline_create_info.renderPass          = renderPass;

	VK_CHECK(
	    vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));

	auto alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));
	VkDescriptorBufferInfo            buffer_descriptor     = create_descriptor(*uniform_buffer_vs);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
	                                            &buffer_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), write_descriptor_sets.size(), write_descriptor_sets.data(), 0, nullptr);
}

void RenderOctomap::create_gltf_pipeline(VkRenderPass renderPass)
{
	if (gltf_pipeline != VK_NULL_HANDLE)
	{
		return;
	}

	// Pipeline layout: reuse existing descriptor set layout (binding 0 UBO) and add push constants.
	struct GltfPushConstants
	{
		glm::mat4 model;
		glm::vec4 color;
	};

	VkPushConstantRange push_constant_range{};
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	push_constant_range.offset     = 0;
	push_constant_range.size       = sizeof(GltfPushConstants);

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);
	pipeline_layout_create_info.pushConstantRangeCount     = 1;
	pipeline_layout_create_info.pPushConstantRanges        = &push_constant_range;
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &gltf_pipeline_layout));

	// Shaders
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
	    load_shader("render_octomap", "gltf.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
	    load_shader("render_octomap", "gltf.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

	// Vertex input: single interleaved binding with POSITION (VEC3, 12 bytes) + COLOR_0 (VEC4, 16 bytes) = stride 28
	std::vector<VkVertexInputBindingDescription> bindings = {
	    vkb::initializers::vertex_input_binding_description(0, 28, VK_VERTEX_INPUT_RATE_VERTEX)};
	std::vector<VkVertexInputAttributeDescription> attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),             // POSITION at offset 0
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 12)};        // COLOR_0 at offset 12

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = attributes.data();

	auto input_assembly_state = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	auto raster_state         = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
	                                                                                        VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	auto blend_attachment     = vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
	auto color_blend_state    = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment);
	auto depth_stencil_state  = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	auto viewport_state       = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);
	auto multisample_state    = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector<VkDynamicState> dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	auto                        dynamic_state         = vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables.data(), dynamic_state_enables.size(), 0);

	auto pipeline_create_info                = vkb::initializers::pipeline_create_info(gltf_pipeline_layout, renderPass, 0);
	pipeline_create_info.pVertexInputState   = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &raster_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.stageCount          = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &gltf_pipeline));
}

void RenderOctomap::create_splat_pipeline(VkRenderPass renderPass)
{
	if (splat_pipeline != VK_NULL_HANDLE)
	{
		return;
	}

	// Descriptor set for splat UBO
	{
		std::vector<VkDescriptorPoolSize> pool_sizes = {
		    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)};
		VkDescriptorPoolCreateInfo pool_info = vkb::initializers::descriptor_pool_create_info(pool_sizes, 1);
		VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &pool_info, nullptr, &splat_descriptor_pool));

		std::vector<VkDescriptorSetLayoutBinding> bindings = {
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)};
		VkDescriptorSetLayoutCreateInfo layout_info = vkb::initializers::descriptor_set_layout_create_info(bindings);
		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &layout_info, nullptr, &splat_descriptor_set_layout));

		VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(splat_descriptor_pool, &splat_descriptor_set_layout, 1);
		VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &splat_descriptor_set));

		VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*splat_uniform_buffer);
		VkWriteDescriptorSet   write             = vkb::initializers::write_descriptor_set(splat_descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &buffer_descriptor);
		vkUpdateDescriptorSets(get_device().get_handle(), 1, &write, 0, nullptr);
	}

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&splat_descriptor_set_layout, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &splat_pipeline_layout));

	std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
	    load_shader("render_octomap", "splat.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
	    load_shader("render_octomap", "splat.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

	// Vertex input: per-instance splat attributes
	std::vector<VkVertexInputBindingDescription> bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(SplatInstance), VK_VERTEX_INPUT_RATE_INSTANCE)};
	std::vector<VkVertexInputAttributeDescription> attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                           // pos
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 3),        // rot
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 7),           // scale
	    vkb::initializers::vertex_input_attribute_description(0, 3, VK_FORMAT_R32_SFLOAT, sizeof(float) * 10),                // opacity
	    vkb::initializers::vertex_input_attribute_description(0, 4, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 11),          // color
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = attributes.data();

	auto input_assembly_state = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0, VK_FALSE);
	auto raster_state         = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
	                                                                                        VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

	// Premultiplied alpha blending
	VkPipelineColorBlendAttachmentState blend_attachment{};
	blend_attachment.colorWriteMask      = 0xf;
	blend_attachment.blendEnable         = VK_TRUE;
	blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;

	auto color_blend_state   = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment);
	auto depth_stencil_state = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
	auto viewport_state      = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);
	auto multisample_state   = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector<VkDynamicState> dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	auto                        dynamic_state         = vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables.data(), dynamic_state_enables.size(), 0);

	auto pipeline_create_info                = vkb::initializers::pipeline_create_info(splat_pipeline_layout, renderPass, 0);
	pipeline_create_info.pVertexInputState   = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &raster_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.stageCount          = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &splat_pipeline));
}

void RenderOctomap::prepare_ubo()
{
	// Vertex shader uniform buffer block
	uniform_buffer_vs = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                         sizeof(uboVS),
	                                                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                         VMA_MEMORY_USAGE_CPU_TO_GPU);
	update_ubo();
}

void RenderOctomap::update_ubo()
{
	uboVS.projection = camera.matrices.perspective;
	uboVS.camera     = camera.matrices.view;

	uniform_buffer_vs->convert_and_update(uboVS);
}

// This just gives the first vertex_buffer
void RenderOctomap::generate_master_cube()
{
	// Setup vertices for a single quad made from two triangles
	std::vector<Vertex> local_vertices =
	    {
	        {{0.5f, 0.5f, 0.5f}},
	        {{0.5f, 0.5f, -0.5f}},
	        {{0.5f, -0.5f, 0.5f}},
	        {{0.5f, -0.5f, -0.5f}},
	        {{-0.5f, 0.5f, 0.5f}},
	        {{-0.5f, 0.5f, -0.5f}},
	        {{-0.5f, -0.5f, 0.5f}},
	        {{-0.5f, -0.5f, -0.5f}}};

	// Setup indices - counter-clockwise winding for all outward-facing triangles
	// Vertices: 0=(+,+,+), 1=(+,+,-), 2=(+,-,+), 3=(+,-,-), 4=(-,+,+), 5=(-,+,-), 6=(-,-,+), 7=(-,-,-)
	std::vector<uint32_t> indices = {
	    // Right face (+X) - looking from +X toward origin
	    0, 2, 3, 3, 1, 0,
	    // Left face (-X) - looking from -X toward origin
	    4, 5, 7, 7, 6, 4,

	    // Top face (+Y) - looking from +Y toward origin
	    0, 1, 5, 5, 4, 0,
	    // Bottom face (-Y) - looking from -Y toward origin
	    2, 6, 7, 7, 3, 2,

	    // Back face (+Z) - looking from +Z toward origin
	    0, 4, 6, 6, 2, 0,
	    // Front face (-Z) - looking from -Z toward origin
	    1, 3, 7, 7, 5, 1};
	index_count = static_cast<uint32_t>(indices.size());

	// Vertex buffer — static geometry, upload once via staging
	{
		auto staging  = vkb::core::BufferC::create_staging_buffer(get_device(), local_vertices);
		vertex_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
		                                                     local_vertices.size() * sizeof(Vertex),
		                                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		                                                     VMA_MEMORY_USAGE_GPU_ONLY);
		with_command_buffer([&](VkCommandBuffer cmd) {
			VkBufferCopy copy{};
			copy.size = local_vertices.size() * sizeof(Vertex);
			vkCmdCopyBuffer(cmd, staging.get_handle(), vertex_buffer->get_handle(), 1, &copy);
		});
	}

	// Index buffer — static geometry, upload once via staging
	{
		auto staging = vkb::core::BufferC::create_staging_buffer(get_device(), indices);

		index_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
		                                                    indices.size() * sizeof(uint32_t),
		                                                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		                                                    VMA_MEMORY_USAGE_GPU_ONLY);
		with_command_buffer([&](VkCommandBuffer cmd) {
			VkBufferCopy copy{};
			copy.size = indices.size() * sizeof(uint32_t);
			vkCmdCopyBuffer(cmd, staging.get_handle(), index_buffer->get_handle(), 1, &copy);
		});
	}
}

void RenderOctomap::setup_vertex_descriptions()
{
	generate_master_cube();
	// Binding description
	vertices.binding_descriptions = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)};

	// Attribute descriptions
	// Describes memory layout and shader positions
	vertices.attribute_descriptions = {
	    // Location 0: Position
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),

	    // Per-Instance attributes
	    vkb::initializers::vertex_input_attribute_description(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),                           // Location 1: Position
	    vkb::initializers::vertex_input_attribute_description(1, 2, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 3),        // Location 2: Color
	    vkb::initializers::vertex_input_attribute_description(1, 3, VK_FORMAT_R32_SFLOAT, sizeof(float) * 7),                 // Location 3: Scale
	};

	// Assign to vertex buffer
	vertices.input_state                                 = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertices.input_state.vertexBindingDescriptionCount   = vertices.binding_descriptions.size();
	vertices.input_state.pVertexBindingDescriptions      = vertices.binding_descriptions.data();
	vertices.input_state.vertexAttributeDescriptionCount = vertices.attribute_descriptions.size();
	vertices.input_state.pVertexAttributeDescriptions    = vertices.attribute_descriptions.data();
}
bool RenderOctomap::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	rebuild_command_buffers();
	return true;
}

void RenderOctomap::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.button("OCTOMAP"))
	{
		on_view_state_changed(ViewState::Octomap);
	}
	if (drawer.button("GLTF MAP"))
	{
		on_view_state_changed(ViewState::GLTFRegular);
	}
	if (drawer.button("SPLATS"))
	{
		on_view_state_changed(ViewState::GLTFSplats);
	}
}

void RenderOctomap::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	ApiVulkanSample::prepare_frame();

	// Update camera movement based on keyboard input (WASD)
	camera.update(delta_time);

	// Apply numpad look: KP_4/6 = yaw left/right, KP_8/2 = pitch up/down
	{
		const float look_speed = 60.0f * delta_time;        // degrees per second
		glm::vec3   delta(0.0f);
		if (numpad_look.up)
			delta.x -= look_speed;
		if (numpad_look.down)
			delta.x += look_speed;
		if (numpad_look.left)
			delta.y += look_speed;
		if (numpad_look.right)
			delta.y -= look_speed;
		if (delta != glm::vec3(0.0f))
		{
			camera.rotate(delta);
		}
	}

	// (Re)build 3D instance data and UBOs before recording.
	if (!paused || camera.updated)
	{
		update_ubo();
	}
	build_cubes();

	// Record only the current command buffer (safe per-frame path).
	recreate_current_command_buffer();
	auto cmd         = draw_cmd_buffers[current_buffer];
	auto begin_info  = vkb::initializers::command_buffer_begin_info();
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clear_values[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;
	render_pass_begin_info.framebuffer              = framebuffers[current_buffer];

	vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	// Draw the 3D map fullscreen; the ImGui overlay is drawn on top afterwards.
	VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor_rect = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
	vkCmdSetScissor(cmd, 0, 1, &scissor_rect);

	VkDeviceSize offsets[2] = {0, 0};

	switch (current_view_state)
	{
		case ViewState::Octomap:
		{
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buffer->get_handle(), offsets);
			if (instance_buffer && index_buffer)
			{
				vkCmdBindVertexBuffers(cmd, 1, 1, &instance_buffer->get_handle(), offsets);
				vkCmdBindIndexBuffer(cmd, index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(cmd, index_count, static_cast<uint32_t>(instances.size()), 0, 0, 0);
			}
			break;
		}

		case ViewState::GLTFRegular:
		{
			if (gltf_pipeline == VK_NULL_HANDLE)
			{
				create_gltf_pipeline(render_pass);
			}
			if (gltf_pipeline == VK_NULL_HANDLE || gltf_nodes.empty())
			{
				break;
			}

			struct GltfPushConstants
			{
				glm::mat4 model;
				glm::vec4 color;
			} pc;

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gltf_pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gltf_pipeline);

			for (auto &d : gltf_nodes)
			{
				if (!d.node || !d.sub_mesh)
				{
					continue;
				}

				auto pos_it = d.sub_mesh->vertex_buffers.find("position");
				if (pos_it == d.sub_mesh->vertex_buffers.end())
				{
					continue;
				}
				VkBuffer pos_buf = pos_it->second.get_handle();

				// Bind single interleaved buffer (contains POSITION + COLOR_0)
				vkCmdBindVertexBuffers(cmd, 0, 1, &pos_buf, offsets);

				// Try to get color from material, otherwise use white
				const auto *mat = dynamic_cast<const vkb::sg::PBRMaterial *>(d.sub_mesh->get_material());
				glm::vec4   col = mat ? mat->base_color_factor : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

				// Flip Y to match the octomap coordinate convention (which applies coords.y *= -1).
				// GLTF is Y-up; our world space expects Y-down.
				static const glm::mat4 kFlipY = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));
				pc.model                      = kFlipY * d.node->get_transform().get_world_matrix();
				pc.color                      = col;
				vkCmdPushConstants(cmd, gltf_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GltfPushConstants), &pc);

				if (d.sub_mesh->index_buffer)
				{
					vkCmdBindIndexBuffer(cmd, d.sub_mesh->index_buffer->get_handle(), 0, d.sub_mesh->index_type);
					vkCmdDrawIndexed(cmd, d.sub_mesh->vertex_indices, 1, 0, 0, 0);
				}
			}
			break;
		}

		case ViewState::GLTFSplats:
		{
			if (!splat_instance_buffer || splat_count == 0)
			{
				break;
			}
			if (!splat_uniform_buffer)
			{
				break;
			}
			if (splat_pipeline == VK_NULL_HANDLE)
			{
				create_splat_pipeline(render_pass);
			}
			if (splat_pipeline == VK_NULL_HANDLE)
			{
				LOGE("Failed to create splat pipeline; skipping splat draw");
				break;
			}

			// Update splat UBO
			splat_ubo.projection = camera.matrices.perspective;
			splat_ubo.view       = camera.matrices.view;
			splat_ubo.viewport   = glm::vec2(static_cast<float>(width), static_cast<float>(height));
			splat_ubo.focalX     = camera.matrices.perspective[0][0] * splat_ubo.viewport.x * 0.5f;
			splat_ubo.focalY     = camera.matrices.perspective[1][1] * splat_ubo.viewport.y * 0.5f;
			splat_uniform_buffer->convert_and_update(splat_ubo);

			// Sort splats back-to-front for correct alpha compositing.
			// Camera position = -R^T * t, extracted from the view matrix.
			{
				glm::mat3 R       = glm::mat3(camera.matrices.view);
				glm::vec3 t       = glm::vec3(camera.matrices.view[3]);
				glm::vec3 cam_pos = -glm::transpose(R) * t;

				std::sort(splat_instances_cpu.begin(), splat_instances_cpu.end(),
				          [&cam_pos](const SplatInstance &a, const SplatInstance &b) {
					          float dax = a.pos[0] - cam_pos.x, day = a.pos[1] - cam_pos.y, daz = a.pos[2] - cam_pos.z;
					          float dbx = b.pos[0] - cam_pos.x, dby = b.pos[1] - cam_pos.y, dbz = b.pos[2] - cam_pos.z;
					          return (dax * dax + day * day + daz * daz) > (dbx * dbx + dby * dby + dbz * dbz);
				          });

				auto *buf = splat_instance_buffer->map();
				memcpy(buf, splat_instances_cpu.data(), splat_instances_cpu.size() * sizeof(SplatInstance));
				splat_instance_buffer->flush();
				splat_instance_buffer->unmap();
			}

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, splat_pipeline_layout, 0, 1, &splat_descriptor_set, 0, nullptr);
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, splat_pipeline);
			vkCmdBindVertexBuffers(cmd, 0, 1, &splat_instance_buffer->get_handle(), offsets);
			vkCmdDraw(cmd, 4, splat_count, 0, 0);
			break;
		}
	}

	// Draw the framework's ImGui overlay (view-mode buttons) on top.
	draw_ui(cmd);

	vkCmdEndRenderPass(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd));

	// Submit to queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void RenderOctomap::input_event(const vkb::InputEvent &input_event)
{
	// For keyboard events, intercept WASD/numpad camera control before the
	// framework's GUI gets a chance to capture the keystroke.
	if (input_event.get_source() == vkb::EventSource::Keyboard)
	{
		const auto        &key_event = static_cast<const vkb::KeyInputEvent &>(input_event);
		const vkb::KeyCode code      = key_event.get_code();

		// WASD movement always passes through to the camera even when the GUI has focus.
		const bool is_wasd =
		    code == vkb::KeyCode::W || code == vkb::KeyCode::A ||
		    code == vkb::KeyCode::S || code == vkb::KeyCode::D;

		if (is_wasd)
		{
			// Always forward WASD to the framework camera regardless of GUI focus.
			ApiVulkanSample::input_event(input_event);
			return;
		}

		// Numpad 4/6/8/2 drive camera look (yaw/pitch) — track state here.
		const bool is_up_action = (key_event.get_action() == vkb::KeyAction::Up);
		if (code == vkb::KeyCode::KP_4)
		{
			numpad_look.left = !is_up_action;
			return;
		}
		if (code == vkb::KeyCode::KP_6)
		{
			numpad_look.right = !is_up_action;
			return;
		}
		if (code == vkb::KeyCode::KP_8)
		{
			numpad_look.up = !is_up_action;
			return;
		}
		if (code == vkb::KeyCode::KP_2)
		{
			numpad_look.down = !is_up_action;
			return;
		}
	}

	// Let the framework handle everything else, including mouse/keyboard
	// input for the ImGui overlay (via ApiVulkanSample -> get_gui().input_event()).
	ApiVulkanSample::input_event(input_event);
}

void RenderOctomap::on_view_state_changed(ViewState new_state)
{
	if (current_view_state == new_state)
	{
		return;
	}

	LOGI("View state changed to: {}", static_cast<int>(new_state));
	current_view_state = new_state;

	switch (new_state)
	{
		case ViewState::Octomap:
			// Octomap is already loaded, just need to rebuild command buffers
			LOGI("Switching to Octomap view");
			break;

		case ViewState::GLTFRegular:
			LOGI("Switching to GLTF Regular view");
			if (!gltf_scene)
			{
				load_gltf_scene("scenes/octmap_and_splats/savedMap_v1.2.0.gltf");
			}
			break;

		case ViewState::GLTFSplats:
			LOGI("Switching to Gaussian Splats view");
			if (!splat_instance_buffer)
			{
				load_gaussian_splats_scene("scenes/octmap_and_splats");
			}
			break;
	}

	// Rebuild command buffers for the new view
	rebuild_command_buffers();
}

void RenderOctomap::load_gltf_scene(const std::string &filename)
{
	LOGI("Loading GLTF scene: {}", filename);

	vkb::GLTFLoader loader(get_device());
	gltf_scene = loader.read_scene_from_file(filename);

	if (gltf_scene)
	{
		LOGI("GLTF scene loaded successfully");

		// Build a flat list of nodes/submeshes for drawing.
		gltf_nodes.clear();
		auto meshes = gltf_scene->get_components<vkb::sg::Mesh>();
		for (auto *mesh : meshes)
		{
			for (auto node : mesh->get_nodes())
			{
				for (auto *sub_mesh : mesh->get_submeshes())
				{
					gltf_nodes.push_back({node, sub_mesh});
				}
			}
		}

		create_gltf_pipeline(render_pass);
	}
	else
	{
		LOGE("Failed to load GLTF scene: {}", filename);
	}
}

void RenderOctomap::load_gaussian_splats_scene(const std::string &scene_dir)
{
	LOGI("Loading Gaussian Splats from directory: {}", scene_dir);

	splat_instances_cpu.clear();
	splat_instance_buffer.reset();

	std::string full_dir = vkb::fs::path::get(vkb::fs::path::Type::Assets) + scene_dir;

	// Collect all *_cell_*.gltf files in the directory, then sort for deterministic order
	std::vector<std::string> cell_files;
	for (auto &entry : std::filesystem::directory_iterator(full_dir))
	{
		if (!entry.is_regular_file())
			continue;
		std::string name = entry.path().filename().string();
		if (name.find("_cell_") != std::string::npos &&
		    name.ends_with(".gltf"))
		{
			cell_files.push_back(scene_dir + "/" + name);
		}
	}
	std::sort(cell_files.begin(), cell_files.end());

	if (cell_files.empty())
	{
		LOGE("No cell GLTF files found in {}", full_dir);
		return;
	}

	for (auto &f : cell_files)
	{
		LOGI("  Loading cell: {}", f);
		load_gaussian_splats_data(f);
	}

	if (splat_instances_cpu.empty())
	{
		LOGE("No splat data loaded from {}", scene_dir);
		return;
	}

	splat_count           = static_cast<uint32_t>(splat_instances_cpu.size());
	splat_instance_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                             splat_instances_cpu.size() * sizeof(SplatInstance),
	                                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                             VMA_MEMORY_USAGE_CPU_TO_GPU);
	auto *buf             = splat_instance_buffer->map();
	memcpy(buf, splat_instances_cpu.data(), splat_instances_cpu.size() * sizeof(SplatInstance));
	splat_instance_buffer->flush();
	splat_instance_buffer->unmap();
	splat_instance_buffer->set_debug_name("render_octomap splat instance buffer");

	if (!splat_uniform_buffer)
	{
		splat_uniform_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
		                                                            sizeof(splat_ubo),
		                                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		                                                            VMA_MEMORY_USAGE_CPU_TO_GPU);
		splat_uniform_buffer->set_debug_name("render_octomap splat ubo");
	}

	LOGI("Loaded {} gaussian splats from {} cells", splat_count, cell_files.size());

	create_splat_pipeline(render_pass);
}

void RenderOctomap::load_gaussian_splats_data(const std::string &filename)
{
	// Parse the splats GLTF directly to extract `KHR_gaussian_splatting` attributes.
	// The file contains a single POINTS primitive with accessor indices for POSITION/COLOR_0 and
	// extension fields ROTATION/SCALE/OPACITY.
	tinygltf::TinyGLTF gltf;
	tinygltf::Model    model;
	std::string        err;
	std::string        warn;

	std::string gltf_file = vkb::fs::path::get(vkb::fs::path::Type::Assets) + filename;
	bool        ok        = gltf.LoadASCIIFromFile(&model, &err, &warn, gltf_file.c_str());
	if (!ok || !err.empty())
	{
		LOGE("Failed to load splats gltf {}: {}", gltf_file, err);
		return;
	}
	if (!warn.empty())
	{
		LOGI("{}", warn);
	}
	if (model.meshes.empty() || model.meshes[0].primitives.empty())
	{
		LOGE("Splats gltf has no meshes/primitives: {}", filename);
		return;
	}

	const tinygltf::Primitive &prim = model.meshes[0].primitives[0];

	auto get_accessor_ptr = [&](int accessor_index, size_t &stride_bytes) -> const uint8_t * {
		if (accessor_index < 0 || accessor_index >= static_cast<int>(model.accessors.size()))
		{
			stride_bytes = 0;
			return nullptr;
		}
		const tinygltf::Accessor   &acc = model.accessors[accessor_index];
		const tinygltf::BufferView &bv  = model.bufferViews[acc.bufferView];
		const tinygltf::Buffer     &buf = model.buffers[bv.buffer];

		if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
		{
			stride_bytes = 0;
			return nullptr;
		}

		size_t comps = 1;
		switch (acc.type)
		{
			case TINYGLTF_TYPE_VEC2:
				comps = 2;
				break;
			case TINYGLTF_TYPE_VEC3:
				comps = 3;
				break;
			case TINYGLTF_TYPE_VEC4:
				comps = 4;
				break;
			case TINYGLTF_TYPE_MAT3:
				comps = 9;
				break;
			default:
				comps = 1;
				break;
		}
		size_t elem_size = comps * sizeof(float);
		stride_bytes     = (bv.byteStride > 0) ? static_cast<size_t>(bv.byteStride) : elem_size;

		return buf.data.data() + static_cast<size_t>(bv.byteOffset) + static_cast<size_t>(acc.byteOffset);
	};

	const int pos_accessor = prim.attributes.contains("POSITION") ? prim.attributes.at("POSITION") : -1;
	int       col_accessor = prim.attributes.contains("COLOR_0") ? prim.attributes.at("COLOR_0") : -1;

	// KHR_gaussian_splatting uses KHR_gaussian_splatting:SH_DEGREE_0_COEF_0 for the base color (DC component)
	if (col_accessor < 0 && prim.attributes.contains("KHR_gaussian_splatting:SH_DEGREE_0_COEF_0"))
	{
		col_accessor = prim.attributes.at("KHR_gaussian_splatting:SH_DEGREE_0_COEF_0");
	}

	if (pos_accessor < 0)
	{
		LOGE("Splats gltf missing POSITION accessor: {}", filename);
		return;
	}

	int rot_accessor     = prim.attributes.contains("KHR_gaussian_splatting:ROTATION") ? prim.attributes.at("KHR_gaussian_splatting:ROTATION") : -1;
	int scale_accessor   = prim.attributes.contains("KHR_gaussian_splatting:SCALE") ? prim.attributes.at("KHR_gaussian_splatting:SCALE") : -1;
	int opacity_accessor = prim.attributes.contains("KHR_gaussian_splatting:OPACITY") ? prim.attributes.at("KHR_gaussian_splatting:OPACITY") : -1;

	if (prim.extensions.contains(GLTF_KHR_GAUSSIAN_SPLATTING_EXTENSION))
	{
		const tinygltf::Value &ext = prim.extensions.at(GLTF_KHR_GAUSSIAN_SPLATTING_EXTENSION);
		if (ext.IsObject())
		{
			// Fallback to old draft extension property names
			if (rot_accessor < 0 && ext.Has("ROTATION"))
			{
				rot_accessor = ext.Get("ROTATION").Get<int>();
			}
			if (scale_accessor < 0 && ext.Has("SCALE"))
			{
				scale_accessor = ext.Get("SCALE").Get<int>();
			}
			if (opacity_accessor < 0 && ext.Has("OPACITY"))
			{
				opacity_accessor = ext.Get("OPACITY").Get<int>();
			}
		}
	}
	if (rot_accessor < 0 || scale_accessor < 0 || opacity_accessor < 0 || col_accessor < 0)
	{
		LOGE("Splats gltf missing required KHR_gaussian_splatting accessors (ROTATION/SCALE/OPACITY/COLOR_0): {}", filename);
		return;
	}

	const size_t count = model.accessors[pos_accessor].count;
	if (count == 0)
	{
		LOGE("Splats gltf has 0 splats: {}", filename);
		return;
	}

	size_t         pos_stride = 0, rot_stride = 0, scale_stride = 0, opa_stride = 0, col_stride = 0;
	const uint8_t *pos_ptr   = get_accessor_ptr(pos_accessor, pos_stride);
	const uint8_t *rot_ptr   = get_accessor_ptr(rot_accessor, rot_stride);
	const uint8_t *scale_ptr = get_accessor_ptr(scale_accessor, scale_stride);
	const uint8_t *opa_ptr   = get_accessor_ptr(opacity_accessor, opa_stride);
	const uint8_t *col_ptr   = get_accessor_ptr(col_accessor, col_stride);
	if (!pos_ptr || !rot_ptr || !scale_ptr || !opa_ptr || !col_ptr)
	{
		LOGE("Splats gltf accessor buffer decode failed: {}", filename);
		return;
	}

	std::vector<SplatInstance> instances;
	instances.resize(count);
	for (size_t i = 0; i < count; ++i)
	{
		const float *p = reinterpret_cast<const float *>(pos_ptr + i * pos_stride);
		const float *r = reinterpret_cast<const float *>(rot_ptr + i * rot_stride);
		const float *s = reinterpret_cast<const float *>(scale_ptr + i * scale_stride);
		const float *o = reinterpret_cast<const float *>(opa_ptr + i * opa_stride);
		const float *c = reinterpret_cast<const float *>(col_ptr + i * col_stride);

		instances[i].pos[0]   = p[0];
		instances[i].pos[1]   = p[1];
		instances[i].pos[2]   = p[2];
		instances[i].rot[0]   = r[0];
		instances[i].rot[1]   = r[1];
		instances[i].rot[2]   = r[2];
		instances[i].rot[3]   = r[3];
		instances[i].scale[0] = s[0];
		instances[i].scale[1] = s[1];
		instances[i].scale[2] = s[2];
		instances[i].opacity  = o[0];
		instances[i].color[0] = c[0];
		instances[i].color[1] = c[1];
		instances[i].color[2] = c[2];
		instances[i]._pad     = 0.0f;
	}

	// Append to the accumulated list (buffer creation is done in load_gaussian_splats_scene)
	splat_instances_cpu.insert(splat_instances_cpu.end(), instances.begin(), instances.end());
	LOGI("  Appended {} splats (total so far: {})", count, splat_instances_cpu.size());
}

std::unique_ptr<vkb::Application> create_render_octomap()
{
	return std::make_unique<RenderOctomap>();
}