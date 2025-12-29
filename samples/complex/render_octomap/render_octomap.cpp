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

#include <tiny_gltf.h>

// KHR_gaussian_splatting extension name
#define KHR_GAUSSIAN_SPLATTING_EXTENSION "KHR_gaussian_splatting"

render_octomap::render_octomap() :
    vertices(), indexCount(), pipelineCache(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE), pipeline(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE), descriptorSetLayout(VK_NULL_HANDLE), descriptorSet(VK_NULL_HANDLE), gui(nullptr), mMaxTreeDepth(16), m_zMin(), m_zMax(), lastMapBuildSize()
{
	title = "Octomap Viewer";
	map   = new octomap::OcTree(0.1f);
}
render_octomap::~render_octomap()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptorSetLayout, nullptr);
		vkDestroyPipeline(get_device().get_handle(), gltf_pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), gltf_pipeline_layout, nullptr);
		vkDestroyPipeline(get_device().get_handle(), splat_pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), splat_pipeline_layout, nullptr);
		vkDestroyDescriptorPool(get_device().get_handle(), splat_descriptor_pool, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), splat_descriptor_set_layout, nullptr);
		vkDestroyPipelineCache(get_device().get_handle(), pipelineCache, nullptr);
		vkDestroyDescriptorPool(get_device().get_handle(), descriptorPool, nullptr);
		delete gui;
		gui = nullptr;
	}
	delete map;
	map = nullptr;
}
void render_octomap::BuildCubes()
{
	const octomap::OcTree *tree = map;
	if (tree->size() == 0)
	{
		return;
	}
	float nextBuildSize = static_cast<float>(lastMapBuildSize) + (static_cast<float>(lastMapBuildSize) * 0.05f);
	if (static_cast<float>(tree->size()) < nextBuildSize)
	{
		return;
	}

	double minX, minY, minZ, maxX, maxY, maxZ;
	tree->getMetricMin(minX, minY, minZ);
	tree->getMetricMax(maxX, maxY, maxZ);

	// set min/max Z for color height map
	m_zMin = static_cast<float>(minZ);
	m_zMax = static_cast<float>(maxZ);

	// this is to get just grey; doing full color heightmap for now.
	// h = std::min(std::max((h-m_zMin)/ (m_zMax - m_zMin), 0.0f), 1.0f) * 0.4f + 0.3f; // h \in [0.3, 0.7]

	instances.clear();
	for (auto it = tree->begin_tree(mMaxTreeDepth), end = tree->end_tree(); it != end; ++it)
	{
		if (it.isLeaf() && tree->isNodeOccupied(*it))
		{
			glm::vec3 coords = {it.getCoordinate().x(), it.getCoordinate().y(), it.getCoordinate().z()};
			coords.y *= -1;
			InstanceData instance;
			instance.pos[0] = coords[0];
			instance.pos[1] = coords[1];
			instance.pos[2] = coords[2];
			float h         = coords[2];
			if (m_zMin >= m_zMax)
			{
				h = 0.5f;
			}
			else
			{
				h = (1.0f - std::min(std::max((h - m_zMin) / (m_zMax - m_zMin), 0.0f), 1.0f)) * 0.8f;
			}

			// blend over HSV-values (more colors)
			float r, g, b;
			float s = 1.0f;
			float v = 1.0f;

			h -= floor(h);
			h *= 6;
			int   i;
			float m, n, f;

			i = floor(h);
			f = h - static_cast<float>(i);
			if (!(i & 1))
			{
				f = 1 - f;        // if "i" is even
			}
			m = v * (1 - s);
			n = v * (1 - s * f);

			switch (i)
			{
				case 6:
				case 0:
					r = v;
					g = n;
					b = m;
					break;
				case 1:
					r = n;
					g = v;
					b = m;
					break;
				case 2:
					r = m;
					g = v;
					b = n;
					break;
				case 3:
					r = m;
					g = n;
					b = v;
					break;
				case 4:
					r = n;
					g = m;
					b = v;
					break;
				case 5:
					r = v;
					g = m;
					b = n;
					break;
				default:
					r = 1;
					g = 0.5f;
					b = 0.5f;
					break;
			}

			instance.col[0] = r;
			instance.col[1] = g;
			instance.col[2] = b;
			instance.col[3] = 1.0f;
			instance.scale  = static_cast<float>(it.getSize());
			instances.push_back(instance);
		}
	}        // end for all voxels
	// Create buffers
	if (!instances.empty())
	{
		instanceBuffer = std::make_unique<vkb::core::BufferC>(get_device(),
		                                                      instances.size() * sizeof(InstanceData),
		                                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		                                                      VMA_MEMORY_USAGE_CPU_TO_GPU);

		auto buf = instanceBuffer->map();
		memcpy(buf, instances.data(), instances.size() * sizeof(InstanceData));
		instanceBuffer->flush();
		instanceBuffer->unmap();
	}
	// instance buffer

	lastBuildTime    = std::chrono::system_clock::now();
	lastMapBuildSize = tree->size();
}

void render_octomap::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.033f, 0.0f}};
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
		// Render ImGui first (sidebar + map panel background), then draw the 3D map into the map viewport.
		// This ensures the opaque `mapDisplay` background doesn't overdraw the 3D content.
		gui->drawFrame(draw_cmd_buffers[i]);

		VkViewport viewport = vkb::initializers::viewport(gui->MapsView.mapSize.x, gui->MapsView.mapSize.y, 0.0f, 1.0f);
		viewport.x          = gui->MapsView.mapPos.x;
		viewport.y          = gui->MapsView.mapPos.y;
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);
		VkRect2D scissorRect;
		scissorRect.offset.x      = static_cast<int32_t>(gui->MapsView.mapPos.x);
		scissorRect.offset.y      = static_cast<int32_t>(gui->MapsView.mapPos.y);
		scissorRect.extent.width  = static_cast<uint32_t>(gui->MapsView.mapSize.x);
		scissorRect.extent.height = static_cast<uint32_t>(gui->MapsView.mapSize.y);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissorRect);

		VkDeviceSize offsets[1] = {0};
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
		                        &descriptorSet, 0, nullptr);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, &vertexBuffer->get_handle(), offsets);
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 1, 1, &instanceBuffer->get_handle(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], indexBuffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(draw_cmd_buffers[i], indexCount, instances.size(), 0, 0, 0);
		// draw_ui(draw_cmd_buffers[i]);
		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}
bool render_octomap::prepare(const vkb::ApplicationOptions &options)
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
	std::string octomapPath = vkb::fs::path::get(vkb::fs::path::Type::Assets, "scenes/octmap_and_splats/octMap.bin");
	map->readBinary(octomapPath);
	BuildCubes();
	gui = new ImGUIUtil(this);
	gui->init(static_cast<float>(width), static_cast<float>(height));
	gui->initResources(render_pass, queue);
	createPipelines(render_pass);
	// Initialize ImGui frame state before first command buffer build
	gui->newFrame(true);
	gui->updateBuffers();
	build_command_buffers();
	prepared = true;
	return true;
}

void render_octomap::createPipelines(VkRenderPass renderPass)
{
	SetupVertexDescriptions();
	prepareUBO();
	auto inputAssemblyState   = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	auto raster_State         = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	auto blendAttachmentState = vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
	auto colorBlendState      = vkb::initializers::pipeline_color_blend_state_create_info(1, &blendAttachmentState);
	auto depthStencilState    = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	auto viewportState        = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);
	auto multisampleState     = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector dynamicStateEnables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_LINE_WIDTH};

	auto dynamicState = vkb::initializers::pipeline_dynamic_state_create_info(dynamicStateEnables.data(), dynamicStateEnables.size(), 0);

	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VK_CHECK(vkCreatePipelineCache(get_device().get_handle(), &pipelineCacheCreateInfo, nullptr, &pipelineCache));

	// Rendering pipeline
	std::vector poolSizes = {
	    // Graphics pipelines uniform buffers
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2)};
	VkDescriptorPoolCreateInfo descriptorPoolInfo = vkb::initializers::descriptor_pool_create_info(poolSizes, 3);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptorPoolInfo, nullptr, &descriptorPool));

	std::vector setLayoutBindings = {
	    // Binding 0: Vertex shader uniform buffer
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)};

	auto descriptorLayout = vkb::initializers::descriptor_set_layout_create_info(setLayoutBindings);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptorLayout, nullptr, &descriptorSetLayout));

	auto pPipelineLayoutCreateInfo = vkb::initializers::pipeline_layout_create_info(&descriptorSetLayout, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));

	// Load shaders
	std::vector shaderStages = {
	    load_shader("render_octomap", "render.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
	    load_shader("render_octomap", "render.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

	auto pipelineCreateInfo = vkb::initializers::pipeline_create_info(pipelineLayout, renderPass, 0);

	pipelineCreateInfo.pVertexInputState   = &vertices.inputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &raster_State;
	pipelineCreateInfo.pColorBlendState    = &colorBlendState;
	pipelineCreateInfo.pMultisampleState   = &multisampleState;
	pipelineCreateInfo.pViewportState      = &viewportState;
	pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
	pipelineCreateInfo.pDynamicState       = &dynamicState;
	pipelineCreateInfo.stageCount          = shaderStages.size();
	pipelineCreateInfo.pStages             = shaderStages.data();
	pipelineCreateInfo.renderPass          = renderPass;

	VK_CHECK(
	    vkCreateGraphicsPipelines(get_device().get_handle(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));

	auto allocInfo = vkb::initializers::descriptor_set_allocate_info(descriptorPool, &descriptorSetLayout, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &allocInfo, &descriptorSet));
	VkDescriptorBufferInfo            buffer_descriptor            = create_descriptor(*uniformBufferVS);
	std::vector<VkWriteDescriptorSet> baseImageWriteDescriptorSets = {
	    vkb::initializers::write_descriptor_set(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
	                                            &buffer_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), baseImageWriteDescriptorSets.size(), baseImageWriteDescriptorSets.data(), 0, nullptr);
}

void render_octomap::create_gltf_pipeline(VkRenderPass renderPass)
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

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptorSetLayout, 1);
	pipeline_layout_create_info.pushConstantRangeCount     = 1;
	pipeline_layout_create_info.pPushConstantRanges        = &push_constant_range;
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &gltf_pipeline_layout));

	// Shaders
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
	    load_shader("render_octomap", "gltf.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
	    load_shader("render_octomap", "gltf.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

	// Vertex input: single interleaved binding with POSITION (VEC3, 12 bytes) + COLOR_0 (VEC4, 16 bytes) = stride 28
	std::vector<VkVertexInputBindingDescription> bindings = {
	    vkb::initializers::vertex_input_binding_description(0, 28, VK_VERTEX_INPUT_RATE_VERTEX)};
	std::vector<VkVertexInputAttributeDescription> attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),             // POSITION at offset 0
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 12)};        // COLOR_0 at offset 12

	VkPipelineVertexInputStateCreateInfo vertexInputState = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertexInputState.vertexBindingDescriptionCount        = static_cast<uint32_t>(bindings.size());
	vertexInputState.pVertexBindingDescriptions           = bindings.data();
	vertexInputState.vertexAttributeDescriptionCount      = static_cast<uint32_t>(attributes.size());
	vertexInputState.pVertexAttributeDescriptions         = attributes.data();

	auto inputAssemblyState = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	auto raster_state       = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
	                                                                                      VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	auto blendAttachment    = vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
	auto colorBlendState    = vkb::initializers::pipeline_color_blend_state_create_info(1, &blendAttachment);
	auto depthStencilState  = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	auto viewportState      = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);
	auto multisampleState   = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	auto                        dynamicState        = vkb::initializers::pipeline_dynamic_state_create_info(dynamicStateEnables.data(), dynamicStateEnables.size(), 0);

	auto pipelineCreateInfo                = vkb::initializers::pipeline_create_info(gltf_pipeline_layout, renderPass, 0);
	pipelineCreateInfo.pVertexInputState   = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &raster_state;
	pipelineCreateInfo.pColorBlendState    = &colorBlendState;
	pipelineCreateInfo.pMultisampleState   = &multisampleState;
	pipelineCreateInfo.pViewportState      = &viewportState;
	pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
	pipelineCreateInfo.pDynamicState       = &dynamicState;
	pipelineCreateInfo.stageCount          = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages             = shaderStages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &gltf_pipeline));
}

void render_octomap::create_splat_pipeline(VkRenderPass renderPass)
{
	if (splat_pipeline != VK_NULL_HANDLE)
	{
		return;
	}

	// Descriptor set for splat UBO
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
		    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)};
		VkDescriptorPoolCreateInfo poolInfo = vkb::initializers::descriptor_pool_create_info(poolSizes, 1);
		VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &poolInfo, nullptr, &splat_descriptor_pool));

		std::vector<VkDescriptorSetLayoutBinding> bindings = {
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)};
		VkDescriptorSetLayoutCreateInfo layoutInfo = vkb::initializers::descriptor_set_layout_create_info(bindings);
		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &layoutInfo, nullptr, &splat_descriptor_set_layout));

		VkDescriptorSetAllocateInfo allocInfo = vkb::initializers::descriptor_set_allocate_info(splat_descriptor_pool, &splat_descriptor_set_layout, 1);
		VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &allocInfo, &splat_descriptor_set));

		VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*splat_uniform_buffer);
		VkWriteDescriptorSet   write             = vkb::initializers::write_descriptor_set(splat_descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &buffer_descriptor);
		vkUpdateDescriptorSets(get_device().get_handle(), 1, &write, 0, nullptr);
	}

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&splat_descriptor_set_layout, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &splat_pipeline_layout));

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
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

	VkPipelineVertexInputStateCreateInfo vertexInputState = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertexInputState.vertexBindingDescriptionCount        = static_cast<uint32_t>(bindings.size());
	vertexInputState.pVertexBindingDescriptions           = bindings.data();
	vertexInputState.vertexAttributeDescriptionCount      = static_cast<uint32_t>(attributes.size());
	vertexInputState.pVertexAttributeDescriptions         = attributes.data();

	auto inputAssemblyState = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0, VK_FALSE);
	auto raster_state       = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
	                                                                                      VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

	// Premultiplied alpha blending
	VkPipelineColorBlendAttachmentState blendAttachment{};
	blendAttachment.colorWriteMask      = 0xf;
	blendAttachment.blendEnable         = VK_TRUE;
	blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

	auto colorBlendState   = vkb::initializers::pipeline_color_blend_state_create_info(1, &blendAttachment);
	auto depthStencilState = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
	auto viewportState     = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);
	auto multisampleState  = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	auto                        dynamicState        = vkb::initializers::pipeline_dynamic_state_create_info(dynamicStateEnables.data(), dynamicStateEnables.size(), 0);

	auto pipelineCreateInfo                = vkb::initializers::pipeline_create_info(splat_pipeline_layout, renderPass, 0);
	pipelineCreateInfo.pVertexInputState   = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &raster_state;
	pipelineCreateInfo.pColorBlendState    = &colorBlendState;
	pipelineCreateInfo.pMultisampleState   = &multisampleState;
	pipelineCreateInfo.pViewportState      = &viewportState;
	pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
	pipelineCreateInfo.pDynamicState       = &dynamicState;
	pipelineCreateInfo.stageCount          = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages             = shaderStages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &splat_pipeline));
}

void render_octomap::prepareUBO()
{
	// Vertex shader uniform buffer block
	uniformBufferVS = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                       sizeof(uboVS),
	                                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                       VMA_MEMORY_USAGE_CPU_TO_GPU);
	updateUBO();
}

void render_octomap::updateUBO()
{
	uboVS.projection = camera.matrices.perspective;
	uboVS.camera     = camera.matrices.view;

	uniformBufferVS->convert_and_update(uboVS);
}

// This just gives the first vertexBuffer
void render_octomap::generateMasterCube()
{
	// Setup vertices for a single quad made from two triangles
	std::vector<Vertex> verticesLoc =
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
	indexCount = static_cast<uint32_t>(indices.size());

	// Create buffers
	// Vertex buffer
	vertexBuffer = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                    verticesLoc.size() * sizeof(Vertex),
	                                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);

	auto buf = vertexBuffer->map();
	memcpy(buf, verticesLoc.data(), verticesLoc.size() * sizeof(Vertex));
	vertexBuffer->flush();
	vertexBuffer->unmap();
	// Index buffer
	indexBuffer = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                   indices.size() * sizeof(uint32_t),
	                                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                                   VMA_MEMORY_USAGE_CPU_TO_GPU);
	buf         = indexBuffer->map();
	memcpy(buf, indices.data(), indices.size() * sizeof(uint32_t));
	indexBuffer->flush();
	indexBuffer->unmap();
}

void render_octomap::SetupVertexDescriptions()
{
	generateMasterCube();
	// Binding description
	vertices.bindingDescriptions = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)};

	// Attribute descriptions
	// Describes memory layout and shader positions
	vertices.attributeDescriptions = {
	    // Location 0: Position
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),

	    // Per-Instance attributes
	    vkb::initializers::vertex_input_attribute_description(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),                           // Location 1: Position
	    vkb::initializers::vertex_input_attribute_description(1, 2, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 3),        // Location 2: Color
	    vkb::initializers::vertex_input_attribute_description(1, 3, VK_FORMAT_R32_SFLOAT, sizeof(float) * 7),                 // Location 3: Scale
	};

	// Assign to vertex buffer
	vertices.inputState                                 = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertices.inputState.vertexBindingDescriptionCount   = vertices.bindingDescriptions.size();
	vertices.inputState.pVertexBindingDescriptions      = vertices.bindingDescriptions.data();
	vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
	vertices.inputState.pVertexAttributeDescriptions    = vertices.attributeDescriptions.data();
}
bool render_octomap::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	rebuild_command_buffers();
	return true;
}

void render_octomap::update_overlay(float delta_time, const std::function<void()> &additional_ui)
{
	// This sample uses a custom ImGUI implementation (ImGUIUtil)
	// The GUI updates are handled in the render() function
	// Call the additional_ui callback if provided
	if (additional_ui)
	{
		additional_ui();
	}
}

void render_octomap::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	ApiVulkanSample::prepare_frame();

	// Update camera movement based on keyboard input (WASD)
	camera.update(delta_time);

	// Update imGui every frame to process input
	ImGuiIO &io = ImGui::GetIO();

	io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
	io.DeltaTime   = delta_time;

	// Process ImGui frame to handle button clicks and other input
	gui->newFrame(frame_count == 0);
	bool ui_buffers_recreated = gui->updateBuffers();

	const bool view_state_changed = gui->MapsView.stateChanged;
	if (view_state_changed)
	{
		gui->MapsView.stateChanged = false;
		onViewStateChanged(gui->MapsView.currentState);
	}

	// (Re)build 3D instance data and UBOs before recording.
	if (!paused || camera.updated)
	{
		updateUBO();
	}
	BuildCubes();

	// Record only the current command buffer (safe per-frame path).
	recreate_current_command_buffer();
	auto cmd         = draw_cmd_buffers[current_buffer];
	auto begin_info  = vkb::initializers::command_buffer_begin_info();
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.033f, 0.0f}};
	clear_values[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;
	render_pass_begin_info.framebuffer              = framebuffers[current_buffer];

	vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	// Draw the 3D map into the viewport.
	VkViewport viewport = vkb::initializers::viewport(gui->MapsView.mapSize.x, gui->MapsView.mapSize.y, 0.0f, 1.0f);
	viewport.x          = gui->MapsView.mapPos.x;
	viewport.y          = gui->MapsView.mapPos.y;
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissorRect;
	scissorRect.offset.x      = static_cast<int32_t>(gui->MapsView.mapPos.x);
	scissorRect.offset.y      = static_cast<int32_t>(gui->MapsView.mapPos.y);
	scissorRect.extent.width  = static_cast<uint32_t>(gui->MapsView.mapSize.x);
	scissorRect.extent.height = static_cast<uint32_t>(gui->MapsView.mapSize.y);
	vkCmdSetScissor(cmd, 0, 1, &scissorRect);

	VkDeviceSize offsets[2] = {0, 0};

	switch (currentViewState)
	{
		case MapView::ViewState::Octomap:
		{
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer->get_handle(), offsets);
			if (instanceBuffer)
			{
				vkCmdBindVertexBuffers(cmd, 1, 1, &instanceBuffer->get_handle(), offsets);
				vkCmdBindIndexBuffer(cmd, indexBuffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(cmd, indexCount, static_cast<uint32_t>(instances.size()), 0, 0, 0);
			}
			break;
		}

		case MapView::ViewState::GLTFRegular:
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

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gltf_pipeline_layout, 0, 1, &descriptorSet, 0, nullptr);
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

				// If material color is default/white and we have vertex colors, the shader will use vertex colors
				pc.model = d.node->get_transform().get_world_matrix();
				pc.color = col;
				vkCmdPushConstants(cmd, gltf_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GltfPushConstants), &pc);

				if (d.sub_mesh->index_buffer)
				{
					vkCmdBindIndexBuffer(cmd, d.sub_mesh->index_buffer->get_handle(), 0, d.sub_mesh->index_type);
					vkCmdDrawIndexed(cmd, d.sub_mesh->vertex_indices, 1, 0, 0, 0);
				}
			}
			break;
		}

		case MapView::ViewState::GLTFSplats:
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
				break;
			}

			// Update splat UBO
			splat_ubo.projection = camera.matrices.perspective;
			splat_ubo.view       = camera.matrices.view;
			splat_ubo.viewport   = glm::vec2(gui->MapsView.mapSize.x, gui->MapsView.mapSize.y);
			splat_ubo.focalX     = camera.matrices.perspective[0][0] * splat_ubo.viewport.x * 0.5f;
			splat_ubo.focalY     = camera.matrices.perspective[1][1] * splat_ubo.viewport.y * 0.5f;
			splat_uniform_buffer->convert_and_update(splat_ubo);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, splat_pipeline_layout, 0, 1, &splat_descriptor_set, 0, nullptr);
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, splat_pipeline);
			vkCmdBindVertexBuffers(cmd, 0, 1, &splat_instance_buffer->get_handle(), offsets);
			vkCmdDraw(cmd, 4, splat_count, 0, 0);
			break;
		}
	}

	// Draw ImGui last so sidebar/buttons are on top. `mapDisplay` is transparent.
	gui->drawFrame(cmd);

	vkCmdEndRenderPass(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd));

	// Submit to queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void render_octomap::input_event(const vkb::InputEvent &input_event)
{
	ImGuiIO &io = ImGui::GetIO();

	if (input_event.get_source() == vkb::EventSource::Mouse)
	{
		const auto &mouse_button   = static_cast<const vkb::MouseButtonInputEvent &>(input_event);
		const float content_scale  = window ? window->get_content_scale_factor() : 1.0f;
		const float mouse_x_scaled = mouse_button.get_pos_x() * content_scale;
		const float mouse_y_scaled = mouse_button.get_pos_y() * content_scale;

		// Use the classic ImGui IO feeding approach for reliable hover/click detection.
		io.MousePos          = ImVec2(mouse_x_scaled, mouse_y_scaled);
		const int  button_id = static_cast<int>(mouse_button.get_button());
		const bool down      = (mouse_button.get_action() == vkb::MouseAction::Down);
		const bool up        = (mouse_button.get_action() == vkb::MouseAction::Up);
		if ((down || up) && button_id >= 0 && button_id < 5)
		{
			io.MouseDown[button_id] = down;
		}

		// Sidebar bounds must match `ImGUIUtil::newFrame()`.
		const float sidebar_width = 240.0f + 20.0f * 2.0f;
		const bool  over_sidebar  = mouse_x_scaled < sidebar_width;
		if (!over_sidebar)
		{
			ApiVulkanSample::input_event(input_event);
		}
		return;
	}

	// For keyboard and other events, use the framework input pipeline.
	ApiVulkanSample::input_event(input_event);
}

void render_octomap::onViewStateChanged(MapView::ViewState newState)
{
	if (currentViewState == newState)
	{
		return;
	}

	LOGI("View state changed to: {}", static_cast<int>(newState));
	currentViewState = newState;

	switch (newState)
	{
		case MapView::ViewState::Octomap:
			// Octomap is already loaded, just need to rebuild command buffers
			LOGI("Switching to Octomap view");
			break;

		case MapView::ViewState::GLTFRegular:
			LOGI("Switching to GLTF Regular view");
			if (!gltfScene)
			{
				loadGLTFScene("scenes/octmap_and_splats/savedMap_v1.1.0.gltf");
			}
			break;

		case MapView::ViewState::GLTFSplats:
			LOGI("Switching to Gaussian Splats view");
			if (!splatsScene)
			{
				loadGaussianSplatsScene("scenes/octmap_and_splats/savedMap_v1.1.0_splats_c0_-1_-1.gltf");
			}
			break;
	}

	// Rebuild command buffers for the new view
	rebuild_command_buffers();
}

void render_octomap::loadGLTFScene(const std::string &filename)
{
	LOGI("Loading GLTF scene: {}", filename);

	vkb::GLTFLoader loader(get_device());
	gltfScene = loader.read_scene_from_file(filename);

	if (gltfScene)
	{
		LOGI("GLTF scene loaded successfully");

		// Build a flat list of nodes/submeshes for drawing.
		gltf_nodes.clear();
		auto meshes = gltfScene->get_components<vkb::sg::Mesh>();
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

void render_octomap::loadGaussianSplatsScene(const std::string &filename)
{
	LOGI("Loading Gaussian Splats scene: {}", filename);
	loadGaussianSplatsData(filename);
	create_splat_pipeline(render_pass);
}

void render_octomap::loadGaussianSplatsData(const std::string &filename)
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
	const int col_accessor = prim.attributes.contains("COLOR_0") ? prim.attributes.at("COLOR_0") : -1;
	if (pos_accessor < 0)
	{
		LOGE("Splats gltf missing POSITION accessor: {}", filename);
		return;
	}

	int rot_accessor     = -1;
	int scale_accessor   = -1;
	int opacity_accessor = -1;
	if (prim.extensions.contains(KHR_GAUSSIAN_SPLATTING_EXTENSION))
	{
		const tinygltf::Value &ext = prim.extensions.at(KHR_GAUSSIAN_SPLATTING_EXTENSION);
		if (ext.IsObject())
		{
			if (ext.Has("ROTATION"))
			{
				rot_accessor = ext.Get("ROTATION").Get<int>();
			}
			if (ext.Has("SCALE"))
			{
				scale_accessor = ext.Get("SCALE").Get<int>();
			}
			if (ext.Has("OPACITY"))
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

	splat_count           = static_cast<uint32_t>(count);
	splat_instance_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                             instances.size() * sizeof(SplatInstance),
	                                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                             VMA_MEMORY_USAGE_CPU_TO_GPU);
	auto buf              = splat_instance_buffer->map();
	memcpy(buf, instances.data(), instances.size() * sizeof(SplatInstance));
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

	LOGI("Loaded {} gaussian splats", splat_count);
}

std::unique_ptr<vkb::Application> create_render_octomap()
{
	return std::make_unique<render_octomap>();
}