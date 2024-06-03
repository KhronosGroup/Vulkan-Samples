/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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

#include <core/hpp_command_pool.h>
#include <heightmap.h>

HPPTerrainTessellation::HPPTerrainTessellation()
{
	title = "HPP Dynamic terrain tessellation";
}

HPPTerrainTessellation::~HPPTerrainTessellation()
{
	if (has_device() && get_device().get_handle())
	{
		vk::Device device = get_device().get_handle();

		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class
		sky_sphere.destroy(device);
		terrain.destroy(device);
		wireframe.destroy(device);
		statistics.destroy(device);
	}
}

bool HPPTerrainTessellation::prepare(const vkb::ApplicationOptions &options)
{
	assert(!prepared);

	if (HPPApiVulkanSample::prepare(options))
	{
		prepare_camera();
		load_assets();
		generate_terrain();
		prepare_uniform_buffers();
		descriptor_pool = create_descriptor_pool();
		prepare_sky_sphere();
		prepare_terrain();
		prepare_wireframe();
		prepare_statistics();
		build_command_buffers();

		prepared = true;
	}

	return prepared;
}

void HPPTerrainTessellation::request_gpu_features(vkb::core::HPPPhysicalDevice &gpu)
{
	// Tessellation shader support is required for this example
	auto &available_features = gpu.get_features();
	if (!available_features.tessellationShader)
	{
		throw vkb::VulkanException(VK_ERROR_FEATURE_NOT_PRESENT, "Selected GPU does not support tessellation shaders!");
	}

	auto &requested_features = gpu.get_mutable_requested_features();

	requested_features.tessellationShader = true;

	// Fill mode non solid is required for wireframe display
	requested_features.fillModeNonSolid = available_features.fillModeNonSolid;
	wireframe.supported                 = available_features.fillModeNonSolid;

	// Pipeline statistics
	requested_features.pipelineStatisticsQuery = available_features.pipelineStatisticsQuery;
	statistics.query_supported                 = available_features.pipelineStatisticsQuery;

	// Enable anisotropic filtering if supported
	requested_features.samplerAnisotropy = available_features.samplerAnisotropy;
	terrain.sampler_anisotropy_supported = available_features.samplerAnisotropy;
}

void HPPTerrainTessellation::build_command_buffers()
{
	vk::CommandBufferBeginInfo    command_buffer_begin_info;
	std::array<vk::ClearValue, 2> clear_values = {{default_clear_color, vk::ClearDepthStencilValue(0.0f, 0)}};

	vk::RenderPassBeginInfo render_pass_begin_info(render_pass, {}, {{0, 0}, extent}, clear_values);

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		auto command_buffer = draw_cmd_buffers[i];
		command_buffer.begin(command_buffer_begin_info);

		if (statistics.query_supported)
		{
			command_buffer.resetQueryPool(statistics.query_pool, 0, 2);
		}

		render_pass_begin_info.framebuffer = framebuffers[i];
		command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

		vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
		command_buffer.setViewport(0, viewport);

		vk::Rect2D scissor({0, 0}, extent);
		command_buffer.setScissor(0, scissor);

		vk::DeviceSize offset = 0;

		// Skysphere
		command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, sky_sphere.pipeline);
		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, sky_sphere.pipeline_layout, 0, sky_sphere.descriptor_set, {});
		draw_model(sky_sphere.geometry, command_buffer);

		// Terrain
		if (statistics.query_supported)
		{
			// Begin pipeline statistics query
			command_buffer.beginQuery(statistics.query_pool, 0, {});
		}

		// Render
		command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, wireframe.enabled ? wireframe.pipeline : terrain.pipeline);
		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, terrain.pipeline_layout, 0, terrain.descriptor_set, {});
		command_buffer.bindVertexBuffers(0, terrain.vertices->get_handle(), offset);
		command_buffer.bindIndexBuffer(terrain.indices->get_handle(), 0, vk::IndexType::eUint32);
		command_buffer.drawIndexed(terrain.index_count, 1, 0, 0, 0);

		if (statistics.query_supported)
		{
			// End pipeline statistics query
			command_buffer.endQuery(statistics.query_pool, 0);
		}

		draw_ui(command_buffer);

		command_buffer.endRenderPass();

		command_buffer.end();
	}
}

void HPPTerrainTessellation::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.checkbox("Tessellation", &terrain.tessellation_enabled))
		{
			update_uniform_buffers();
		}
		if (drawer.input_float("Factor", &terrain.tessellation.tessellation_factor, 0.05f, "%.2f"))
		{
			update_uniform_buffers();
		}
		if (wireframe.supported)
		{
			if (drawer.checkbox("Wireframe", &wireframe.enabled))
			{
				rebuild_command_buffers();
			}
		}
	}
	if (statistics.query_supported)
	{
		if (drawer.header("Pipeline statistics"))
		{
			drawer.text("VS invocations: %d", statistics.results[0]);
			drawer.text("TE invocations: %d", statistics.results[1]);
		}
	}
}

void HPPTerrainTessellation::render(float delta_time)
{
	if (prepared)
	{
		draw();
	}
}

void HPPTerrainTessellation::view_changed()
{
	update_uniform_buffers();
}

vk::DescriptorPool HPPTerrainTessellation::create_descriptor_pool()
{
	std::array<vk::DescriptorPoolSize, 2> pool_sizes = {{{vk::DescriptorType::eUniformBuffer, 3}, {vk::DescriptorType::eCombinedImageSampler, 3}}};
	return get_device().get_handle().createDescriptorPool({{}, 2, pool_sizes});
}

vk::DescriptorSetLayout HPPTerrainTessellation::create_sky_sphere_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 2> layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
	     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::DescriptorSetLayoutCreateInfo skysphere_descriptor_layout({}, layout_bindings);
	return get_device().get_handle().createDescriptorSetLayout(skysphere_descriptor_layout);
}

vk::Pipeline HPPTerrainTessellation::create_sky_sphere_pipeline()
{
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {
	    load_shader("terrain_tessellation", "skysphere.vert", vk::ShaderStageFlagBits::eVertex),
	    load_shader("terrain_tessellation", "skysphere.frag", vk::ShaderStageFlagBits::eFragment)};

	// Vertex bindings an attributes
	// Binding description
	std::array<vk::VertexInputBindingDescription, 1> vertex_input_bindings = {{{0, sizeof(HPPVertex), vk::VertexInputRate::eVertex}}};

	// Attribute descriptions
	std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attributes = {{{0, 0, vk::Format::eR32G32B32Sfloat, 0},                        // Position
	                                                                               {1, 0, vk::Format::eR32G32B32Sfloat, sizeof(float) * 3},        // Normal
	                                                                               {2, 0, vk::Format::eR32G32Sfloat, sizeof(float) * 6}}};         // UV

	vk::PipelineVertexInputStateCreateInfo vertex_input_state({}, vertex_input_bindings, vertex_input_attributes);

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthCompareOp   = vk::CompareOp::eGreater;
	depth_stencil_state.depthTestEnable  = true;
	depth_stencil_state.depthWriteEnable = false;
	depth_stencil_state.back.compareOp   = vk::CompareOp::eGreater;
	depth_stencil_state.front            = depth_stencil_state.back;

	// For the sky_sphere use triangle list topology
	return vkb::common::create_graphics_pipeline(get_device().get_handle(),
	                                             pipeline_cache,
	                                             shader_stages,
	                                             vertex_input_state,
	                                             vk::PrimitiveTopology::eTriangleList,
	                                             0,
	                                             vk::PolygonMode::eFill,
	                                             vk::CullModeFlagBits::eBack,
	                                             vk::FrontFace::eCounterClockwise,
	                                             {blend_attachment_state},
	                                             depth_stencil_state,
	                                             sky_sphere.pipeline_layout,
	                                             render_pass);
}

vk::DescriptorSetLayout HPPTerrainTessellation::create_terrain_descriptor_set_layout()
{
	// Terrain
	std::array<vk::DescriptorSetLayoutBinding, 3> layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eTessellationEvaluation},
	     {1,
	      vk::DescriptorType::eCombinedImageSampler,
	      1,
	      vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eTessellationEvaluation | vk::ShaderStageFlagBits::eFragment},
	     {2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::DescriptorSetLayoutCreateInfo terrain_descriptor_layout({}, layout_bindings);
	return get_device().get_handle().createDescriptorSetLayout(terrain_descriptor_layout);
}

vk::Pipeline HPPTerrainTessellation::create_terrain_pipeline(vk::PolygonMode polygon_mode)
{
	// Vertex bindings an attributes
	// Binding description
	std::array<vk::VertexInputBindingDescription, 1> vertex_input_bindings = {
	    {{0, sizeof(HPPTerrainTessellation::Vertex), vk::VertexInputRate::eVertex}}};

	// Attribute descriptions
	std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attributes = {{{0, 0, vk::Format::eR32G32B32Sfloat, 0},                        // Position
	                                                                               {1, 0, vk::Format::eR32G32B32Sfloat, sizeof(float) * 3},        // Normal
	                                                                               {2, 0, vk::Format::eR32G32Sfloat, sizeof(float) * 6}}};         // UV

	vk::PipelineVertexInputStateCreateInfo vertex_input_state({}, vertex_input_bindings, vertex_input_attributes);

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthCompareOp   = vk::CompareOp::eGreater;
	depth_stencil_state.depthTestEnable  = true;
	depth_stencil_state.depthWriteEnable = true;
	depth_stencil_state.back.compareOp   = vk::CompareOp::eGreater;
	depth_stencil_state.front            = depth_stencil_state.back;

	return vkb::common::create_graphics_pipeline(get_device().get_handle(),
	                                             pipeline_cache,
	                                             terrain.shader_stages,
	                                             vertex_input_state,
	                                             vk::PrimitiveTopology::ePatchList,
	                                             4,        // we render the terrain as a grid of quad patches
	                                             polygon_mode,
	                                             vk::CullModeFlagBits::eBack,
	                                             vk::FrontFace::eCounterClockwise,
	                                             {blend_attachment_state},
	                                             depth_stencil_state,
	                                             terrain.pipeline_layout,
	                                             render_pass);
}

void HPPTerrainTessellation::draw()
{
	HPPApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);

	// Submit to queue
	queue.submit(submit_info);

	if (statistics.query_supported)
	{
		// Read query results for displaying in next frame
		statistics.results =
		    get_device()
		        .get_handle()
		        .getQueryPoolResult<std::array<uint64_t, 2>>(statistics.query_pool, 0, 1, sizeof(statistics.results), vk::QueryResultFlagBits::e64)
		        .value;
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

	for (auto x = 0; x < patch_size; x++)
	{
		for (auto y = 0; y < patch_size; y++)
		{
			uint32_t index         = (x + y * patch_size);
			vertices[index].pos[0] = 2.0f * x + 1.0f - patch_size;
			vertices[index].pos[1] = 0.0f;
			vertices[index].pos[2] = 2.0f * y * 1.0f - patch_size;
			vertices[index].uv     = glm::vec2(static_cast<float>(x) / patch_size, static_cast<float>(y) / patch_size) * uv_scale;
		}
	}

	// Calculate normals from height map using a sobel filter
	vkb::HeightMap height_map("textures/terrain_heightmap_r16.ktx", patch_size);
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
					heights[hx + 1][hy + 1] = height_map.get_height(x + hx, y + hy);
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

	// Create staging buffers

	vkb::core::HPPBuffer vertex_staging(get_device(), vertex_buffer_size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_staging.update(vertices);

	vkb::core::HPPBuffer index_staging(get_device(), index_buffer_size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
	index_staging.update(indices);

	terrain.vertices = std::make_unique<vkb::core::HPPBuffer>(
	    get_device(), vertex_buffer_size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY);

	terrain.indices = std::make_unique<vkb::core::HPPBuffer>(
	    get_device(), index_buffer_size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY);

	// Copy from staging buffers
	vk::CommandBuffer copy_command = vkb::common::allocate_command_buffer(get_device().get_handle(), get_device().get_command_pool().get_handle());
	copy_command.begin(vk::CommandBufferBeginInfo());
	copy_command.copyBuffer(vertex_staging.get_handle(), terrain.vertices->get_handle(), {{0, 0, vertex_buffer_size}});
	copy_command.copyBuffer(index_staging.get_handle(), terrain.indices->get_handle(), {{0, 0, index_buffer_size}});
	get_device().flush_command_buffer(copy_command, queue, true);
}

void HPPTerrainTessellation::load_assets()
{
	sky_sphere.geometry = load_model("scenes/geosphere.gltf");
	sky_sphere.texture  = load_texture("textures/skysphere_rgba.ktx", vkb::scene_graph::components::HPPImage::Color);

	// Terrain textures are stored in a texture array with layers corresponding to terrain height; create a repeating sampler
	terrain.terrain_array = load_texture_array("textures/terrain_texturearray_rgba.ktx", vkb::scene_graph::components::HPPImage::Color, vk::SamplerAddressMode::eRepeat);

	// Height data is stored in a one-channel texture; create a mirroring sampler
	terrain.height_map = load_texture("textures/terrain_heightmap_r16.ktx", vkb::scene_graph::components::HPPImage::Other, vk::SamplerAddressMode::eMirroredRepeat);
}

void HPPTerrainTessellation::prepare_camera()
{
	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.type = vkb::CameraType::FirstPerson;
	camera.set_perspective(60.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 512.0f, 0.1f);
	camera.set_rotation(glm::vec3(-12.0f, 159.0f, 0.0f));
	camera.set_translation(glm::vec3(18.0f, 22.5f, 57.5f));
	camera.translation_speed = 7.5f;
}

void HPPTerrainTessellation::prepare_sky_sphere()
{
	sky_sphere.descriptor_set_layout = create_sky_sphere_descriptor_set_layout();
	sky_sphere.pipeline_layout       = get_device().get_handle().createPipelineLayout({{}, sky_sphere.descriptor_set_layout});
	sky_sphere.pipeline              = create_sky_sphere_pipeline();
	sky_sphere.descriptor_set        = vkb::common::allocate_descriptor_set(get_device().get_handle(), descriptor_pool, {sky_sphere.descriptor_set_layout});
	update_sky_sphere_descriptor_set();
}

void HPPTerrainTessellation::prepare_statistics()
{
	if (statistics.query_supported)
	{
		// Create query pool
		statistics.query_pool = vkb::common::create_query_pool(get_device().get_handle(),
		                                                       vk::QueryType::ePipelineStatistics,
		                                                       2,
		                                                       vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations |
		                                                           vk::QueryPipelineStatisticFlagBits::eTessellationEvaluationShaderInvocations);
	}
}

void HPPTerrainTessellation::prepare_terrain()
{
	terrain.shader_stages = {load_shader("terrain_tessellation", "terrain.vert", vk::ShaderStageFlagBits::eVertex),
	                         load_shader("terrain_tessellation", "terrain.frag", vk::ShaderStageFlagBits::eFragment),
	                         load_shader("terrain_tessellation", "terrain.tesc", vk::ShaderStageFlagBits::eTessellationControl),
	                         load_shader("terrain_tessellation", "terrain.tese", vk::ShaderStageFlagBits::eTessellationEvaluation)};

	terrain.descriptor_set_layout = create_terrain_descriptor_set_layout();
	terrain.pipeline_layout       = get_device().get_handle().createPipelineLayout({{}, terrain.descriptor_set_layout});
	terrain.pipeline              = create_terrain_pipeline(vk::PolygonMode::eFill);
	terrain.descriptor_set        = vkb::common::allocate_descriptor_set(get_device().get_handle(), descriptor_pool, {terrain.descriptor_set_layout});
	update_terrain_descriptor_set();
}

// Prepare and initialize uniform buffer containing shader uniforms
void HPPTerrainTessellation::prepare_uniform_buffers()
{
	// Shared tessellation shader stages uniform buffer
	terrain.tessellation_buffer =
	    std::make_unique<vkb::core::HPPBuffer>(get_device(), sizeof(terrain.tessellation), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Skysphere vertex shader uniform buffer
	sky_sphere.transform_buffer =
	    std::make_unique<vkb::core::HPPBuffer>(get_device(), sizeof(sky_sphere.transform), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void HPPTerrainTessellation::prepare_wireframe()
{
	if (wireframe.supported)
	{
		// wireframe mode uses nearly the same settings as the terrain mode... just vk::PolygonMode::eLine, instead of vk::PolygonMode::eFill
		wireframe.pipeline = create_terrain_pipeline(vk::PolygonMode::eLine);
	};
}

void HPPTerrainTessellation::update_uniform_buffers()
{
	// Tessellation
	terrain.tessellation.projection   = camera.matrices.perspective;
	terrain.tessellation.modelview    = camera.matrices.view * glm::mat4(1.0f);
	terrain.tessellation.light_pos.y  = -0.5f - terrain.tessellation.displacement_factor;        // todo: Not used yet
	terrain.tessellation.viewport_dim = glm::vec2(static_cast<float>(extent.width), static_cast<float>(extent.height));

	frustum.update(terrain.tessellation.projection * terrain.tessellation.modelview);
	memcpy(terrain.tessellation.frustum_planes, frustum.get_planes().data(), sizeof(glm::vec4) * 6);

	float saved_factor = terrain.tessellation.tessellation_factor;
	if (!terrain.tessellation_enabled)
	{
		// Setting this to zero sets all tessellation factors to 1.0 in the shader
		terrain.tessellation.tessellation_factor = 0.0f;
	}

	terrain.tessellation_buffer->convert_and_update(terrain.tessellation);

	if (!terrain.tessellation_enabled)
	{
		terrain.tessellation.tessellation_factor = saved_factor;
	}

	// Skysphere vertex shader
	sky_sphere.transform = camera.matrices.perspective * glm::mat4(glm::mat3(camera.matrices.view));
	sky_sphere.transform_buffer->convert_and_update(sky_sphere.transform);
}

void HPPTerrainTessellation::update_sky_sphere_descriptor_set()
{
	vk::DescriptorBufferInfo skysphere_buffer_descriptor(sky_sphere.transform_buffer->get_handle(), 0, VK_WHOLE_SIZE);

	vk::DescriptorImageInfo skysphere_image_descriptor(
	    sky_sphere.texture.sampler,
	    sky_sphere.texture.image->get_vk_image_view().get_handle(),
	    descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, sky_sphere.texture.image->get_vk_image_view().get_format()));

	std::array<vk::WriteDescriptorSet, 2> skysphere_write_descriptor_sets = {
	    {{sky_sphere.descriptor_set, 0, {}, vk::DescriptorType::eUniformBuffer, {}, skysphere_buffer_descriptor},
	     {sky_sphere.descriptor_set, 1, {}, vk::DescriptorType::eCombinedImageSampler, skysphere_image_descriptor}}};

	get_device().get_handle().updateDescriptorSets(skysphere_write_descriptor_sets, {});
}

void HPPTerrainTessellation::update_terrain_descriptor_set()
{
	vk::DescriptorBufferInfo terrain_buffer_descriptor(terrain.tessellation_buffer->get_handle(), 0, VK_WHOLE_SIZE);

	vk::DescriptorImageInfo heightmap_image_descriptor(
	    terrain.height_map.sampler,
	    terrain.height_map.image->get_vk_image_view().get_handle(),
	    descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, terrain.height_map.image->get_vk_image_view().get_format()));

	vk::DescriptorImageInfo terrainmap_image_descriptor(
	    terrain.terrain_array.sampler,
	    terrain.terrain_array.image->get_vk_image_view().get_handle(),
	    descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, terrain.terrain_array.image->get_vk_image_view().get_format()));

	std::array<vk::WriteDescriptorSet, 3> terrain_write_descriptor_sets = {
	    {{terrain.descriptor_set, 0, {}, vk::DescriptorType::eUniformBuffer, {}, terrain_buffer_descriptor},
	     {terrain.descriptor_set, 1, {}, vk::DescriptorType::eCombinedImageSampler, heightmap_image_descriptor},
	     {terrain.descriptor_set, 2, {}, vk::DescriptorType::eCombinedImageSampler, terrainmap_image_descriptor}}};

	get_device().get_handle().updateDescriptorSets(terrain_write_descriptor_sets, {});
}

std::unique_ptr<vkb::Application> create_hpp_terrain_tessellation()
{
	return std::make_unique<HPPTerrainTessellation>();
}
