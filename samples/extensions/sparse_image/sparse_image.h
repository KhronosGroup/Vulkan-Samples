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

size_t const ON_SCREEN_HORIZONTAL_BLOCKS = 10;
size_t const ON_SCREEN_VERTICAL_BLOCKS   = 6;
double const FOV_DEGREES                 = 60.0;
double const MIP_LEVEL_MARGIN            = 0.3;

class SparseImage : public ApiVulkanSample
{
  public:

	struct MVP
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	struct SimpleBuffer
	{
		VkBuffer       buffer;
		VkDeviceMemory memory;
		void          *mapped_memory;
		size_t         num_elements;
		size_t         size;
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
		size_t num_blocks;
		size_t block_base_index;        
		size_t width;
		size_t height;
	};

	struct TextureBlock
	{
		uint32_t row;
		uint32_t column;
		double   old_mip_level;
		double   new_mip_level;
		bool     on_screen;
	};

	struct MemPageDescription
	{
		size_t  x;
		size_t  y;
		uint8_t mip_level;
	};

	struct PageTable
	{
		bool                                 bound;             
		bool                                 written;
		bool                                 gen_mip_required;        
		size_t                               memory_index;
		std::list<std::pair<size_t, size_t>> render_required_list;
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

	struct VirtualTexture
	{
		VkImage        texture_image;
		VkImageView    texture_image_view;
		VkDeviceMemory texture_memory;        

		std::unique_ptr<vkb::sg::Image> row_data_image;

		std::unique_ptr<vkb::core::Buffer> single_page_buffer;

		std::list<size_t> available_memory_index_list;

		// dimensions
		size_t width;
		size_t height;

		// number of virtual pages (what if the total image was allocated)
		size_t num_pages;
		size_t page_size;

		// table that includes data on which page is allocated to what memory block from the textureMemory vector.
		std::vector<struct PageTable> page_table;

		// list containing information which pages from the virtual should be bound
		std::list<size_t> bind_list;

		// list containing information which pages should be freed (present but not required or required for mip generation)
		std::list<size_t> free_list;

		bool update_required = false;

		// sparse image - related format and memory properties
		VkSparseImageFormatProperties   format_properties;
		VkSparseImageMemoryRequirements memory_sparse_requirements;
		VkMemoryRequirements            mem_requirements;

		std::vector<std::vector<MipBlock>> current_mip_table;
		std::vector<std::vector<MipBlock>> new_mip_table;
		std::list<struct TextureBlock>     texture_block_update_list;

		uint8_t base_mip_level;
		uint8_t mip_levels;

		std::vector<struct MipProperties> mip_properties;
	};


    struct CalculateMipLevelData
	{
		std::vector<std::vector<Point>>          mesh;
		std::vector<std::vector<MipBlock>>       mip_table;

		uint32_t vertical_num_blocks;
		uint32_t horizontal_num_blocks;

		std::vector<float>                       ax_vertical;
		std::vector<float>                       ax_horizontal;

		glm::mat4  mvp_transform;

		VkExtent2D texture_base_dim;
		VkExtent2D screen_base_dim;

		CalculateMipLevelData(glm::mat4 mvp_transform, VkExtent2D texture_base_dim, VkExtent2D screen_base_dim, uint32_t vertical_num_blocks, uint32_t horizontal_num_blocks);
		void calculate_mesh_coordinates();
		void calculate_mip_levels();
	};

	struct VirtualTexture virtual_texture;

	std::unique_ptr<vkb::core::Buffer> vertex_buffer;

	std::unique_ptr<vkb::core::Buffer> index_buffer;
	size_t                             index_count;

	std::unique_ptr<vkb::core::Buffer> mvp_buffer;

	VkPipeline       sample_pipeline{};
	VkPipelineLayout sample_pipeline_layout{};
	
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorSet       descriptor_set;
	VkSampler             texture_sampler;

	size_t on_screen_num_vertical_blocks   = ON_SCREEN_VERTICAL_BLOCKS;
	size_t on_screen_num_horizontal_blocks = ON_SCREEN_HORIZONTAL_BLOCKS;
	double fov_degrees                     = FOV_DEGREES;
	double mip_level_margin                = MIP_LEVEL_MARGIN;


	SparseImage();
	virtual ~SparseImage();

	void setup_camera();
	void load_assets();
	void process_camera_change();

	void prepare_pipelines();

	void create_vertex_buffer();
	void create_index_buffer();

	void create_uniform_buffer();
	void create_texture_sampler();

	void create_descriptor_set_layout();
	void create_descriptor_pool();
	void create_descriptor_sets();

	void create_sparse_texture_image();

	struct MemPageDescription get_mem_page_description(size_t memory_index);
	void                      calculate_mips_table(glm::mat4 mvp_transform, uint32_t numVerticalBlocks, uint32_t numHorizontalBlocks, std::vector<std::vector<MipBlock>> &mipTable);
	void                      compare_mips_table();
	void                      calculate_required_memory_layout();
	void                      get_associated_memory_blocks(const TextureBlock &on_screen_block);
	void                      get_memory_dependency_for_the_block(size_t column, size_t row, uint8_t mip_level, std::list<size_t> &index_list);
	void                      check_mip_page_requirements(std::list<MemPageDescription> &mipgen_required_list, struct MemPageDescription mip_dependency);
	void                      bind_sparse_image();
	void                      separate_single_row_data_block(uint8_t buffer[], const VkExtent2D blockDim, VkOffset2D offset, size_t stride);
	void                      generate_mips();
	void                      transitionImageLayout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);

	uint32_t                  get_mip_level(size_t page_index);


	// Override basic framework functionality
	void build_command_buffers() override;
	void render(float delta_time) override;
	void view_changed() override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
};

std::unique_ptr<vkb::VulkanSample> create_sparse_image();
