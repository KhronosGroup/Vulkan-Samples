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

#pragma once

#include "api_vulkan_sample.h"

size_t const SPARSE_IMAGE_ON_SCREEN_HORIZONTAL_BLOCKS = 50U;
size_t const SPARSE_IMAGE_ON_SCREEN_VERTICAL_BLOCKS   = 30U;
double const SPARSE_IMAGE_FOV_DEGREES                 = 60.0;

class SparseImage : public ApiVulkanSample
{
  public:
	enum Stages
	{
		SPARSE_IMAGE_IDLE_STAGE,
		SPARSE_IMAGE_CALCULATE_MIPS_TABLE_STAGE,
		SPARSE_IMAGE_COMPARE_MIPS_TABLE_STAGE,
		SPARSE_IMAGE_PROCESS_TEXTURE_BLOCKS_STAGE,
		SPARSE_IMAGE_UPDATE_AND_GENERATE_STAGE,
		SPARSE_IMAGE_FREE_UNUSED_MEMORY_STAGE,
		SPARSE_IMAGE_NUM_STAGES,
	};

	struct MVP
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	struct SimpleVertex
	{
		glm::vec2 norm;
		glm::vec2 uv;
	};

	struct MipProperties
	{
		size_t num_rows;
		size_t num_columns;
		size_t mip_num_pages;
		size_t mip_base_page_index;
		size_t width;
		size_t height;
	};

	struct TextureBlock
	{
		size_t  row;
		size_t  column;
		uint8_t old_mip_level;
		uint8_t new_mip_level;
		bool    on_screen;
	};

	struct MemPageDescription
	{
		size_t  x;
		size_t  y;
		uint8_t mip_level;
	};

	struct PageTable
	{
		bool   bound;                   // bound via vkQueueBindSparse()
		bool   valid;                   // bound and contains valid data
		bool   gen_mip_required;        // required for the mip generation
		bool   fixed;                   // not freed from the memory at any cases
		size_t memory_index;            // index from available_memory_index_list corresponding to this page

		std::list<std::tuple<uint8_t, size_t, size_t>> render_required_list;        // list holding information on what BLOCKS require this particular memory page to be valid for rendering
	};

	struct Point
	{
		double x;
		double y;
		bool   on_screen;
	};

	struct MipBlock
	{
		uint8_t mip_level;
		bool    on_screen;
	};

	struct VirtualTexture
	{
		VkImage        texture_image;
		VkImageView    texture_image_view;
		VkDeviceMemory texture_memory;

		// Dimensions
		size_t width;
		size_t height;

		// Number of virtual pages (what if the total image was allocated)
		size_t num_pages;
		size_t page_size;

		uint8_t                           base_mip_level;
		uint8_t                           mip_levels;
		std::vector<struct MipProperties> mip_properties;

		std::vector<std::vector<MipBlock>> current_mip_table;
		std::vector<std::vector<MipBlock>> new_mip_table;

		// Image containing a single, most detailed mip, allocated in the CPU memory, coppied to VRAM via staging buffer single_page_buffer
		std::unique_ptr<vkb::sg::Image>    row_data_image;
		std::unique_ptr<vkb::core::Buffer> single_page_buffer;

		// Key table that includes data on which page is allocated to what memory block from the textureMemory vector
		std::vector<struct PageTable> page_table;

		// List containing BLOCKS for which the required mip level has changed or/and its on-screen visibility changed
		std::list<struct TextureBlock> texture_block_update_list;

		// List containing information which pages from the page_table should be bound
		std::list<size_t> bind_list;

		// List containing information which pages from the page_table should be updated (either loaded from CPU memory or blitted)
		std::list<size_t> update_list;

		// List containing information which pages should be freed
		std::list<size_t> free_list;

		// List of available memory pages (whole memory page pool is statically allocated at the beginning)
		std::list<size_t> available_memory_index_list;

		bool update_required = false;

		// Sparse-image-related format and memory properties
		VkSparseImageFormatProperties   format_properties;
		VkSparseImageMemoryRequirements memory_sparse_requirements;
		VkMemoryRequirements            mem_requirements;

		std::vector<VkSparseImageMemoryBind> sparse_image_memory_bind;
	};

	struct CalculateMipLevelData
	{
		std::vector<std::vector<Point>>    mesh;
		std::vector<std::vector<MipBlock>> mip_table;

		uint32_t vertical_num_blocks;
		uint32_t horizontal_num_blocks;

		uint8_t mip_levels;

		std::vector<float> ax_vertical;
		std::vector<float> ax_horizontal;

		glm::mat4 mvp_transform;

		VkExtent2D texture_base_dim;
		VkExtent2D screen_base_dim;

		CalculateMipLevelData(glm::mat4 mvp_transform, VkExtent2D texture_base_dim, VkExtent2D screen_base_dim, uint32_t vertical_num_blocks, uint32_t horizontal_num_blocks, uint8_t mip_levels);
		void calculate_mesh_coordinates();
		void calculate_mip_levels();
	};

	bool        transfer_finished      = false;
	bool        init_transfer_finished = false;
	uint8_t     frame_counter          = 0U;
	enum Stages next_stage             = SPARSE_IMAGE_IDLE_STAGE;

	struct VirtualTexture virtual_texture;

	VkQueue sparse_queue;
	size_t  sparse_queue_family_index;

	std::unique_ptr<vkb::core::Buffer> vertex_buffer;

	std::unique_ptr<vkb::core::Buffer> index_buffer;
	size_t                             index_count;

	std::unique_ptr<vkb::core::Buffer> mvp_buffer;

	glm::mat4 current_mvp_transform;

	VkPipeline       sample_pipeline{};
	VkPipelineLayout sample_pipeline_layout{};

	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorSet       descriptor_set;
	VkSampler             texture_sampler;

	size_t on_screen_num_vertical_blocks   = SPARSE_IMAGE_ON_SCREEN_VERTICAL_BLOCKS;
	size_t on_screen_num_horizontal_blocks = SPARSE_IMAGE_ON_SCREEN_HORIZONTAL_BLOCKS;
	double fov_degrees                     = SPARSE_IMAGE_FOV_DEGREES;

	//==================================================================================================
	SparseImage();
	virtual ~SparseImage();

	void setup_camera();
	void load_assets();

	void prepare_pipelines();

	void create_sparse_bind_queue();

	void create_vertex_buffer();
	void create_index_buffer();

	void create_uniform_buffer();
	void create_texture_sampler();

	void create_descriptor_set_layout();
	void create_descriptor_pool();
	void create_descriptor_sets();

	void create_sparse_texture_image();

	void                      update_mvp();
	void                      process_stage(enum Stages next_stage);
	void                      free_unused_memory();
	void                      update_and_generate();
	void                      process_texture_blocks();
	struct MemPageDescription get_mem_page_description(size_t memory_index);
	void                      calculate_mips_table(glm::mat4 mvp_transform, uint32_t numVerticalBlocks, uint32_t numHorizontalBlocks, std::vector<std::vector<MipBlock>> &mipTable);
	void                      compare_mips_table();
	void                      process_texture_block(const TextureBlock &on_screen_block);
	void                      get_memory_dependency_for_the_block(size_t column, size_t row, uint8_t mip_level, std::list<size_t> &index_list);
	void                      check_mip_page_requirements(std::list<MemPageDescription> &mipgen_required_list, struct MemPageDescription mip_dependency);
	void                      bind_sparse_image();
	void                      separate_single_row_data_block(uint8_t buffer[], const VkExtent2D blockDim, VkOffset2D offset, size_t stride);
	void                      transition_mip_layout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, uint8_t mip_level);
	uint8_t                   get_mip_level(size_t page_index);
	size_t                    get_memory_index(struct MemPageDescription mem_page_desc);

	// Override basic framework functionalities
	void build_command_buffers() override;
	void render(float delta_time) override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
};

std::unique_ptr<vkb::VulkanSample> create_sparse_image();
