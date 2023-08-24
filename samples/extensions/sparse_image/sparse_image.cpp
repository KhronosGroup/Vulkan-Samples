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

#include "sparse_image.h"
#include <iomanip>

SparseImage::SparseImage()
{
	title = "Sparse Image";
	setup_camera();
}

SparseImage::~SparseImage()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), sample_pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), sample_pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
	}
}

void SparseImage::load_assets()
{
	virtual_texture.row_data_image = vkb::sg::Image::load("clay_terrain.jpg", "clay_terrain.jpg", vkb::sg::Image::ContentType::Color);

	VkExtent3D   tex_extent = virtual_texture.row_data_image->get_extent();
	VkDeviceSize image_size = tex_extent.width * tex_extent.height * 4U;

	virtual_texture.width     = tex_extent.width;
	virtual_texture.height    = tex_extent.height;
}

void SparseImage::create_sparse_bind_queue()
{
	sparse_queue_family_index = get_device().get_queue_family_index(VK_QUEUE_SPARSE_BINDING_BIT);
	vkGetDeviceQueue(get_device().get_handle(), sparse_queue_family_index, 0U, &sparse_queue);
}

bool SparseImage::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	load_assets();

	create_descriptor_set_layout();

	create_vertex_buffer();
	create_index_buffer();
	create_uniform_buffer();

	create_sparse_bind_queue();
	create_sparse_texture_image();
	create_texture_sampler();

	create_descriptor_pool();
	create_descriptor_sets();

	prepare_pipelines();
	build_command_buffers();

	// Update MVP + new mip block calculations
	struct MVP mvp_ubo = {};
	mvp_ubo.model      = glm::mat4(1.0f);
	mvp_ubo.view       = camera.matrices.view;
	mvp_ubo.proj       = camera.matrices.perspective;

	mvp_buffer->update(&mvp_ubo, sizeof(mvp_ubo));

	current_mvp_transform = mvp_ubo.proj * mvp_ubo.view * mvp_ubo.model;

	// Generate a mesh and calculate required mip level for each texture block
	calculate_mips_table(current_mvp_transform, on_screen_num_vertical_blocks, on_screen_num_horizontal_blocks, virtual_texture.new_mip_table);
	// Compare new on-screen-present mip block set with the previous one
	compare_mips_table();
	// Defining all the flags/lists for binding and transfer
	calculate_required_memory_layout();

	next_stage = SPARSE_IMAGE_BIND_PAGES_PRE_MIP_GEN_STAGE;

	prepared = true;
	return true;
}

void SparseImage::prepare_pipelines()
{
	// Create a blank pipeline layout.
	VkPipelineLayoutCreateInfo layout_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_info, nullptr, &sample_pipeline_layout));

	VkPipelineVertexInputStateCreateInfo vertex_input = vkb::initializers::pipeline_vertex_input_state_create_info();

	/* Binding description */
	std::array<VkVertexInputBindingDescription, 1> vertex_input_bindings = {vkb::initializers::vertex_input_binding_description(0, sizeof(SimpleVertex), VK_VERTEX_INPUT_RATE_VERTEX)};

	/* Attribute description */
	std::array<VkVertexInputAttributeDescription, 2> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SimpleVertex, norm)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(SimpleVertex, uv))};

	vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input.pVertexAttributeDescriptions    = vertex_input_attributes.data();
	vertex_input.vertexBindingDescriptionCount   = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input.pVertexBindingDescriptions      = vertex_input_bindings.data();


	// Specify we will use triangle lists to draw geometry.
	VkPipelineInputAssemblyStateCreateInfo input_assembly = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	// Specify rasterization state.
	VkPipelineRasterizationStateCreateInfo raster = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);

	// Our attachment will write to all color channels, but no blending is enabled.
	VkPipelineColorBlendAttachmentState blend_attachment = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo blend = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment);

	// We will have one viewport and scissor box.
	VkPipelineViewportStateCreateInfo viewport = vkb::initializers::pipeline_viewport_state_create_info(1, 1);

	// Enable depth testing (using reversed depth-buffer for increased precision).
	VkPipelineDepthStencilStateCreateInfo depth_stencil = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER);

	// No multisampling.
	VkPipelineMultisampleStateCreateInfo multisample = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	std::array<VkDynamicState, 2>    dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic = vkb::initializers::pipeline_dynamic_state_create_info(dynamics.data(), vkb::to_u32(dynamics.size()));

	// Load our SPIR-V shaders.
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

	// Vertex stage of the pipeline
	shader_stages[0] = load_shader("sparse_image/sparse.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("sparse_image/sparse.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// We need to specify the pipeline layout and the render pass description up front as well.
	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(sample_pipeline_layout, render_pass);
	pipeline_create_info.stageCount                   = vkb::to_u32(shader_stages.size());
	pipeline_create_info.pStages                      = shader_stages.data();
	pipeline_create_info.pVertexInputState            = &vertex_input;
	pipeline_create_info.pInputAssemblyState          = &input_assembly;
	pipeline_create_info.pRasterizationState          = &raster;
	pipeline_create_info.pColorBlendState             = &blend;
	pipeline_create_info.pMultisampleState            = &multisample;
	pipeline_create_info.pViewportState               = &viewport;
	pipeline_create_info.pDepthStencilState           = &depth_stencil;
	pipeline_create_info.pDynamicState                = &dynamic;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &sample_pipeline));
}

void SparseImage::setup_camera()
{
	camera.type = vkb::CameraType::FirstPerson;
	camera.set_perspective(fov_degrees, static_cast<float>(width) / static_cast<float>(height), 0.1f, 512.0f);
	camera.set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));
	camera.set_translation(glm::vec3(0.0f, 0.0f, -50.0f));
	camera.translation_speed = 1.0f;
}

void SparseImage::transition_image_layout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, uint8_t mip_level)
{
	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkImageMemoryBarrier barrier{};
	barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout                       = old_layout;
	barrier.newLayout                       = new_layout;
	barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.image                           = image;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel   = mip_level;
	barrier.subresourceRange.levelCount     = 1U;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage     = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage     = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		source_stage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage     = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else
	{
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	device->flush_command_buffer(command_buffer, queue, true);
}


void SparseImage::separate_single_row_data_block(uint8_t buffer[], const VkExtent2D blockDim, VkOffset2D offset, size_t stride)
{
	for (size_t row = 0U; row < blockDim.height; row++)
	{
		for (size_t column = 0U; column < blockDim.width; column++)
		{
			size_t position                                   = (row + offset.y) * stride + (column + offset.x) * 4;
			buffer[row * blockDim.width * 4 + column * 4]     = virtual_texture.row_data_image->get_data()[position];
			buffer[row * blockDim.width * 4 + column * 4 + 1] = virtual_texture.row_data_image->get_data()[position + 1];
			buffer[row * blockDim.width * 4 + column * 4 + 2] = virtual_texture.row_data_image->get_data()[position + 2];
			buffer[row * blockDim.width * 4 + column * 4 + 3] = virtual_texture.row_data_image->get_data()[position + 3];
		}
	}
}


void SparseImage::bind_sparse_image()
{
	virtual_texture.bind_list.sort();

	for (size_t count = 0U; count < virtual_texture.num_pages; count++)
	{
		if (virtual_texture.bind_list.empty() == true)
		{
			virtual_texture.sparse_image_memory_bind[count].subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			virtual_texture.sparse_image_memory_bind[count].memory = VK_NULL_HANDLE;
			continue;
		}

		size_t page_index = virtual_texture.bind_list.front();

		if (count != page_index)
		{
			virtual_texture.sparse_image_memory_bind[count].subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			virtual_texture.sparse_image_memory_bind[count].memory = VK_NULL_HANDLE;
			continue;
		}

		if (virtual_texture.available_memory_index_list.empty() == true)
		{
			throw std::runtime_error("availableMemoryIndexList jest empty");
		}

		virtual_texture.bind_list.pop_front();

		size_t memory_block_index;
		if (virtual_texture.page_table[page_index].bound == true)
		{
			memory_block_index = virtual_texture.page_table[page_index].memory_index;
		}
		else
		{
			memory_block_index = virtual_texture.available_memory_index_list.front();
			virtual_texture.available_memory_index_list.pop_front();
		}

		size_t pageOffset = virtual_texture.page_size * memory_block_index;

		virtual_texture.page_table[page_index].memory_index          = memory_block_index;
		virtual_texture.page_table[page_index].bound                 = true;
		virtual_texture.sparse_image_memory_bind[count].memory       = virtual_texture.texture_memory;
		virtual_texture.sparse_image_memory_bind[count].memoryOffset = pageOffset;
	}

	VkBindSparseInfo bind_sparse_info = vkb::initializers::bind_sparse_info();
	bind_sparse_info.bufferBindCount  = 0U;
	bind_sparse_info.pBufferBinds     = nullptr;

	VkSparseImageMemoryBindInfo sparse_image_memory_bind_info{};
	sparse_image_memory_bind_info.image     = virtual_texture.texture_image;
	sparse_image_memory_bind_info.bindCount = virtual_texture.sparse_image_memory_bind.size();
	sparse_image_memory_bind_info.pBinds    = virtual_texture.sparse_image_memory_bind.data();

	bind_sparse_info.imageBindCount = 1U;
	bind_sparse_info.pImageBinds    = &sparse_image_memory_bind_info;

	VkSparseImageOpaqueMemoryBindInfo sparse_image_opaque_memory_bind_info{};
	sparse_image_opaque_memory_bind_info.image     = nullptr;
	sparse_image_opaque_memory_bind_info.bindCount = 0U;
	sparse_image_opaque_memory_bind_info.pBinds    = nullptr;

	bind_sparse_info.imageOpaqueBindCount = 0U;
	bind_sparse_info.pImageOpaqueBinds    = nullptr;

	bind_sparse_info.signalSemaphoreCount = 0U;
	bind_sparse_info.pSignalSemaphores    = nullptr;
	bind_sparse_info.waitSemaphoreCount   = 0U;
	bind_sparse_info.pWaitSemaphores      = nullptr;

	vkQueueBindSparse(sparse_queue, 1U, &bind_sparse_info, nullptr);
}


uint32_t SparseImage::get_mip_level(size_t page_index)
{
	uint32_t mip_level = 0;
	if (virtual_texture.mip_levels == 1U)
	{
		return virtual_texture.base_mip_level;
	}

	for (uint32_t i = virtual_texture.base_mip_level; i < virtual_texture.mip_levels; i++)
	{
		if (page_index < (virtual_texture.mip_properties[i].block_base_index + virtual_texture.mip_properties[i].num_blocks))
		{
			mip_level = i;
			break;
		}
	}
	return mip_level;
}

void SparseImage::calculate_required_memory_layout()
{
	while (virtual_texture.texture_block_update_list.empty() != true)
	{
		TextureBlock texture_block = virtual_texture.texture_block_update_list.front();
		virtual_texture.texture_block_update_list.pop_front();
		get_associated_memory_blocks(texture_block);
	}
}

void SparseImage::get_associated_memory_blocks(const TextureBlock &texture_block)
{
	std::list<size_t> memory_index_list;

	double new_integral_part, new_franctional_part, old_integral_part, old_fractional_part;
	new_franctional_part = modf(texture_block.new_mip_level, &new_integral_part);
	old_fractional_part  = modf(texture_block.old_mip_level, &old_integral_part);

	if (old_integral_part < virtual_texture.mip_levels)
	{
		// Old value calculations and removal from the render required list
		get_memory_dependency_for_the_block(texture_block.column, texture_block.row, static_cast<uint8_t>(old_integral_part), memory_index_list);

		while (memory_index_list.empty() != true)
		{
			size_t memory_index = memory_index_list.front();
			memory_index_list.pop_front();

			virtual_texture.page_table[memory_index].render_required_list.remove({texture_block.column, texture_block.row});
		}
	}

	if (texture_block.on_screen == false) { return; }

	// New value calculations and pushing into update required list
	get_memory_dependency_for_the_block(texture_block.column, texture_block.row, static_cast<uint8_t>(new_integral_part), memory_index_list);

	while (memory_index_list.empty() != true)
	{
		size_t memory_index = memory_index_list.front();
		memory_index_list.pop_front();

		virtual_texture.page_table[memory_index].render_required_list.remove({texture_block.column, texture_block.row});
		virtual_texture.page_table[memory_index].render_required_list.push_front({texture_block.column, texture_block.row});

		if (virtual_texture.page_table[memory_index].valid == false)
		{
			virtual_texture.update_list.remove(memory_index);
			virtual_texture.update_list.push_back(memory_index);
			virtual_texture.update_required = true;
		}


		std::list<MemPageDescription> mipgen_required_list;
		struct MemPageDescription     mem_page_description = get_mem_page_description(memory_index);

		mipgen_required_list.push_front(mem_page_description);
		//virtual_texture.page_table[memory_index].gen_mip_required = true;

		while (mipgen_required_list.empty() != true)
		{
			mem_page_description = mipgen_required_list.front();
			mipgen_required_list.pop_front();

			check_mip_page_requirements(mipgen_required_list, mem_page_description);
		}
	}
}

struct SparseImage::MemPageDescription SparseImage::get_mem_page_description(size_t memory_index)
{
	struct MemPageDescription mem_page_description = {};
	uint8_t                   mip_level            = virtual_texture.mip_levels - 1U;

	while (memory_index < virtual_texture.mip_properties[mip_level].block_base_index && mip_level > virtual_texture.base_mip_level)
	{
		mip_level--;
	}

	mem_page_description.mip_level = mip_level;
	mem_page_description.x        = (memory_index - virtual_texture.mip_properties[mip_level].block_base_index) % virtual_texture.mip_properties[mip_level].num_columns;
	mem_page_description.y        = (memory_index - virtual_texture.mip_properties[mip_level].block_base_index) / virtual_texture.mip_properties[mip_level].num_columns;

	return mem_page_description;
}


void SparseImage::check_mip_page_requirements(std::list<MemPageDescription> &mipgen_required_list, struct MemPageDescription mem_page_description)
{
	if (mem_page_description.mip_level == 0)
	{
		return;
	}

	struct MemPageDescription new_mem_page_description;
	new_mem_page_description.mip_level = mem_page_description.mip_level - 1;
	for (uint8_t y = 0; y < 2; y++)
	{
		for (uint8_t x = 0; x < 2; x++)
		{
			new_mem_page_description.x = (mem_page_description.x * 2) + x;
			new_mem_page_description.y = (mem_page_description.y * 2) + y;

			size_t memoryIndex = virtual_texture.mip_properties[new_mem_page_description.mip_level].block_base_index + virtual_texture.mip_properties[new_mem_page_description.mip_level].num_columns * new_mem_page_description.y + new_mem_page_description.x;

			virtual_texture.page_table[memoryIndex].gen_mip_required = true;
			mipgen_required_list.push_front(new_mem_page_description);

			if (virtual_texture.page_table[memoryIndex].valid == false)
			{
				virtual_texture.update_list.push_front(memoryIndex);
			}
		}
	}
}


// add memory indexes to the list
void SparseImage::get_memory_dependency_for_the_block(size_t column, size_t row, uint8_t mip_level, std::list<size_t> &index_list)
{
	double height_on_screen_divider = 1.0 / on_screen_num_vertical_blocks;
	double width_on_screen_divider  = 1.0 / on_screen_num_horizontal_blocks;

	double xLow  = width_on_screen_divider * column;
	double xHigh = width_on_screen_divider * (column + 1);

	double yLow  = height_on_screen_divider * row;
	double yHigh = height_on_screen_divider * (row + 1);

	double texel_width  = virtual_texture.mip_properties[mip_level].width;
	double texel_height = virtual_texture.mip_properties[mip_level].height;

	double in_memory_row_pages    = texel_height / virtual_texture.format_properties.imageGranularity.height;
	double in_memory_column_pages = texel_width / virtual_texture.format_properties.imageGranularity.width;

	double height_in_memory_divider = 1.0 / in_memory_row_pages;
	double width_in_memory_divider  = 1.0 / in_memory_column_pages;

	size_t memXLow  = static_cast<size_t>(floor(xLow / width_in_memory_divider));
	size_t memXHigh = static_cast<size_t>(ceil(xHigh / width_in_memory_divider));

	size_t memYLow  = static_cast<size_t>(floor(yLow / height_in_memory_divider));
	size_t memYHigh = static_cast<size_t>(ceil(yHigh / height_in_memory_divider));

	for (size_t y = memYLow; y < memYHigh; y++)
	{
		for (size_t x = memXLow; x < memXHigh; x++)
		{
			size_t memoryIndex = virtual_texture.mip_properties[mip_level].block_base_index + virtual_texture.mip_properties[mip_level].num_columns * y + x;
			index_list.push_front(memoryIndex);
		}
	}
}

void SparseImage::compare_mips_table()
{
	if (virtual_texture.texture_block_update_list.empty() != true)
	{
		//throw std::runtime_error("BlockUpdateList not empty!");
	}
	for (size_t y = 0U; y < virtual_texture.current_mip_table.size(); y++)
	{
		for (size_t x = 0U; x < virtual_texture.current_mip_table[0].size(); x++)
		{
			double new_integral_part, new_franctional_part, old_integral_part, old_fractional_part;
			new_franctional_part = modf(virtual_texture.new_mip_table[y][x].mip_level, &new_integral_part);
			old_fractional_part  = modf(virtual_texture.current_mip_table[y][x].mip_level, &old_integral_part);

			if (virtual_texture.new_mip_table[y][x].on_screen == false && virtual_texture.current_mip_table[y][x].on_screen == true)
			{
				TextureBlock texture_block = {y, x, virtual_texture.current_mip_table[y][x].mip_level, virtual_texture.new_mip_table[y][x].mip_level, false};
				virtual_texture.texture_block_update_list.push_back(texture_block);
				virtual_texture.current_mip_table[y][x] = virtual_texture.new_mip_table[y][x];
			}
			else if ((new_integral_part != old_integral_part) && (virtual_texture.new_mip_table[y][x].on_screen == true))
			{
				TextureBlock texture_block = {y, x, virtual_texture.current_mip_table[y][x].mip_level, virtual_texture.new_mip_table[y][x].mip_level, true};
				virtual_texture.texture_block_update_list.push_back(texture_block);
				virtual_texture.current_mip_table[y][x] = virtual_texture.new_mip_table[y][x];
			}
			else if (virtual_texture.new_mip_table[y][x].on_screen == true && virtual_texture.current_mip_table[y][x].on_screen == false)
			{
				TextureBlock texture_block = {y, x, virtual_texture.current_mip_table[y][x].mip_level, virtual_texture.new_mip_table[y][x].mip_level, true};
				virtual_texture.texture_block_update_list.push_back(texture_block);
				virtual_texture.current_mip_table[y][x] = virtual_texture.new_mip_table[y][x];
			}
		}
	}
}


void SparseImage::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	// Clear color and depth values.
	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	// Begin the render pass.
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
		auto cmd = draw_cmd_buffers[i];

		// Begin command buffer.
		vkBeginCommandBuffer(cmd, &command_buffer_begin_info);

		// Set framebuffer for this command buffer.
		render_pass_begin_info.framebuffer = framebuffers[i];
		// We will add draw commands in the same command buffer.
		vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		// Bind the graphics pipeline.
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sample_pipeline);

		// Set viewport dynamically
		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		// Set scissor dynamically
		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, sample_pipeline_layout, 0, 1, &descriptor_set, 0, NULL);

		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, &vertex_buffer->get_handle(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(draw_cmd_buffers[i], index_count, 1, 0, 0, 0);

		// Draw user interface.
		draw_ui(draw_cmd_buffers[i]);

		// Complete render pass.
		vkCmdEndRenderPass(cmd);

		// Complete the command buffer.
		VK_CHECK(vkEndCommandBuffer(cmd));
	}
}


void SparseImage::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	if (camera.updated)
	{
		// Update MVP + new mip block calculations
		struct MVP mvp_ubo = {};
		mvp_ubo.model      = glm::mat4(1.0f);
		mvp_ubo.view       = camera.matrices.view;
		mvp_ubo.proj       = camera.matrices.perspective;

		mvp_buffer->update(&mvp_ubo, sizeof(mvp_ubo));

		current_mvp_transform = mvp_ubo.proj * mvp_ubo.view * mvp_ubo.model;
	}

	switch (next_stage)
	{
		case SPARSE_IMAGE_CALCULATE_MESH_STAGE:
			// Generate a mesh and calculate required mip level for each texture block
			calculate_mips_table(current_mvp_transform, on_screen_num_vertical_blocks, on_screen_num_horizontal_blocks, virtual_texture.new_mip_table);

			next_stage = SPARSE_IMAGE_REQUIRED_MEMORY_LAYOUT_STAGE;
			break;

		case SPARSE_IMAGE_REQUIRED_MEMORY_LAYOUT_STAGE:
			// Compare new on-screen-present mip block set with the previous one
			compare_mips_table();
			// Defining all the flags/lists for binding and transfer
			calculate_required_memory_layout();

			if (virtual_texture.update_required == true)
			{
				next_stage = SPARSE_IMAGE_BIND_PAGES_PRE_MIP_GEN_STAGE;
			}
			else
			{
				next_stage = SPARSE_IMAGE_CALCULATE_MESH_STAGE;
			}
			break;

		case SPARSE_IMAGE_BIND_PAGES_PRE_MIP_GEN_STAGE:
			for (size_t i = 0U; i < virtual_texture.num_pages; i++)
			{
				if (virtual_texture.page_table[i].render_required_list.empty() == false || virtual_texture.page_table[i].gen_mip_required == true)
				{
					virtual_texture.bind_list.push_front(i);
				}
			}

			bind_sparse_image();
			next_stage = SPARSE_IMAGE_UPDATE_LEVEL0_STAGE;

			virtual_texture.update_list.sort();

			while (virtual_texture.update_list.empty() == false)
			{
				uint8_t current_mip_level = 0xFF;

				size_t memory_index = virtual_texture.update_list.front();
				virtual_texture.update_list.pop_front();

				uint8_t mip_level = get_mip_level(memory_index);

				if (current_mip_level != mip_level)
				{
					if (mip_level == 0U)
					{
						transition_image_layout(virtual_texture.texture_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_level);
					}
					else
					{
						transition_image_layout(virtual_texture.texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mip_level - 1U);
						transition_image_layout(virtual_texture.texture_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_level);
					}
					current_mip_level = mip_level;
				}

				if ((virtual_texture.page_table[memory_index].gen_mip_required == true || virtual_texture.page_table[memory_index].render_required_list.empty() == false) && virtual_texture.page_table[memory_index].bound == true && virtual_texture.page_table[memory_index].valid == false)
				{
					VkExtent2D block_extent{};
					block_extent.height = virtual_texture.sparse_image_memory_bind[memory_index].extent.height;
					block_extent.width  = virtual_texture.sparse_image_memory_bind[memory_index].extent.width;

					VkOffset2D block_offset{};
					block_offset.x = virtual_texture.sparse_image_memory_bind[memory_index].offset.x;
					block_offset.y = virtual_texture.sparse_image_memory_bind[memory_index].offset.y;

					if (mip_level == 0U)
					{
						std::vector<uint8_t> temp_buffer;
						temp_buffer.resize(virtual_texture.page_size);

						separate_single_row_data_block(temp_buffer.data(), block_extent, block_offset, virtual_texture.width * 4U);

						virtual_texture.single_page_buffer->update(temp_buffer, 0U);

						VkMappedMemoryRange mapped_range{};
						mapped_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
						mapped_range.memory = virtual_texture.single_page_buffer->get_memory();
						mapped_range.offset = 0;
						mapped_range.size   = virtual_texture.page_size;
						vkFlushMappedMemoryRanges(device->get_handle(), 1, &mapped_range);

						VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

						VkBufferImageCopy region{};
						region.bufferOffset      = 0;
						region.bufferRowLength   = 0;
						region.bufferImageHeight = 0;

						region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
						region.imageSubresource.mipLevel       = 0;
						region.imageSubresource.baseArrayLayer = 0;
						region.imageSubresource.layerCount     = 1;

						region.imageOffset = VkOffset3D({block_offset.x, block_offset.y, 0});
						region.imageExtent = VkExtent3D({block_extent.width, block_extent.height, 1});

						vkCmdCopyBufferToImage(command_buffer, virtual_texture.single_page_buffer->get_handle(), virtual_texture.texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

						device->flush_command_buffer(command_buffer, queue, true);

						virtual_texture.page_table[memory_index].valid = true;
					}
					else
					{
						VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

						VkImageBlit blit_cmd{};
						blit_cmd.srcOffsets[0]                 = {(block_offset.x) * 2, (block_offset.y) * 2, 0};
						blit_cmd.srcOffsets[1]                 = {(block_offset.x + static_cast<int>(block_extent.width)) * 2, (block_offset.y + static_cast<int>(block_extent.height)) * 2, 1};        // TODO 1x1 mip
						blit_cmd.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
						blit_cmd.srcSubresource.mipLevel       = mip_level - 1;
						blit_cmd.srcSubresource.baseArrayLayer = 0;
						blit_cmd.srcSubresource.layerCount     = 1;
						blit_cmd.dstOffsets[0]                 = {block_offset.x, block_offset.y, 0};
						blit_cmd.dstOffsets[1]                 = {block_offset.x + static_cast<int>(block_extent.width), block_offset.y + static_cast<int>(block_extent.height), 1};        // TODO 1x1 mip
						blit_cmd.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
						blit_cmd.dstSubresource.mipLevel       = mip_level;
						blit_cmd.dstSubresource.baseArrayLayer = 0;
						blit_cmd.dstSubresource.layerCount     = 1;

						vkCmdBlitImage(command_buffer,
						               virtual_texture.texture_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						               virtual_texture.texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						               1, &blit_cmd,
						               VK_FILTER_LINEAR);

						device->flush_command_buffer(command_buffer, queue, true);
					}
				}
			}
			virtual_texture.update_required = false;

			next_stage                      = SPARSE_IMAGE_FREE_MEMORY_STAGE;
			break;

		case SPARSE_IMAGE_FREE_MEMORY_STAGE:

			for (size_t i = 0U; i < virtual_texture.num_pages; i++)
			{
				if (virtual_texture.page_table[i].render_required_list.empty() == false)
				{
					virtual_texture.bind_list.push_front(i);
				}
			}

			bind_sparse_image();

			for (size_t i = 0U; i < virtual_texture.num_pages; i++)
			{
				virtual_texture.page_table[i].gen_mip_required = false;

				if (virtual_texture.page_table[i].bound == true)
				{
					if (virtual_texture.page_table[i].render_required_list.empty() == true)
					{
						virtual_texture.free_list.push_front(i);
					}
				}
			}

			while (virtual_texture.free_list.empty() != true)
			{
				size_t index = virtual_texture.free_list.front();
				virtual_texture.free_list.pop_front();

				virtual_texture.page_table[index].bound = false;
				virtual_texture.page_table[index].valid = false;
				virtual_texture.available_memory_index_list.push_back(virtual_texture.page_table[index].memory_index);
			}

			next_stage = SPARSE_IMAGE_CALCULATE_MESH_STAGE;
			break;
		
		default:
			break;
	}

	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

std::unique_ptr<vkb::VulkanSample> create_sparse_image()
{
	return std::make_unique<SparseImage>();
}


void SparseImage::calculate_mips_table(glm::mat4 mvp_transform, uint32_t numVerticalBlocks, uint32_t numHorizontalBlocks, std::vector<std::vector<MipBlock>> &mipTable)
{
	struct CalculateMipLevelData meshData(mvp_transform, VkExtent2D({static_cast<uint32_t>(virtual_texture.width), static_cast<uint32_t>(virtual_texture.height)}), VkExtent2D({static_cast<uint32_t>(width), static_cast<uint32_t>(height)}) , on_screen_num_vertical_blocks, on_screen_num_horizontal_blocks, virtual_texture.mip_levels);
	
	meshData.calculate_mesh_coordinates();
	meshData.calculate_mip_levels();

	mipTable = meshData.mip_table;
}

SparseImage::CalculateMipLevelData::CalculateMipLevelData(glm::mat4 mvp_transform, VkExtent2D texture_base_dim, VkExtent2D screen_base_dim, uint32_t vertical_num_blocks, uint32_t horizontal_num_blocks, uint8_t mip_levels) :
    mvp_transform(mvp_transform), texture_base_dim(texture_base_dim), screen_base_dim(screen_base_dim), mesh{0}, vertical_num_blocks(vertical_num_blocks), horizontal_num_blocks(horizontal_num_blocks), mip_levels(mip_levels)
{
	mesh.resize(vertical_num_blocks + 1U);
	for (auto& row : mesh)
	{
		row.resize(horizontal_num_blocks + 1U);
	}

	ax_vertical.resize(horizontal_num_blocks + 1U);
	ax_horizontal.resize(vertical_num_blocks + 1U);
}


void SparseImage::CalculateMipLevelData::calculate_mesh_coordinates()
{
	// Assuming it is a rectangle
	glm::vec4 top_left(-100.0f, -100.0f, 0.0f, 1.0f);
	glm::vec4 top_right(100.0f, -100.0f, 0.0f, 1.0f);
	glm::vec4 bottom_left(-100.0f, 100.0f, 0.0f, 1.0f);
	glm::vec4 bottom_right(100.0f, 100.0f, 0.0f, 1.0f);

	float h_interval = (top_right[0] - top_left[0]) / horizontal_num_blocks;
	float v_interval = (bottom_left[1] - top_left[1]) / vertical_num_blocks;

	//std::ofstream meshTableFile;
	//meshTableFile.open("C:\\MAIN\\projects\\meshTable.txt", std::ios::out || std::ios::trunc);


	for (size_t v_index = 0U; v_index < vertical_num_blocks + 1U; v_index++)
	{
		for (size_t h_index = 0U; h_index < horizontal_num_blocks + 1U; h_index++)
		{
			float x_norm = top_left[0] + h_index * h_interval;
			float y_norm = top_left[1] + v_index * v_interval;

			glm::vec4 result = mvp_transform * glm::vec4(x_norm, y_norm, 0.0f, 1.0f);

			double x = 0.5 * screen_base_dim.width * result[0] / abs(result[3]);
			double y = 0.5 * screen_base_dim.height * result[1] / abs(result[3]);

			mesh[v_index][h_index].x = x;
			mesh[v_index][h_index].y = y;

			mesh[v_index][h_index].on_screen = (x < (screen_base_dim.width / (-2.0))) || (x > (screen_base_dim.width / 2.0)) || (y < (screen_base_dim.height / (-2.0))) || (y > (screen_base_dim.height / 2.0)) || (result[3] < 0.0) ? false : true;
	//		meshTableFile << std::setw(4) << floor(mesh[v_index][h_index].on_screen) << '\t';
		}
	//	meshTableFile << std::endl;
	}
	//meshTableFile.close();

	for (size_t v_index = 0U; v_index < ax_horizontal.size(); v_index++)
	{
		if (abs(mesh[v_index][0].x - mesh[v_index][1].x) < 0.01)
		{
			ax_horizontal[v_index] = 1000.0;
		}
		else
		{
			ax_horizontal[v_index] = (mesh[v_index][0].y - mesh[v_index][1].y) / (mesh[v_index][0].x - mesh[v_index][1].x);
		}
	}

	for (size_t h_index = 0U; h_index < ax_vertical.size(); h_index++)
	{
		if (abs(mesh[0][h_index].x - mesh[1][h_index].x) < 0.01)
		{
			ax_vertical[h_index] = 1000.0;
		}
		else
		{
			ax_vertical[h_index] = (mesh[0][h_index].y - mesh[1][h_index].y) / (mesh[0][h_index].x - mesh[1][h_index].x);
		}
	}
}


/*!
 *  Function:   CalculateMipLevelData::calculate_mip_levels()
 *  Desc:       This is the very core function for calculating what level of detail is required for a particular block.
 *              Number of vertical and horizontal blocks is described by the global constants ON_SCREEN_VERTICAL_BLOCKS and ON_SCREEN_HORIZONTAL_BLOCKS.
 *              These constants are completely arbitrary - the more blocks, the better precision, the greater calculation overhead.
 *
 *              What this function does, is based on the mesh data created in calculateMeshCoordinates(), for each node within a mesh it calculates:
 *              "What is the ratio between x/y movement on the screen to the u/v movement on the texture?".
 *              The idea is, that if we move pixel-by-pixel along the x or y axis on screen, if the small, on-screen step causes a significant step on-texture, then the high level of detail is required.
 *              The formula used for those calculations is the very same as the one used the shader side:
 *                  LOD = log2 (max(dT / dx, dT / dy)); where:
 *                      - dT is an on-texture step in texels,
 *                      - dx, dy are on-screen steps in pixels.
 *
 *              One thing that makes these calculations complicated is that with the data provided by the mesh we move from one node to the other. But those steps (either horizontal or vertical) does not neccesarily
 *              go along the x and y axis. Because of that each vertical and horizontal step needs to be digested into x and y movement. Given that fact, for each rectangular...ish block that holds information on LOD required,
 *              there need to be 4 movements calculated and compared with their counterparts on the texture side.
 *
 *              Naming convention explained and method:
 *              - first mentioning of either "..Vertical.." or "..Horizontal.." in the variable name means that this variable is used in calculations related to moving one node down (vertical) or right (horizontal) from the current position.
 *                Calculations are handled from the top-left corner of the texture, so we are moving aither bottom or right (on the texture, not neccessarily on the screen).
 *              - Ph stands for "Point h". It is a point from which the vertical or horizontal step is splitted into x and y step.
 *              - A is a vertice we start calculations from. From A we move to either bottom node (which is B) or to the right node (which is C).
 *
 *              - IMPORTANT:    I assume that:
 *                                  - each block is a parallelogram which is obviously not 1:1 true, but the more precise we get (the more blocks we split the texture to) the more accurate this statement is.
 *                                  - the image is not "stretched'  within a single block, which has the same rules as stated above.
 *                              With those assumption, I'm providing a parralel lines from the Ph point to the corresponding edges. This creates an another parrallelogram.
 *
 *                              Variables named: ..verticalVertical.. or ..verticalHorizontalTop.. should be understood as: This relates to the vertical step (from A --> B) and
 *                                               describes the edge from Ph to the vertical edge / describes the edge from the Ph to the horizontal top edge.
 *
 *                              Assuming that the image is not stretched within a single block, I calculate the ratio of for example (verticalVertical / AtoB) or (verticalHorizontalTop / AtoC).
 *                              I know, that each parrallelogram on-screen corresponds to the fixed-size rectangular on-texture. Given the ratio I calculated I can just get calculate the on-texture step in texels from the right-triangle property
 *                              and compare it to the x or y step of vertical/horizontal step in pixels on-screen.
 *
 */
void SparseImage::CalculateMipLevelData::calculate_mip_levels()
{
	//std::ofstream mipTableFile;
	//mipTableFile.open("C:\\MAIN\\projects\\mipTable.txt", std::ios::out || std::ios::trunc);

	size_t num_rows    = mesh.size() - 1;
	size_t num_columns = mesh[0].size() - 1;

	mip_table.resize(num_rows);
	for (auto &row : mip_table)
	{
		row.resize(num_columns);
	}

	for (size_t row = 0U; row < num_rows; row += 1U)
	{
		for (size_t column = 0U; column < num_columns; column += 1U)
		{
			double dTx = texture_base_dim.width / (num_columns);
			double dTy = texture_base_dim.height / (num_rows);

			double dIx_vertical = mesh[row][column].x - mesh[row + 1][column].x;
			double dIy_vertical = mesh[row][column].y - mesh[row + 1][column].y;

			double dIx_horizontal = mesh[row][column].x - mesh[row][column + 1].x;
			double dIy_horizontal = mesh[row][column].y - mesh[row][column + 1].y;

			double AB_vertical   = sqrt(pow(dIx_vertical, 2) + pow(dIy_vertical, 2));
			double AC_horizontal = sqrt(pow(dIx_horizontal, 2) + pow(dIy_horizontal, 2));

			double pH_vertical_x, pH_vertical_y, pH_horizontal_x, pH_horizontal_y;

			pH_vertical_x   = mesh[row][column].x;
			pH_vertical_y   = mesh[row + 1][column].y;
			pH_horizontal_x = mesh[row][column + 1].x;
			pH_horizontal_y = mesh[row][column].y;

			double pH_vertical_to_A   = sqrt(pow(mesh[row][column].x - pH_vertical_x, 2) + pow(mesh[row][column].y - pH_vertical_y, 2));
			double pH_vertical_to_B   = sqrt(pow(mesh[row + 1][column].x - pH_vertical_x, 2) + pow(mesh[row + 1][column].y - pH_vertical_y, 2));
			double pH_horizontal_to_A = sqrt(pow(mesh[row][column].x - pH_horizontal_x, 2) + pow(mesh[row][column].y - pH_horizontal_y, 2));
			double pH_horizontal_to_C = sqrt(pow(mesh[row][column + 1].x - pH_horizontal_x, 2) + pow(mesh[row][column + 1].y - pH_horizontal_y, 2));

			double a_vertical   = ax_vertical[column];
			double a_horizontal = ax_horizontal[row];

			double x_vertical_vertical = (a_vertical * mesh[row][column].x + pH_vertical_y - (pH_vertical_x * a_horizontal) - mesh[row][column].y) / (a_vertical - a_horizontal);
			double y_vertical_vertical = (x_vertical_vertical - mesh[row][column].x) * a_vertical + mesh[row][column].y;

			double x_vertical_horizontal_top    = (a_horizontal * mesh[row][column].x + pH_vertical_y - (pH_vertical_x * a_vertical) - mesh[row][column].y) / (a_horizontal - a_vertical);
			double y_vertical_horizontal_top    = (x_vertical_horizontal_top - mesh[row][column].x) * a_horizontal + mesh[row][column].y;
			double x_vertical_horizontal_bottom = (a_horizontal * mesh[row + 1][column].x + pH_vertical_y - (pH_vertical_x * a_vertical) - mesh[row + 1][column].y) / (a_horizontal - a_vertical);
			double y_vertical_horizontal_bottom = (x_vertical_horizontal_bottom - mesh[row + 1][column].x) * a_horizontal + mesh[row + 1][column].y;

			double x_horizontal_horizontal = (a_horizontal * mesh[row][column].x + pH_horizontal_y - (pH_horizontal_x * a_vertical) - mesh[row][column].y) / (a_horizontal - a_vertical);
			double y_horizontal_horizontal = (x_horizontal_horizontal - mesh[row][column].x) * a_horizontal + mesh[row][column].y;

			double x_horizontal_vertical_left  = (a_vertical * mesh[row][column].x + pH_horizontal_y - (pH_horizontal_x * a_horizontal) - mesh[row][column].y) / (a_vertical - a_horizontal);
			double y_horizontal_vertical_left  = (x_horizontal_vertical_left - mesh[row][column].x) * a_vertical + mesh[row][column].y;
			double x_horizontal_vertical_right = (a_vertical * mesh[row][column + 1].x + pH_horizontal_y - (pH_horizontal_x * a_horizontal) - mesh[row][column + 1].y) / (a_vertical - a_horizontal);
			double y_horizontal_vertical_right = (x_horizontal_vertical_right - mesh[row][column + 1].x) * a_vertical + mesh[row][column + 1].y;

			double on_screen_pH_vertical_vertical          = sqrt(pow(pH_vertical_x - x_vertical_vertical, 2) + pow(pH_vertical_y - y_vertical_vertical, 2));
			double on_screen_pH_vertical_horizontal_top    = sqrt(pow(pH_vertical_x - x_vertical_horizontal_top, 2) + pow(pH_vertical_y - y_vertical_horizontal_top, 2));
			double on_screen_pH_vertical_horizontal_bottom = sqrt(pow(pH_vertical_x - x_vertical_horizontal_bottom, 2) + pow(pH_vertical_y - y_vertical_horizontal_bottom, 2));
			double on_screen_pH_horizontal_horizontal      = sqrt(pow(pH_horizontal_x - x_horizontal_horizontal, 2) + pow(pH_horizontal_y - y_horizontal_horizontal, 2));
			double on_screen_pH_horizontal_vertical_left   = sqrt(pow(pH_horizontal_x - x_horizontal_vertical_left, 2) + pow(pH_horizontal_y - y_horizontal_vertical_left, 2));
			double on_screen_pH_horizontal_vertical_right  = sqrt(pow(pH_horizontal_x - x_horizontal_vertical_right, 2) + pow(pH_horizontal_y - y_horizontal_vertical_right, 2));

			double on_texture_pH_vertical_vertical          = on_screen_pH_vertical_vertical / AC_horizontal * dTx;
			double on_texture_pH_vertical_horizontal_top    = on_screen_pH_vertical_horizontal_top / AB_vertical * dTy;
			double on_texture_pH_vertical_horizontal_bottom = on_screen_pH_vertical_horizontal_bottom / AB_vertical * dTy;
			double on_texture_pH_horizontal_horizontal      = on_screen_pH_horizontal_horizontal / AB_vertical * dTy;
			double on_texture_pH_horizontal_vertical_left   = on_screen_pH_horizontal_vertical_left / AC_horizontal * dTx;
			double on_texture_pH_horizontal_vertical_right  = on_screen_pH_horizontal_vertical_right / AC_horizontal * dTx;

			double x_texture_to_screen_vertical_ratio   = abs(pH_vertical_to_A) < 1.0 ? 0.0 : sqrt(pow(on_texture_pH_vertical_vertical, 2) + pow(on_texture_pH_vertical_horizontal_top, 2)) / abs(pH_vertical_to_A);
			double y_texture_to_screen_vertical_ratio   = abs(pH_vertical_to_B) < 1.0 ? 0.0 : sqrt(pow(on_texture_pH_vertical_vertical, 2) + pow(on_texture_pH_vertical_horizontal_bottom, 2)) / abs(pH_vertical_to_B);
			double x_texture_to_screen_horizontal_ratio = abs(pH_horizontal_to_A) < 1.0 ? 0.0 : sqrt(pow(on_texture_pH_horizontal_horizontal, 2) + pow(on_texture_pH_horizontal_vertical_left, 2)) / abs(pH_horizontal_to_A);
			double y_texture_to_screen_horizontal_ratio = abs(pH_horizontal_to_C) < 1.0 ? 0.0 : sqrt(pow(on_texture_pH_horizontal_horizontal, 2) + pow(on_texture_pH_horizontal_vertical_right, 2)) / abs(pH_horizontal_to_C);

			double delta     = 1.0 * std::max(std::max(x_texture_to_screen_horizontal_ratio, y_texture_to_screen_horizontal_ratio), std::max(x_texture_to_screen_vertical_ratio, y_texture_to_screen_vertical_ratio));
			double mip_level = std::min(static_cast<double>(mip_levels - 1U), std::max(log2(delta), 0.0));

			mip_table[row][column].mip_level = static_cast<double>(mip_level);
			mip_table[row][column].on_screen = mesh[row][column].on_screen && mesh[row + 1][column].on_screen && mesh[row][column + 1].on_screen;
	//		mipTableFile << std::left << std::setprecision(3) << std::setw(4) << std::setfill('0') << mip_level << "\t";
		}
	//	mipTableFile << std::endl;
	}
	//mipTableFile.close();
}


void SparseImage::create_vertex_buffer()
{
	std::array<struct SimpleVertex, 4> vertices;
	VkDeviceSize                       vertices_size = sizeof(vertices[0]) * vertices.size();

	vertices[0].norm = {-100.0f, -100.0f};
	vertices[1].norm = { 100.0f, -100.0f};
	vertices[2].norm = { 100.0f,  100.0f};
	vertices[3].norm = {-100.0f,  100.0f};

	vertices[0].uv   = {0.0f, 0.0f};
	vertices[1].uv   = {1.0f, 0.0f};
	vertices[2].uv   = {1.0f, 1.0f};
	vertices[3].uv   = {0.0f, 1.0f};

	std::unique_ptr<vkb::core::Buffer> staging_buffer;
	staging_buffer = std::make_unique<vkb::core::Buffer>(get_device(), vertices_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer  = std::make_unique<vkb::core::Buffer>(get_device(), vertices_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		
	staging_buffer->update(vertices.data(), vertices_size);

	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copy_buffer_info{};
	copy_buffer_info.srcOffset = 0;
	copy_buffer_info.dstOffset = 0;
	copy_buffer_info.size      = vertices_size;

	vkCmdCopyBuffer(command_buffer, staging_buffer->get_handle(), vertex_buffer->get_handle(), 1, &copy_buffer_info);
	device->flush_command_buffer(command_buffer, queue, true);
}

void SparseImage::create_index_buffer()
{
	std::array<uint16_t, 6> indices      = {0, 1, 2, 2, 3, 0};
	VkDeviceSize            indices_size = sizeof(indices[0]) * indices.size();
	index_count                          = indices.size();

	std::unique_ptr<vkb::core::Buffer> staging_buffer;
	staging_buffer = std::make_unique<vkb::core::Buffer>(get_device(), indices_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	index_buffer   = std::make_unique<vkb::core::Buffer>(get_device(), indices_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	staging_buffer->update(indices.data(), indices_size);

	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copy_buffer_info{};
	copy_buffer_info.srcOffset = 0;
	copy_buffer_info.dstOffset = 0;
	copy_buffer_info.size      = indices_size;

	vkCmdCopyBuffer(command_buffer, staging_buffer->get_handle(), index_buffer->get_handle(), 1, &copy_buffer_info);
	device->flush_command_buffer(command_buffer, queue, true);
}


/**
 * 	@fn void sparse_image::create_descriptor_pool()
 * 	@brief Creating descriptor pool with size adjusted to use uniform buffer and image sampler
 */
void SparseImage::create_descriptor_pool()
{
	std::array<VkDescriptorPoolSize, 2> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
	};

	VkDescriptorPoolCreateInfo pool_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        1);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &pool_info, VK_NULL_HANDLE, &descriptor_pool));
}

/**
 * 	@fn void sparse_image::setup_descriptor_set_layout()
 * 	@brief Creating layout for descriptor sets
 */
void SparseImage::create_descriptor_set_layout()
{
	std::array<VkDescriptorSetLayoutBinding, 2> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_VERTEX_BIT,
	        0),
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        VK_SHADER_STAGE_FRAGMENT_BIT,
	        1),
	};

	VkDescriptorSetLayoutCreateInfo set_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &set_layout_create_info, VK_NULL_HANDLE, &descriptor_set_layout));
}

/**
 * 	@fn void sparse_image::create_descriptor_sets()
 * 	@brief Creating both descriptor set:
 * 		   1. Uniform buffer
 * 		   2. Image sampler
 */
void SparseImage::create_descriptor_sets()
{
	VkDescriptorSetAllocateInfo set_alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &set_alloc_info, &descriptor_set));

	VkDescriptorBufferInfo descriptor_buffer_info = create_descriptor(*mvp_buffer);

	VkDescriptorImageInfo image_info{};
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.imageView   = virtual_texture.texture_image_view;
	image_info.sampler     = texture_sampler;

	std::array<VkWriteDescriptorSet, 2> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(
	        descriptor_set,
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        0U,
	        &descriptor_buffer_info,
	        1U),
	    vkb::initializers::write_descriptor_set(
	        descriptor_set,
	        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        1U,
	        &image_info,
	        1U)};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}


void SparseImage::create_uniform_buffer()
{
	VkDeviceSize buffer_size = sizeof(struct MVP);
	mvp_buffer               = std::make_unique<vkb::core::Buffer>(get_device(), buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

void SparseImage::create_texture_sampler()
{
	VkSamplerCreateInfo sampler_info = vkb::initializers::sampler_create_info();

	sampler_info.magFilter    = VK_FILTER_LINEAR;
	sampler_info.minFilter    = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(device->get_gpu().get_handle(), &properties);

	sampler_info.anisotropyEnable        = VK_TRUE;
	sampler_info.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
	sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable           = VK_FALSE;
	sampler_info.compareOp               = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_info.mipLodBias              = 0.0f;
	sampler_info.minLod                  = static_cast<float>(virtual_texture.base_mip_level);
	sampler_info.maxLod                  = static_cast<float>(virtual_texture.mip_levels - 1U);

	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_info, nullptr, &texture_sampler));
}


/**
 * @fn void sparse_image::request_gpu_features(vkb::PhysicalDevice &gpu)
 * @brief Enabling features
 */
void SparseImage::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
	// Enable anisotropic filtering if supported
	if (gpu.get_features().sparseBinding && gpu.get_features().sparseResidencyImage2D && gpu.get_features().shaderResourceResidency)
	{
		gpu.get_mutable_requested_features().sparseBinding           = VK_TRUE;
		gpu.get_mutable_requested_features().sparseResidencyImage2D  = VK_TRUE;
		gpu.get_mutable_requested_features().shaderResourceResidency = VK_TRUE;
	}
	else
	{
		throw std::runtime_error("Sparse binding not supported");
	}

}


void SparseImage::create_sparse_texture_image()
{
	// Creating an Image
	VkImageCreateInfo sparse_image_create_info = vkb::initializers::image_create_info();
	sparse_image_create_info.imageType = VK_IMAGE_TYPE_2D;

	sparse_image_create_info.extent.width  = static_cast<uint32_t>(virtual_texture.width);
	sparse_image_create_info.extent.height = static_cast<uint32_t>(virtual_texture.height);
	sparse_image_create_info.extent.depth  = 1;

	virtual_texture.base_mip_level       = 0U;
	virtual_texture.mip_levels           = 5U;

	sparse_image_create_info.mipLevels   = virtual_texture.mip_levels;
	sparse_image_create_info.arrayLayers = 1;

	sparse_image_create_info.flags         = VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;
	sparse_image_create_info.format        = VK_FORMAT_R8G8B8A8_SRGB;
	sparse_image_create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
	sparse_image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	sparse_image_create_info.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	sparse_image_create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
	sparse_image_create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateImage(get_device().get_handle(), &sparse_image_create_info, nullptr, &virtual_texture.texture_image));

	//==================================================================================================
	// Calculating memory dependencies and defining total number of pages and page size

	std::vector<VkSparseImageFormatProperties>   sparse_image_format_properties;
	std::vector<VkSparseImageMemoryRequirements> sparse_image_memory_requirements;
	VkMemoryRequirements                         mem_requirements;

	uint32_t property_count;
	uint32_t memory_req_count;

	vkGetPhysicalDeviceSparseImageFormatProperties(device->get_gpu().get_handle(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, &property_count, nullptr);
	sparse_image_format_properties.resize(property_count);
	vkGetPhysicalDeviceSparseImageFormatProperties(device->get_gpu().get_handle(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, &property_count, sparse_image_format_properties.data());

	vkGetImageSparseMemoryRequirements(get_device().get_handle(), virtual_texture.texture_image, &memory_req_count, nullptr);
	sparse_image_memory_requirements.resize(memory_req_count);
	vkGetImageSparseMemoryRequirements(get_device().get_handle(), virtual_texture.texture_image, &memory_req_count, sparse_image_memory_requirements.data());

	vkGetImageMemoryRequirements(get_device().get_handle(), virtual_texture.texture_image, &mem_requirements);

	virtual_texture.format_properties          = sparse_image_format_properties[0];
	virtual_texture.memory_sparse_requirements = sparse_image_memory_requirements[0];
	virtual_texture.mem_requirements           = mem_requirements;

	virtual_texture.page_size          = virtual_texture.format_properties.imageGranularity.height * virtual_texture.format_properties.imageGranularity.width * 4U;
	virtual_texture.single_page_buffer = std::make_unique<vkb::core::Buffer>(get_device(), virtual_texture.page_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);

	// calculate page size
	virtual_texture.page_size = sparse_image_format_properties[0].imageGranularity.depth * sparse_image_format_properties[0].imageGranularity.height * sparse_image_format_properties[0].imageGranularity.width * 4U;

	// calculate total number of pages
	size_t num_total_pages    = 0U;
	size_t current_mip_height = virtual_texture.height;
	size_t current_mip_width  = virtual_texture.width;

	virtual_texture.mip_properties.resize(virtual_texture.mip_levels);

	for (uint32_t mip_level = 0U; mip_level < virtual_texture.mip_levels; mip_level++)
	{
		size_t numRows    = current_mip_height / sparse_image_format_properties[0].imageGranularity.height;
		size_t numColumns = current_mip_width / sparse_image_format_properties[0].imageGranularity.width;

		if (current_mip_height % sparse_image_format_properties[0].imageGranularity.height)
		{
			numRows++;
		}
		if (current_mip_width % sparse_image_format_properties[0].imageGranularity.width)
		{
			numColumns++;
		}

		num_total_pages += numRows * numColumns;

		virtual_texture.mip_properties[mip_level].width            = current_mip_width;
		virtual_texture.mip_properties[mip_level].height           = current_mip_height;
		virtual_texture.mip_properties[mip_level].num_columns      = numColumns;
		virtual_texture.mip_properties[mip_level].num_rows         = numRows;
		virtual_texture.mip_properties[mip_level].num_blocks       = numRows * numColumns;
		virtual_texture.mip_properties[mip_level].block_base_index = mip_level > 0 ? virtual_texture.mip_properties[mip_level - 1].block_base_index + virtual_texture.mip_properties[mip_level - 1].num_blocks : 0U;

		if (current_mip_height > 1)
		{
			current_mip_height /= 2U;
		}
		if (current_mip_width > 1)
		{
			current_mip_width /= 2U;
		}
	}

	virtual_texture.width  = virtual_texture.mip_properties[0].width;
	virtual_texture.height = virtual_texture.mip_properties[0].height;

	virtual_texture.current_mip_table.resize(on_screen_num_vertical_blocks);
	virtual_texture.new_mip_table.resize(on_screen_num_vertical_blocks);


	for (size_t y = 0U; y < on_screen_num_vertical_blocks; y++)
	{
		virtual_texture.current_mip_table[y].resize(on_screen_num_horizontal_blocks);
		virtual_texture.new_mip_table[y].resize(on_screen_num_horizontal_blocks);

		for (size_t x = 0U; x < on_screen_num_horizontal_blocks; x++)
		{
			virtual_texture.current_mip_table[y][x].mip_level = 0xFF;
			virtual_texture.current_mip_table[y][x].on_screen = false;

			virtual_texture.new_mip_table[y][x].mip_level     = 0x00;
			virtual_texture.new_mip_table[y][x].on_screen = false;
		}
	}

	virtual_texture.num_pages = num_total_pages;
	virtual_texture.page_table.resize(num_total_pages);

	virtual_texture.sparse_image_memory_bind.resize(virtual_texture.num_pages);

	for (size_t page_index = 0U; page_index < virtual_texture.num_pages; page_index++)
	{
		uint32_t mipLevel = get_mip_level(page_index);

		virtual_texture.sparse_image_memory_bind[page_index].subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		virtual_texture.sparse_image_memory_bind[page_index].subresource.arrayLayer = 0U;
		virtual_texture.sparse_image_memory_bind[page_index].subresource.mipLevel   = mipLevel;
		virtual_texture.sparse_image_memory_bind[page_index].flags                  = 0U;

		virtual_texture.sparse_image_memory_bind[page_index].offset.x = ((page_index - virtual_texture.mip_properties[mipLevel].block_base_index) % virtual_texture.mip_properties[mipLevel].num_columns) * virtual_texture.format_properties.imageGranularity.width;
		virtual_texture.sparse_image_memory_bind[page_index].offset.y = ((page_index - virtual_texture.mip_properties[mipLevel].block_base_index) / virtual_texture.mip_properties[mipLevel].num_columns) * virtual_texture.format_properties.imageGranularity.height;
		virtual_texture.sparse_image_memory_bind[page_index].offset.z = 0;

		virtual_texture.sparse_image_memory_bind[page_index].extent.depth = virtual_texture.format_properties.imageGranularity.depth;
		virtual_texture.sparse_image_memory_bind[page_index].extent.width  = (virtual_texture.mip_properties[mipLevel].width - virtual_texture.sparse_image_memory_bind[page_index].offset.x < virtual_texture.format_properties.imageGranularity.width) ? virtual_texture.mip_properties[mipLevel].width - virtual_texture.sparse_image_memory_bind[page_index].offset.x : virtual_texture.format_properties.imageGranularity.width;
		virtual_texture.sparse_image_memory_bind[page_index].extent.height = (virtual_texture.mip_properties[mipLevel].height - virtual_texture.sparse_image_memory_bind[page_index].offset.y < virtual_texture.format_properties.imageGranularity.height) ? virtual_texture.mip_properties[mipLevel].height - virtual_texture.sparse_image_memory_bind[page_index].offset.y : virtual_texture.format_properties.imageGranularity.height;
	
	}

	VkMemoryAllocateInfo memory_allocate_info{};
	memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize  = virtual_texture.page_size * virtual_texture.mip_properties[0].num_blocks; //TODO
	memory_allocate_info.memoryTypeIndex = get_device().get_memory_type(virtual_texture.mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	for (size_t i = 0U; i < (virtual_texture.mip_properties[0].num_blocks); i++)
	{
		virtual_texture.available_memory_index_list.push_back(i);
	}

	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &virtual_texture.texture_memory));

	//==================================================================================================
	// Creating texture image view

	VkImageViewCreateInfo view_info = vkb::initializers::image_view_create_info();
	view_info.image                          = virtual_texture.texture_image;
	view_info.viewType                       = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format                         = VK_FORMAT_R8G8B8A8_SRGB;
	view_info.subresourceRange.aspectMask    = VK_IMAGE_ASPECT_COLOR_BIT;
	view_info.subresourceRange.baseMipLevel  = virtual_texture.base_mip_level;
	view_info.subresourceRange.levelCount    = virtual_texture.mip_levels;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount     = 1;

	VK_CHECK(vkCreateImageView(get_device().get_handle(), &view_info, nullptr, &virtual_texture.texture_image_view));
}

void SparseImage::view_changed()
{
	//process_camera_change();
}
