/* Copyright (c) 2019-2022, Sascha Willems
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
 * Dynamic terrain tessellation
 */

#include "terrain_tessellation.h"

#include "heightmap.h"

TerrainTessellation::TerrainTessellation()
{
	title = "Dynamic terrain tessellation";
}

TerrainTessellation::~TerrainTessellation()
{
	if (device)
	{
		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(get_device().get_handle(), pipelines.terrain, nullptr);
		if (pipelines.wireframe != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(get_device().get_handle(), pipelines.wireframe, nullptr);
		}
		vkDestroyPipeline(get_device().get_handle(), pipelines.skysphere, nullptr);

		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.skysphere, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.terrain, nullptr);

		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.terrain, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.skysphere, nullptr);

		uniform_buffers.skysphere_vertex.reset();
		uniform_buffers.terrain_tessellation.reset();

		textures.heightmap.image.reset();
		vkDestroySampler(get_device().get_handle(), textures.heightmap.sampler, nullptr);
		textures.skysphere.image.reset();
		vkDestroySampler(get_device().get_handle(), textures.skysphere.sampler, nullptr);
		textures.terrain_array.image.reset();
		vkDestroySampler(get_device().get_handle(), textures.terrain_array.sampler, nullptr);

		if (query_pool != VK_NULL_HANDLE)
		{
			vkDestroyQueryPool(get_device().get_handle(), query_pool, nullptr);
			vkDestroyBuffer(get_device().get_handle(), query_result.buffer, nullptr);
			vkFreeMemory(get_device().get_handle(), query_result.memory, nullptr);
		}
	}
}

void TerrainTessellation::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	auto &requested_features = gpu.get_mutable_requested_features();

	// Tessellation shader support is required for this example
	if (gpu.get_features().tessellationShader)
	{
		requested_features.tessellationShader = VK_TRUE;
	}
	else
	{
		throw vkb::VulkanException(VK_ERROR_FEATURE_NOT_PRESENT, "Selected GPU does not support tessellation shaders!");
	}

	// Fill mode non solid is required for wireframe display
	if (gpu.get_features().fillModeNonSolid)
	{
		requested_features.fillModeNonSolid = VK_TRUE;
	}

	// Pipeline statistics
	if (gpu.get_features().pipelineStatisticsQuery)
	{
		requested_features.pipelineStatisticsQuery = VK_TRUE;
	}

	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		requested_features.samplerAnisotropy = VK_TRUE;
	}
}

// Setup pool and buffer for storing pipeline statistics results
void TerrainTessellation::setup_query_result_buffer()
{
	uint32_t buffer_size = 2 * sizeof(uint64_t);

	VkMemoryRequirements memory_requirements;
	VkMemoryAllocateInfo memory_allocation = vkb::initializers::memory_allocate_info();
	VkBufferCreateInfo   buffer_create_info =
	    vkb::initializers::buffer_create_info(
	        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	        buffer_size);

	// Results are saved in a host visible buffer for easy access by the application
	VK_CHECK(vkCreateBuffer(get_device().get_handle(), &buffer_create_info, nullptr, &query_result.buffer));
	vkGetBufferMemoryRequirements(get_device().get_handle(), query_result.buffer, &memory_requirements);
	memory_allocation.allocationSize  = memory_requirements.size;
	memory_allocation.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocation, nullptr, &query_result.memory));
	VK_CHECK(vkBindBufferMemory(get_device().get_handle(), query_result.buffer, query_result.memory, 0));

	// Create query pool
	if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
	{
		VkQueryPoolCreateInfo query_pool_info = {};
		query_pool_info.sType                 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		query_pool_info.queryType             = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		query_pool_info.pipelineStatistics =
		    VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
		    VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT;
		query_pool_info.queryCount = 2;
		VK_CHECK(vkCreateQueryPool(get_device().get_handle(), &query_pool_info, NULL, &query_pool));
	}
}

// Retrieves the results of the pipeline statistics query submitted to the command buffer
void TerrainTessellation::get_query_results()
{
	// We use vkGetQueryResults to copy the results into a host visible buffer
	vkGetQueryPoolResults(
	    get_device().get_handle(),
	    query_pool,
	    0,
	    1,
	    sizeof(pipeline_stats),
	    pipeline_stats,
	    sizeof(uint64_t),
	    VK_QUERY_RESULT_64_BIT);
}

void TerrainTessellation::load_assets()
{
	skysphere = load_model("scenes/geosphere.gltf");

	textures.skysphere = load_texture("textures/skysphere_rgba.ktx", vkb::sg::Image::Color);
	// Terrain textures are stored in a texture array with layers corresponding to terrain height
	textures.terrain_array = load_texture_array("textures/terrain_texturearray_rgba.ktx", vkb::sg::Image::Color);

	// Height data is stored in a one-channel texture
	textures.heightmap = load_texture("textures/terrain_heightmap_r16.ktx", vkb::sg::Image::Other);

	VkSamplerCreateInfo sampler_create_info = vkb::initializers::sampler_create_info();

	// Setup a mirroring sampler for the height map
	vkDestroySampler(get_device().get_handle(), textures.heightmap.sampler, nullptr);
	sampler_create_info.magFilter    = VK_FILTER_LINEAR;
	sampler_create_info.minFilter    = VK_FILTER_LINEAR;
	sampler_create_info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	sampler_create_info.addressModeV = sampler_create_info.addressModeU;
	sampler_create_info.addressModeW = sampler_create_info.addressModeU;
	sampler_create_info.compareOp    = VK_COMPARE_OP_NEVER;
	sampler_create_info.minLod       = 0.0f;
	sampler_create_info.maxLod       = static_cast<float>(textures.heightmap.image->get_mipmaps().size());
	sampler_create_info.borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_create_info, nullptr, &textures.heightmap.sampler));

	// Setup a repeating sampler for the terrain texture layers
	vkDestroySampler(get_device().get_handle(), textures.terrain_array.sampler, nullptr);
	sampler_create_info              = vkb::initializers::sampler_create_info();
	sampler_create_info.magFilter    = VK_FILTER_LINEAR;
	sampler_create_info.minFilter    = VK_FILTER_LINEAR;
	sampler_create_info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeV = sampler_create_info.addressModeU;
	sampler_create_info.addressModeW = sampler_create_info.addressModeU;
	sampler_create_info.compareOp    = VK_COMPARE_OP_NEVER;
	sampler_create_info.minLod       = 0.0f;
	sampler_create_info.maxLod       = static_cast<float>(textures.terrain_array.image->get_mipmaps().size());
	sampler_create_info.borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	if (get_device().get_gpu().get_features().samplerAnisotropy)
	{
		sampler_create_info.maxAnisotropy    = 4.0f;
		sampler_create_info.anisotropyEnable = VK_TRUE;
	}
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_create_info, nullptr, &textures.terrain_array.sampler));
}

void TerrainTessellation::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = default_clear_color;
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
		{
			vkCmdResetQueryPool(draw_cmd_buffers[i], query_pool, 0, 2);
		}

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdSetLineWidth(draw_cmd_buffers[i], 1.0f);

		VkDeviceSize offsets[1] = {0};

		// Skysphere
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skysphere);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.skysphere, 0, 1, &descriptor_sets.skysphere, 0, NULL);
		draw_model(skysphere, draw_cmd_buffers[i]);

		// Terrain
		if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
		{
			// Begin pipeline statistics query
			vkCmdBeginQuery(draw_cmd_buffers[i], query_pool, 0, 0);
		}
		// Render
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? pipelines.wireframe : pipelines.terrain);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.terrain, 0, 1, &descriptor_sets.terrain, 0, NULL);
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, terrain.vertices->get(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], terrain.indices->get_handle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(draw_cmd_buffers[i], terrain.index_count, 1, 0, 0, 0);
		if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
		{
			// End pipeline statistics query
			vkCmdEndQuery(draw_cmd_buffers[i], query_pool, 0);
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

// Generate a terrain quad patch for feeding to the tessellation control shader
void TerrainTessellation::generate_terrain()
{
	const uint32_t patch_size   = 64;
	const float    uv_scale     = 1.0f;
	const uint32_t vertex_count = patch_size * patch_size;
	Vertex        *vertices     = new Vertex[vertex_count];

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
	const uint32_t w           = (patch_size - 1);
	const uint32_t index_count = w * w * 4;
	uint32_t      *indices     = new uint32_t[index_count];
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
		VkBuffer       buffer;
		VkDeviceMemory memory;
	} vertex_staging, index_staging;

	// Create staging buffers

	vertex_staging.buffer = get_device().create_buffer(
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	    vertex_buffer_size,
	    &vertex_staging.memory,
	    vertices);

	index_staging.buffer = get_device().create_buffer(
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	    index_buffer_size,
	    &index_staging.memory,
	    indices);

	terrain.vertices = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                       vertex_buffer_size,
	                                                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                       VMA_MEMORY_USAGE_GPU_ONLY);

	terrain.indices = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                      index_buffer_size,
	                                                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                      VMA_MEMORY_USAGE_GPU_ONLY);

	// Copy from staging buffers
	VkCommandBuffer copy_command = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copy_region = {};

	copy_region.size = vertex_buffer_size;
	vkCmdCopyBuffer(
	    copy_command,
	    vertex_staging.buffer,
	    terrain.vertices->get_handle(),
	    1,
	    &copy_region);

	copy_region.size = index_buffer_size;
	vkCmdCopyBuffer(
	    copy_command,
	    index_staging.buffer,
	    terrain.indices->get_handle(),
	    1,
	    &copy_region);

	device->flush_command_buffer(copy_command, queue, true);

	vkDestroyBuffer(get_device().get_handle(), vertex_staging.buffer, nullptr);
	vkFreeMemory(get_device().get_handle(), vertex_staging.memory, nullptr);
	vkDestroyBuffer(get_device().get_handle(), index_staging.buffer, nullptr);
	vkFreeMemory(get_device().get_handle(), index_staging.memory, nullptr);

	delete[] vertices;
	delete[] indices;
}

void TerrainTessellation::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes =
	    {
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3)};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        2);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void TerrainTessellation::setup_descriptor_set_layouts()
{
	VkDescriptorSetLayoutCreateInfo           descriptor_layout;
	VkPipelineLayoutCreateInfo                pipeline_layout_create_info;
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings;

	// Terrain
	set_layout_bindings =
	    {
	        // Binding 0 : Shared Tessellation shader ubo
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
	            0),
	        // Binding 1 : Height map
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	            VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	            1),
	        // Binding 3 : Terrain texture array layers
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	            VK_SHADER_STAGE_FRAGMENT_BIT,
	            2),
	    };

	descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layouts.terrain));
	pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layouts.terrain, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.terrain));

	// Skysphere
	set_layout_bindings =
	    {
	        // Binding 0 : Vertex shader ubo
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            VK_SHADER_STAGE_VERTEX_BIT,
	            0),
	        // Binding 1 : Color map
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	            VK_SHADER_STAGE_FRAGMENT_BIT,
	            1),
	    };

	descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layouts.skysphere));
	pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layouts.skysphere, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.skysphere));
}

void TerrainTessellation::setup_descriptor_sets()
{
	VkDescriptorSetAllocateInfo       alloc_info;
	std::vector<VkWriteDescriptorSet> write_descriptor_sets;

	// Terrain
	alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layouts.terrain, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.terrain));

	VkDescriptorBufferInfo terrain_buffer_descriptor   = create_descriptor(*uniform_buffers.terrain_tessellation);
	VkDescriptorImageInfo  heightmap_image_descriptor  = create_descriptor(textures.heightmap);
	VkDescriptorImageInfo  terrainmap_image_descriptor = create_descriptor(textures.terrain_array);
	write_descriptor_sets =
	    {
	        // Binding 0 : Shared tessellation shader ubo
	        vkb::initializers::write_descriptor_set(
	            descriptor_sets.terrain,
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            0,
	            &terrain_buffer_descriptor),
	        // Binding 1 : Displacement map
	        vkb::initializers::write_descriptor_set(
	            descriptor_sets.terrain,
	            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	            1,
	            &heightmap_image_descriptor),
	        // Binding 2 : Color map (alpha channel)
	        vkb::initializers::write_descriptor_set(
	            descriptor_sets.terrain,
	            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	            2,
	            &terrainmap_image_descriptor),
	    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);

	// Skysphere
	alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layouts.skysphere, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.skysphere));

	VkDescriptorBufferInfo skysphere_buffer_descriptor = create_descriptor(*uniform_buffers.skysphere_vertex);
	VkDescriptorImageInfo  skysphere_image_descriptor  = create_descriptor(textures.skysphere);
	write_descriptor_sets =
	    {
	        // Binding 0 : Vertex shader ubo
	        vkb::initializers::write_descriptor_set(
	            descriptor_sets.skysphere,
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            0,
	            &skysphere_buffer_descriptor),
	        // Binding 1 : Fragment shader color map
	        vkb::initializers::write_descriptor_set(
	            descriptor_sets.skysphere,
	            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	            1,
	            &skysphere_image_descriptor),
	    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
}

void TerrainTessellation::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
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
	        VK_TRUE,
	        VK_TRUE,
	        VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_LINE_WIDTH};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	// We render the terrain as a grid of quad patches
	VkPipelineTessellationStateCreateInfo tessellation_state =
	    vkb::initializers::pipeline_tessellation_state_create_info(4);

	// Vertex bindings an attributes
	// Binding description
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(TerrainTessellation::Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
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

	std::array<VkPipelineShaderStageCreateInfo, 4> shader_stages;

	// Terrain tessellation pipeline
	shader_stages[0] = load_shader("terrain_tessellation/terrain.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("terrain_tessellation/terrain.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	shader_stages[2] = load_shader("terrain_tessellation/terrain.tesc", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
	shader_stages[3] = load_shader("terrain_tessellation/terrain.tese", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(pipeline_layouts.terrain, render_pass, 0);

	pipeline_create_info.pVertexInputState   = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.pTessellationState  = &tessellation_state;
	pipeline_create_info.stageCount          = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();
	pipeline_create_info.renderPass          = render_pass;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.terrain));

	// Terrain wireframe pipeline
	if (get_device().get_gpu().get_features().fillModeNonSolid)
	{
		rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;
		VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.wireframe));
	};

	// Skysphere pipeline

	// Stride from glTF model vertex layout
	vertex_input_bindings[0].stride = sizeof(::Vertex);

	rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
	// Revert to triangle list topology
	input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// Reset tessellation state
	pipeline_create_info.pTessellationState = nullptr;
	// Don't write to depth buffer
	depth_stencil_state.depthWriteEnable = VK_FALSE;
	pipeline_create_info.stageCount      = 2;
	pipeline_create_info.layout          = pipeline_layouts.skysphere;
	shader_stages[0]                     = load_shader("terrain_tessellation/skysphere.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]                     = load_shader("terrain_tessellation/skysphere.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.skysphere));
}

// Prepare and initialize uniform buffer containing shader uniforms
void TerrainTessellation::prepare_uniform_buffers()
{
	// Shared tessellation shader stages uniform buffer
	uniform_buffers.terrain_tessellation = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                                           sizeof(ubo_tess),
	                                                                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                                           VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Skysphere vertex shader uniform buffer
	uniform_buffers.skysphere_vertex = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                                       sizeof(ubo_vs),
	                                                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                                       VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void TerrainTessellation::update_uniform_buffers()
{
	// Tessellation

	ubo_tess.projection   = camera.matrices.perspective;
	ubo_tess.modelview    = camera.matrices.view * glm::mat4(1.0f);
	ubo_tess.light_pos.y  = -0.5f - ubo_tess.displacement_factor;        // todo: Not uesed yet
	ubo_tess.viewport_dim = glm::vec2(static_cast<float>(width), static_cast<float>(height));

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

void TerrainTessellation::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be sumitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
	{
		// Read query results for displaying in next frame
		get_query_results();
	}

	ApiVulkanSample::submit_frame();
}

bool TerrainTessellation::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	// Note: Using Revsered depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.type = vkb::CameraType::FirstPerson;
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 512.0f, 0.1f);
	camera.set_rotation(glm::vec3(-12.0f, 159.0f, 0.0f));
	camera.set_translation(glm::vec3(18.0f, 22.5f, 57.5f));
	camera.translation_speed = 7.5f;

	load_assets();
	generate_terrain();
	if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
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

void TerrainTessellation::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
}

void TerrainTessellation::view_changed()
{
	update_uniform_buffers();
}

void TerrainTessellation::on_update_ui_overlay(vkb::Drawer &drawer)
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
		if (get_device().get_gpu().get_features().fillModeNonSolid)
		{
			if (drawer.checkbox("Wireframe", &wireframe))
			{
				build_command_buffers();
			}
		}
	}
	if (get_device().get_gpu().get_features().pipelineStatisticsQuery)
	{
		if (drawer.header("Pipeline statistics"))
		{
			drawer.text("VS invocations: %d", pipeline_stats[0]);
			drawer.text("TE invocations: %d", pipeline_stats[1]);
		}
	}
}

std::unique_ptr<vkb::Application> create_terrain_tessellation()
{
	return std::make_unique<TerrainTessellation>();
}
