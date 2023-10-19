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

size_t const SPARSE_IMAGE_ON_SCREEN_NUM_HORIZONTAL_BLOCKS = 50U;
size_t const SPARSE_IMAGE_ON_SCREEN_NUM_VERTICAL_BLOCKS   = 30U;
double const SPARSE_IMAGE_FOV_DEGREES                     = 60.0;
size_t const SPARSE_IMAGE_NUM_PAGES_IN_SINGLE_ALLOC       = 50U;

class SparseImage : public ApiVulkanSample
{
  public:
	bool color_highlight        = true;
	bool color_highlight_mem    = true;
	bool memory_defragmentation = true;

	enum Stages
	{
		SPARSE_IMAGE_IDLE_STAGE,
		SPARSE_IMAGE_CALCULATE_MIPS_TABLE_STAGE,
		SPARSE_IMAGE_COMPARE_MIPS_TABLE_STAGE,
		SPARSE_IMAGE_FREE_MEMORY_STAGE,
		SPARSE_IMAGE_PROCESS_TEXTURE_BLOCKS_STAGE,
		SPARSE_IMAGE_UPDATE_AND_GENERATE_STAGE,
		SPARSE_IMAGE_NUM_STAGES,
	};

	struct MVP
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	struct FragSettingsData
	{
		bool color_highlight;
		int  minLOD;
		int  maxLOD;
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
		bool operator<(TextureBlock const &other) const
		{
			if (this->new_mip_level == other.new_mip_level)
			{
				if (this->column == other.column)
				{
					return this->row < other.row;
				}
				else
				{
					return this->column < other.column;
				}
			}
			return this->new_mip_level < other.new_mip_level;
		};

		size_t row;
		size_t column;
		double old_mip_level;
		double new_mip_level;
		bool   on_screen;
	};

	struct MemPageDescription
	{
		size_t  x;
		size_t  y;
		uint8_t mip_level;
	};

	struct Point
	{
		double x;
		double y;
		bool   on_screen;
	};

	struct MipBlock
	{
		double mip_level;
		bool   on_screen;
	};

	struct MemSector;

	struct PageInfo
	{
		std::shared_ptr<MemSector> memory_sector;
		uint32_t                   offset;
	};

	struct PageTable
	{
		bool     valid;                   // bound via vkQueueBindSparse() and contains valid data
		bool     gen_mip_required;        // required for the mip generation
		bool     fixed;                   // not freed from the memory at any cases
		PageInfo page_memory_info;        // memory-related info

		std::set<std::tuple<uint8_t, size_t, size_t>> render_required_set;        // set holding information on what BLOCKS require this particular memory page to be valid for rendering
	};

	struct MemAllocInfo
	{
		VkDevice device;
		uint64_t page_size;
		uint32_t memory_type_index;

		void get_allocation(PageInfo &page_memory_info, size_t page_index);

		uint32_t get_size()
		{
			return memory_sectors.size();
		}

		std::list<std::weak_ptr<MemSector>> &get_handle()
		{
			return memory_sectors;
		}

	  private:
		std::list<std::weak_ptr<MemSector>> memory_sectors;
	};

	struct MemSector : public MemAllocInfo
	{
		VkDeviceMemory memory = VK_NULL_HANDLE;

		std::set<uint32_t> available_offsets;
		std::set<size_t>   virt_page_indices;

		MemSector(MemAllocInfo &mem_alloc_info)
		{
			this->device            = mem_alloc_info.device;
			this->page_size         = mem_alloc_info.page_size;
			this->memory_type_index = mem_alloc_info.memory_type_index;

			VkMemoryAllocateInfo memory_allocate_info{};
			memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memory_allocate_info.allocationSize  = page_size * SPARSE_IMAGE_NUM_PAGES_IN_SINGLE_ALLOC;
			memory_allocate_info.memoryTypeIndex = memory_type_index;

			VkDeviceMemory memory;
			VK_CHECK(vkAllocateMemory(device, &memory_allocate_info, nullptr, &memory));
			this->memory = memory;

			for (size_t i = 0U; i < SPARSE_IMAGE_NUM_PAGES_IN_SINGLE_ALLOC; i++)
			{
				available_offsets.insert(page_size * i);
			}
		}

		~MemSector()
		{
			vkFreeMemory(device, memory, nullptr);
		}
	};

	friend bool sort_memory_sector(const std::weak_ptr<MemSector> left, const std::weak_ptr<MemSector> right)
	{
		if (left.expired())
		{
			return false;
		}
		else if (right.expired())
		{
			return true;
		}
		return left.lock()->available_offsets.size() > right.lock()->available_offsets.size();
	};

	struct VirtualTexture
	{
		VkImage      texture_image;
		VkImageView  texture_image_view;
		MemAllocInfo memory_allocations;

		// Dimensions
		size_t width;
		size_t height;

		// Number of bytes per page
		size_t page_size;

		uint8_t                    base_mip_level;
		uint8_t                    mip_levels;
		std::vector<MipProperties> mip_properties;

		std::vector<std::vector<MipBlock>> current_mip_table;
		std::vector<std::vector<MipBlock>> new_mip_table;

		// Image containing a single, most detailed mip, allocated in the CPU memory, coppied to VRAM via staging buffer single_page_buffer
		std::unique_ptr<vkb::sg::Image>    raw_data_image;
		std::unique_ptr<vkb::core::Buffer> single_page_buffer;

		// Key table that includes data on which page is allocated to what memory block from the textureMemory vector
		std::vector<PageTable> page_table;

		// Set containing BLOCKS for which the required mip level has changed or/and its on-screen visibility changed
		std::set<TextureBlock> texture_block_update_set;

		// Set containing information which pages from the page_table should be updated (either loaded from CPU memory or blitted)
		std::set<size_t> update_set;

		// Vector of available memory pages (whole memory page pool is statically allocated at the beginning)
		std::vector<size_t> available_memory_page_indices;

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

	bool        update_frag     = false;
	bool        update_required = false;
	uint8_t     frame_counter   = 0U;
	enum Stages next_stage      = SPARSE_IMAGE_IDLE_STAGE;

	VirtualTexture virtual_texture;

	VkQueue sparse_queue;
	size_t  sparse_queue_family_index;

	std::unique_ptr<vkb::core::Buffer> vertex_buffer;

	std::unique_ptr<vkb::core::Buffer> index_buffer;
	size_t                             index_count;

	std::unique_ptr<vkb::core::Buffer> mvp_buffer;
	std::unique_ptr<vkb::core::Buffer> frag_settings_data_buffer;

	glm::mat4 current_mvp_transform;

	VkPipeline       sample_pipeline{};
	VkPipelineLayout sample_pipeline_layout{};

	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorSet       descriptor_set;
	VkSampler             texture_sampler;

	//==================================================================================================
	SparseImage();
	virtual ~SparseImage();

	void setup_camera();
	void load_assets();

	void prepare_pipelines();

	void create_sparse_bind_queue();

	void create_vertex_buffer();
	void create_index_buffer();

	void create_uniform_buffers();
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
	struct MemPageDescription get_mem_page_description(size_t page_index);
	void                      calculate_mips_table(glm::mat4 mvp_transform, uint32_t numVerticalBlocks, uint32_t numHorizontalBlocks, std::vector<std::vector<MipBlock>> &mipTable);
	void                      compare_mips_table();
	void                      process_texture_block(const TextureBlock &on_screen_block);
	std::vector<size_t>       get_memory_dependency_for_the_block(size_t column, size_t row, uint8_t mip_level);
	void                      check_mip_page_requirements(std::vector<MemPageDescription> &mipgen_required_vec, MemPageDescription mip_dependency);
	void                      bind_sparse_image();
	void                      copy_single_raw_data_block(uint8_t buffer[], const VkExtent2D blockDim, VkOffset2D offset, size_t stride);
	void                      load_least_detailed_level();
	void                      set_least_detailed_level();
	void                      update_frag_settings();
	uint8_t                   get_mip_level(size_t page_index);
	size_t                    get_page_index(MemPageDescription mem_page_desc);

	// Override basic framework functionalities
	void         build_command_buffers() override;
	void         render(float delta_time) override;
	bool         prepare(const vkb::ApplicationOptions &options) override;
	void         request_gpu_features(vkb::PhysicalDevice &gpu) override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
};

std::unique_ptr<vkb::VulkanSample> create_sparse_image();
