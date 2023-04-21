/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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
 * Dynamic terrain tessellation, using vulkan.hpp
 */

#include "hpp_terrain_tessellation.h"

#include <heightmap.h>

HPPTerrainTessellation::HPPTerrainTessellation()
{
	title = "HPP Dynamic terrain tessellation";
}

HPPTerrainTessellation::~HPPTerrainTessellation()
{
	if (get_device() && get_device()->get_handle())
	{
		vk::Device device = get_device()->get_handle();

		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class
		device.destroyPipeline(pipelines.skysphere);
		device.destroyPipeline(pipelines.terrain);
		if (pipelines.wireframe)
		{
			device.destroyPipeline(pipelines.wireframe);
		}

		device.destroyPipelineLayout(pipeline_layouts.skysphere);
		device.destroyPipelineLayout(pipeline_layouts.terrain);

		device.destroyDescriptorSetLayout(descriptor_set_layouts.skysphere);
		device.destroyDescriptorSetLayout(descriptor_set_layouts.terrain);

		device.destroySampler(textures.heightmap.sampler);
		device.destroySampler(textures.skysphere.sampler);
		device.destroySampler(textures.terrain_array.sampler);

		if (query_pool)
		{
			device.destroyQueryPool(query_pool);
			device.destroyBuffer(query_result.buffer);
			device.freeMemory(query_result.memory);
		}
	}
}

bool HPPTerrainTessellation::prepare(vkb::platform::HPPPlatform &platform)
{
	if (!HPPApiVulkanSample::prepare(platform))
	{
		return false;
	}

	// Note: Using Reserved depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.type = vkb::CameraType::FirstPerson;
	camera.set_perspective(60.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 512.0f, 0.1f);
	camera.set_rotation(glm::vec3(-12.0f, 159.0f, 0.0f));
	camera.set_translation(glm::vec3(18.0f, 22.5f, 57.5f));
	camera.translation_speed = 7.5f;

	load_assets();
	generate_terrain();
	if (get_device()->get_gpu().get_features().pipelineStatisticsQuery)
	{
		setup_query_result_buffer();
	}
	prepare_uniform_buffers();
	setup_descriptor_set_layouts();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_sets();
	build_command_buffers();
	prepared = true;
	return true;
}

void HPPTerrainTessellation::request_gpu_features(vkb::core::HPPPhysicalDevice &gpu)
{
	// Tessellation shader support is required for this example
	if (!gpu.get_features().tessellationShader)
	{
		throw vkb::VulkanException(VK_ERROR_FEATURE_NOT_PRESENT, "Selected GPU does not support tessellation shaders!");
	}

	auto &requested_features = gpu.get_mutable_requested_features();

	requested_features.tessellationShader = true;

	// Fill mode non solid is required for wireframe display
	requested_features.fillModeNonSolid = gpu.get_features().fillModeNonSolid;

	// Pipeline statistics
	requested_features.pipelineStatisticsQuery = gpu.get_features().pipelineStatisticsQuery;

	// Enable anisotropic filtering if supported
	requested_features.samplerAnisotropy = gpu.get_features().samplerAnisotropy;
}

void HPPTerrainTessellation::build_command_buffers()
{
	vk::CommandBufferBeginInfo    command_buffer_begin_info;
	std::array<vk::ClearValue, 2> clear_values = {{default_clear_color, vk::ClearDepthStencilValue(0.0f, 0)}};

	vk::RenderPassBeginInfo render_pass_begin_info(render_pass, {}, {{0, 0}, extent}, clear_values);

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		draw_cmd_buffers[i].begin(command_buffer_begin_info);

		if (get_device()->get_gpu().get_features().pipelineStatisticsQuery)
		{
			draw_cmd_buffers[i].resetQueryPool(query_pool, 0, 2);
		}

		render_pass_begin_info.framebuffer = framebuffers[i];
		draw_cmd_buffers[i].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

		vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
		draw_cmd_buffers[i].setViewport(0, viewport);

		vk::Rect2D scissor({0, 0}, extent);
		draw_cmd_buffers[i].setScissor(0, scissor);

		draw_cmd_buffers[i].setLineWidth(1.0f);

		vk::DeviceSize offset = 0;

		// Skysphere
		draw_cmd_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.skysphere);
		draw_cmd_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layouts.skysphere, 0, descriptor_sets.skysphere, {});
		draw_model(skysphere, draw_cmd_buffers[i]);

		// Terrain
		if (get_device()->get_gpu().get_features().pipelineStatisticsQuery)
		{
			// Begin pipeline statistics query
			draw_cmd_buffers[i].beginQuery(query_pool, 0, {});
		}

		// Render
		draw_cmd_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, wireframe ? pipelines.wireframe : pipelines.terrain);
		draw_cmd_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layouts.terrain, 0, descriptor_sets.terrain, {});
		draw_cmd_buffers[i].bindVertexBuffers(0, terrain.vertices->get_handle(), offset);
		draw_cmd_buffers[i].bindIndexBuffer(terrain.indices->get_handle(), 0, vk::IndexType::eUint32);
		draw_cmd_buffers[i].drawIndexed(terrain.index_count, 1, 0, 0, 0);
		if (get_device()->get_gpu().get_features().pipelineStatisticsQuery)
		{
			// End pipeline statistics query
			draw_cmd_buffers[i].endQuery(query_pool, 0);
		}

		draw_ui(draw_cmd_buffers[i]);

		draw_cmd_buffers[i].endRenderPass();

		draw_cmd_buffers[i].end();
	}
}

void HPPTerrainTessellation::on_update_ui_overlay(vkb::HPPDrawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.checkbox("Tessellation", &tessellation))
		{
			update_uniform_buffers();
		}
		if (drawer.input_float("Factor", &ubo_tess.tessellation_factor, 0.05f, 2))
		{
			update_uniform_buffers();
		}
		if (get_device()->get_gpu().get_features().fillModeNonSolid)
		{
			if (drawer.checkbox("Wireframe", &wireframe))
			{
				build_command_buffers();
			}
		}
	}
	if (get_device()->get_gpu().get_features().pipelineStatisticsQuery)
	{
		if (drawer.header("Pipeline statistics"))
		{
			drawer.text("VS invocations: %d", pipeline_stats[0]);
			drawer.text("TE invocations: %d", pipeline_stats[1]);
		}
	}
}

void HPPTerrainTessellation::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
}

void HPPTerrainTessellation::view_changed()
{
	update_uniform_buffers();
}

void HPPTerrainTessellation::draw()
{
	HPPApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);

	// Submit to queue
	queue.submit(submit_info);

	if (get_device()->get_gpu().get_features().pipelineStatisticsQuery)
	{
		// Read query results for displaying in next frame
		pipeline_stats =
		    get_device()->get_handle().getQueryPoolResult<std::array<uint64_t, 2>>(query_pool, 0, 1, sizeof(pipeline_stats), vk::QueryResultFlagBits::e64).value;
	}

	HPPApiVulkanSample::submit_frame();
}

// Generate a terrain quad patch for feeding to the tessellation control shader
void HPPTerrainTessellation::generate_terrain()
{
	const uint32_t      patch_size   = 64;
	const float         uv_scale     = 1.0f;
	const uint32_t      vertex_count = patch_size * patch_size;
	std::vector<Vertex> vertices;
	vertices.resize(vertex_count);

	const float wx = 2.0f;
	const float wy = 2.0f;

	for (auto x = 0; x < patch_size; x++)
	{
		for (auto y = 0; y < patch_size; y++)
		{
			uint32_t index         = (x + y * patch_size);
			vertices[index].pos[0] = x * wx + wx / 2.0f - static_cast<float>(patch_size) * wx / 2.0f;
			vertices[index].pos[1] = 0.0f;
			vertices[index].pos[2] = y * wy + wy / 2.0f - static_cast<float>(patch_size) * wy / 2.0f;
			vertices[index].uv     = glm::vec2(static_cast<float>(x) / patch_size, static_cast<float>(y) / patch_size) * uv_scale;
		}
	}

	// Calculate normals from height map using a sobel filter
	vkb::HeightMap heightmap("textures/terrain_heightmap_r16.ktx", patch_size);
	for (auto x = 0; x < patch_size; x++)
	{
		for (auto y = 0; y < patch_size; y++)
		{
			// Get height samples centered around current position
			float heights[3][3];
			for (auto hx = -1; hx <= 1; hx++)
			{
				for (auto hy = -1; hy <= 1; hy++)
				{
					heights[hx + 1][hy + 1] = heightmap.get_height(x + hx, y + hy);
				}
			}

			// Calculate the normal
			glm::vec3 normal;
			// Gx sobel filter
			normal.x = heights[0][0] - heights[2][0] + 2.0f * heights[0][1] - 2.0f * heights[2][1] + heights[0][2] - heights[2][2];
			// Gy sobel filter
			normal.z = heights[0][0] + 2.0f * heights[1][0] + heights[2][0] - heights[0][2] - 2.0f * heights[1][2] - heights[2][2];
			// Calculate missing up component of the normal using the filtered x and y axis
			// The first value controls the bump strength
			normal.y = 0.25f * sqrt(1.0f - normal.x * normal.x - normal.z * normal.z);

			vertices[x + y * patch_size].normal = glm::normalize(normal * glm::vec3(2.0f, 1.0f, 2.0f));
		}
	}

	// Indices
	const uint32_t        w           = (patch_size - 1);
	const uint32_t        index_count = w * w * 4;
	std::vector<uint32_t> indices;
	indices.resize(index_count);
	for (auto x = 0; x < w; x++)
	{
		for (auto y = 0; y < w; y++)
		{
			uint32_t index     = (x + y * w) * 4;
			indices[index]     = (x + y * patch_size);
			indices[index + 1] = indices[index] + patch_size;
			indices[index + 2] = indices[index + 1] + 1;
			indices[index + 3] = indices[index] + 1;
		}
	}
	terrain.index_count = index_count;

	uint32_t vertex_buffer_size = vertex_count * sizeof(Vertex);
	uint32_t index_buffer_size  = index_count * sizeof(uint32_t);

	struct
	{
		vk::Buffer       buffer;
		vk::DeviceMemory memory;
	} vertex_staging, index_staging;

	// Create staging buffers

	std::tie(vertex_staging.buffer, vertex_staging.memory) = get_device()->create_buffer(
	    vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertex_buffer_size, vertices.data());

	std::tie(index_staging.buffer, index_staging.memory) = get_device()->create_buffer(
	    vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, index_buffer_size, indices.data());

	terrain.vertices = std::make_unique<vkb::core::HPPBuffer>(
	    *get_device(), vertex_buffer_size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY);

	terrain.indices = std::make_unique<vkb::core::HPPBuffer>(
	    *get_device(), index_buffer_size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY);

	// Copy from staging buffers
	vk::CommandBuffer copy_command = device->create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

	copy_command.copyBuffer(vertex_staging.buffer, terrain.vertices->get_handle(), {{0, 0, vertex_buffer_size}});

	copy_command.copyBuffer(index_staging.buffer, terrain.indices->get_handle(), {{0, 0, index_buffer_size}});

	device->flush_command_buffer(copy_command, queue, true);

	get_device()->get_handle().destroyBuffer(vertex_staging.buffer);
	get_device()->get_handle().freeMemory(vertex_staging.memory);
	get_device()->get_handle().destroyBuffer(index_staging.buffer);
	get_device()->get_handle().freeMemory(index_staging.memory);
}

void HPPTerrainTessellation::load_assets()
{
	skysphere = load_model("scenes/geosphere.gltf");

	textures.skysphere = load_texture("textures/skysphere_rgba.ktx", vkb::sg::Image::Color);
	// Terrain textures are stored in a texture array with layers corresponding to terrain height
	textures.terrain_array = load_texture_array("textures/terrain_texturearray_rgba.ktx", vkb::sg::Image::Color);

	// Height data is stored in a one-channel texture
	textures.heightmap = load_texture("textures/terrain_heightmap_r16.ktx", vkb::sg::Image::Other);

	// Setup a mirroring sampler for the height map
	vk::SamplerCreateInfo sampler_create_info;
	sampler_create_info.magFilter     = vk::Filter::eLinear;
	sampler_create_info.minFilter     = vk::Filter::eLinear;
	sampler_create_info.mipmapMode    = vk::SamplerMipmapMode::eLinear;
	sampler_create_info.addressModeU  = vk::SamplerAddressMode::eMirroredRepeat;
	sampler_create_info.addressModeV  = sampler_create_info.addressModeU;
	sampler_create_info.addressModeW  = sampler_create_info.addressModeU;
	sampler_create_info.maxAnisotropy = 1.0f;
	sampler_create_info.compareOp     = vk::CompareOp::eNever;
	sampler_create_info.minLod        = 0.0f;
	sampler_create_info.maxLod        = static_cast<float>(textures.heightmap.image->get_mipmaps().size());
	sampler_create_info.borderColor   = vk::BorderColor::eFloatOpaqueWhite;
	get_device()->get_handle().destroySampler(textures.heightmap.sampler);
	textures.heightmap.sampler = get_device()->get_handle().createSampler(sampler_create_info);

	// Setup a repeating sampler for the terrain texture layers
	sampler_create_info.addressModeU = vk::SamplerAddressMode::eRepeat;
	sampler_create_info.addressModeV = sampler_create_info.addressModeU;
	sampler_create_info.addressModeW = sampler_create_info.addressModeU;
	sampler_create_info.maxLod       = static_cast<float>(textures.terrain_array.image->get_mipmaps().size());
	if (get_device()->get_gpu().get_features().samplerAnisotropy)
	{
		sampler_create_info.maxAnisotropy    = 4.0f;
		sampler_create_info.anisotropyEnable = true;
	}
	get_device()->get_handle().destroySampler(textures.terrain_array.sampler);
	textures.terrain_array.sampler = get_device()->get_handle().createSampler(sampler_create_info);
}

void HPPTerrainTessellation::prepare_pipelines()
{
	std::array<vk::PipelineShaderStageCreateInfo, 4> terrain_shader_stages = {{load_shader("terrain_tessellation/terrain.vert", vk::ShaderStageFlagBits::eVertex),
	                                                                           load_shader("terrain_tessellation/terrain.frag", vk::ShaderStageFlagBits::eFragment),
	                                                                           load_shader("terrain_tessellation/terrain.tesc", vk::ShaderStageFlagBits::eTessellationControl),
	                                                                           load_shader("terrain_tessellation/terrain.tese", vk::ShaderStageFlagBits::eTessellationEvaluation)}};

	// Vertex bindings an attributes
	// Binding description
	std::array<vk::VertexInputBindingDescription, 1> vertex_input_bindings = {{{0, sizeof(HPPTerrainTessellation::Vertex), vk::VertexInputRate::eVertex}}};

	// Attribute descriptions
	std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attributes = {{{0, 0, vk::Format::eR32G32B32Sfloat, 0},                        // Position
	                                                                               {1, 0, vk::Format::eR32G32B32Sfloat, sizeof(float) * 3},        // Normal
	                                                                               {2, 0, vk::Format::eR32G32Sfloat, sizeof(float) * 6}}};         // UV

	vk::PipelineVertexInputStateCreateInfo vertex_input_state({}, vertex_input_bindings, vertex_input_attributes);

	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state({}, vk::PrimitiveTopology::ePatchList, false);

	// We render the terrain as a grid of quad patches
	vk::PipelineTessellationStateCreateInfo tessellation_state({}, 4);

	vk::PipelineViewportStateCreateInfo viewport_state({}, 1, nullptr, 1, nullptr);

	vk::PipelineRasterizationStateCreateInfo rasterization_state;
	rasterization_state.polygonMode = vk::PolygonMode::eFill;
	rasterization_state.cullMode    = vk::CullModeFlagBits::eBack;
	rasterization_state.frontFace   = vk::FrontFace::eCounterClockwise;
	rasterization_state.lineWidth   = 1.0f;

	vk::PipelineMultisampleStateCreateInfo multisample_state({}, vk::SampleCountFlagBits::e1);

	// Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthCompareOp   = vk::CompareOp::eGreater;
	depth_stencil_state.depthTestEnable  = true;
	depth_stencil_state.depthWriteEnable = true;
	depth_stencil_state.back.compareOp   = vk::CompareOp::eGreater;
	depth_stencil_state.front            = depth_stencil_state.back;

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo color_blend_state({}, false, {}, blend_attachment_state);

	std::array<vk::DynamicState, 3> dynamic_state_enables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eLineWidth};

	vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_state_enables);

	vk::GraphicsPipelineCreateInfo pipeline_create_info({},
	                                                    terrain_shader_stages,
	                                                    &vertex_input_state,
	                                                    &input_assembly_state,
	                                                    &tessellation_state,
	                                                    &viewport_state,
	                                                    &rasterization_state,
	                                                    &multisample_state,
	                                                    &depth_stencil_state,
	                                                    &color_blend_state,
	                                                    &dynamic_state,
	                                                    pipeline_layouts.terrain,
	                                                    render_pass,
	                                                    {},
	                                                    {},
	                                                    -1);

	vk::Result result;
	std::tie(result, pipelines.terrain) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);

	// Terrain wireframe pipeline
	if (get_device()->get_gpu().get_features().fillModeNonSolid)
	{
		rasterization_state.polygonMode       = vk::PolygonMode::eLine;
		std::tie(result, pipelines.wireframe) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
		assert(result == vk::Result::eSuccess);
	};

	// Skysphere pipeline

	// Stride from glTF model vertex layout
	vertex_input_bindings[0].stride = sizeof(HPPVertex);

	rasterization_state.polygonMode = vk::PolygonMode::eFill;
	// Revert to triangle list topology
	input_assembly_state.topology = vk::PrimitiveTopology::eTriangleList;
	// Reset tessellation state
	pipeline_create_info.pTessellationState = nullptr;
	// Don't write to depth buffer
	depth_stencil_state.depthWriteEnable                                     = false;
	pipeline_create_info.layout                                              = pipeline_layouts.skysphere;
	std::array<vk::PipelineShaderStageCreateInfo, 2> skysphere_shader_stages = {{load_shader("terrain_tessellation/skysphere.vert", vk::ShaderStageFlagBits::eVertex),
	                                                                             load_shader("terrain_tessellation/skysphere.frag", vk::ShaderStageFlagBits::eFragment)}};
	pipeline_create_info.setStages(skysphere_shader_stages);

	std::tie(result, pipelines.skysphere) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);
}

// Prepare and initialize uniform buffer containing shader uniforms
void HPPTerrainTessellation::prepare_uniform_buffers()
{
	// Shared tessellation shader stages uniform buffer
	uniform_buffers.terrain_tessellation =
	    std::make_unique<vkb::core::HPPBuffer>(*get_device(), sizeof(ubo_tess), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Skysphere vertex shader uniform buffer
	uniform_buffers.skysphere_vertex =
	    std::make_unique<vkb::core::HPPBuffer>(*get_device(), sizeof(ubo_vs), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void HPPTerrainTessellation::setup_descriptor_pool()
{
	std::array<vk::DescriptorPoolSize, 2> pool_sizes = {{{vk::DescriptorType::eUniformBuffer, 3}, {vk::DescriptorType::eCombinedImageSampler, 3}}};

	vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 2, pool_sizes);

	descriptor_pool = get_device()->get_handle().createDescriptorPool(descriptor_pool_create_info);
}

void HPPTerrainTessellation::setup_descriptor_set_layouts()
{
	// Terrain
	std::array<vk::DescriptorSetLayoutBinding, 3> terrain_layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eTessellationEvaluation},
	     {1,
	      vk::DescriptorType::eCombinedImageSampler,
	      1,
	      vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eTessellationEvaluation | vk::ShaderStageFlagBits::eFragment},
	     {2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::DescriptorSetLayoutCreateInfo terrain_descriptor_layout({}, terrain_layout_bindings);
	descriptor_set_layouts.terrain = get_device()->get_handle().createDescriptorSetLayout(terrain_descriptor_layout);
#if defined(ANDROID)
	vk::PipelineLayoutCreateInfo terrain_pipeline_layout_create_info({}, 1, &descriptor_set_layouts.terrain);
#else
	vk::PipelineLayoutCreateInfo  terrain_pipeline_layout_create_info({}, descriptor_set_layouts.terrain);
#endif
	pipeline_layouts.terrain = get_device()->get_handle().createPipelineLayout(terrain_pipeline_layout_create_info);

	// Skysphere
	std::array<vk::DescriptorSetLayoutBinding, 2> skysphere_layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
	     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::DescriptorSetLayoutCreateInfo skysphere_descriptor_layout({}, skysphere_layout_bindings);
	descriptor_set_layouts.skysphere = get_device()->get_handle().createDescriptorSetLayout(skysphere_descriptor_layout);
#if defined(ANDROID)
	vk::PipelineLayoutCreateInfo skysphere_pipeline_layout_create_info({}, 1, &descriptor_set_layouts.skysphere);
#else
	vk::PipelineLayoutCreateInfo  skysphere_pipeline_layout_create_info({}, descriptor_set_layouts.skysphere);
#endif
	pipeline_layouts.skysphere = get_device()->get_handle().createPipelineLayout(skysphere_pipeline_layout_create_info);
}

void HPPTerrainTessellation::setup_descriptor_sets()
{
	// Terrain
#if defined(ANDROID)
	vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool, 1, &descriptor_set_layouts.terrain);
#else
	vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool, descriptor_set_layouts.terrain);
#endif
	descriptor_sets.terrain = get_device()->get_handle().allocateDescriptorSets(alloc_info).front();

	vk::DescriptorBufferInfo terrain_buffer_descriptor(uniform_buffers.terrain_tessellation->get_handle(), 0, VK_WHOLE_SIZE);
	vk::DescriptorImageInfo  heightmap_image_descriptor(
        textures.heightmap.sampler,
        textures.heightmap.image->get_vk_image_view().get_handle(),
        descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, textures.heightmap.image->get_vk_image_view().get_format()));
	vk::DescriptorImageInfo terrainmap_image_descriptor(
	    textures.terrain_array.sampler,
	    textures.terrain_array.image->get_vk_image_view().get_handle(),
	    descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, textures.terrain_array.image->get_vk_image_view().get_format()));
	std::array<vk::WriteDescriptorSet, 3> terrain_write_descriptor_sets = {
	    {{descriptor_sets.terrain, 0, {}, vk::DescriptorType::eUniformBuffer, {}, terrain_buffer_descriptor},
	     {descriptor_sets.terrain, 1, {}, vk::DescriptorType::eCombinedImageSampler, heightmap_image_descriptor},
	     {descriptor_sets.terrain, 2, {}, vk::DescriptorType::eCombinedImageSampler, terrainmap_image_descriptor}}};
	get_device()->get_handle().updateDescriptorSets(terrain_write_descriptor_sets, {});

	// Skysphere
	alloc_info.setSetLayouts(descriptor_set_layouts.skysphere);
	descriptor_sets.skysphere = get_device()->get_handle().allocateDescriptorSets(alloc_info).front();

	vk::DescriptorBufferInfo skysphere_buffer_descriptor(uniform_buffers.skysphere_vertex->get_handle(), 0, VK_WHOLE_SIZE);
	vk::DescriptorImageInfo  skysphere_image_descriptor(
        textures.skysphere.sampler,
        textures.skysphere.image->get_vk_image_view().get_handle(),
        descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, textures.skysphere.image->get_vk_image_view().get_format()));
	std::array<vk::WriteDescriptorSet, 2> skysphere_write_descriptor_sets = {
	    {{descriptor_sets.skysphere, 0, {}, vk::DescriptorType::eUniformBuffer, {}, skysphere_buffer_descriptor},
	     {descriptor_sets.skysphere, 1, {}, vk::DescriptorType::eCombinedImageSampler, skysphere_image_descriptor}}};
	get_device()->get_handle().updateDescriptorSets(skysphere_write_descriptor_sets, {});
}

// Setup pool and buffer for storing pipeline statistics results
void HPPTerrainTessellation::setup_query_result_buffer()
{
	const uint32_t buffer_size = 2 * sizeof(uint64_t);

	vk::BufferCreateInfo buffer_create_info({}, buffer_size, vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst);

	// Results are saved in a host visible buffer for easy access by the application
	query_result.buffer                        = get_device()->get_handle().createBuffer(buffer_create_info);
	vk::MemoryRequirements memory_requirements = get_device()->get_handle().getBufferMemoryRequirements(query_result.buffer);
	vk::MemoryAllocateInfo memory_allocation(
	    memory_requirements.size,
	    get_device()->get_gpu().get_memory_type(memory_requirements.memoryTypeBits,
	                                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
	query_result.memory = get_device()->get_handle().allocateMemory(memory_allocation);
	get_device()->get_handle().bindBufferMemory(query_result.buffer, query_result.memory, 0);

	// Create query pool
	if (get_device()->get_gpu().get_features().pipelineStatisticsQuery)
	{
		vk::QueryPoolCreateInfo query_pool_info({},
		                                        vk::QueryType::ePipelineStatistics,
		                                        2,
		                                        vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations |
		                                            vk::QueryPipelineStatisticFlagBits::eTessellationEvaluationShaderInvocations);
		query_pool = get_device()->get_handle().createQueryPool(query_pool_info);
	}
}

void HPPTerrainTessellation::update_uniform_buffers()
{
	// Tessellation
	ubo_tess.projection   = camera.matrices.perspective;
	ubo_tess.modelview    = camera.matrices.view * glm::mat4(1.0f);
	ubo_tess.light_pos.y  = -0.5f - ubo_tess.displacement_factor;        // todo: Not used yet
	ubo_tess.viewport_dim = glm::vec2(static_cast<float>(extent.width), static_cast<float>(extent.height));

	frustum.update(ubo_tess.projection * ubo_tess.modelview);
	memcpy(ubo_tess.frustum_planes, frustum.get_planes().data(), sizeof(glm::vec4) * 6);

	float saved_factor = ubo_tess.tessellation_factor;
	if (!tessellation)
	{
		// Setting this to zero sets all tessellation factors to 1.0 in the shader
		ubo_tess.tessellation_factor = 0.0f;
	}

	uniform_buffers.terrain_tessellation->convert_and_update(ubo_tess);

	if (!tessellation)
	{
		ubo_tess.tessellation_factor = saved_factor;
	}

	// Skysphere vertex shader
	ubo_vs.mvp = camera.matrices.perspective * glm::mat4(glm::mat3(camera.matrices.view));
	uniform_buffers.skysphere_vertex->convert_and_update(ubo_vs.mvp);
}

std::unique_ptr<vkb::Application> create_hpp_terrain_tessellation()
{
	return std::make_unique<HPPTerrainTessellation>();
}
