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

size_t const ON_SCREEN_HORIZONTAL_BLOCKS = 100;
size_t const ON_SCREEN_VERTICAL_BLOCKS   = 80;
double const FOV_DEGREES                 = 60.0;
double const MIP_LEVEL_MARGIN            = 0.3;

class sparse_image : public ApiVulkanSample
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

		struct SimpleBuffer single_page_buffer;


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




	struct CalculateEdgeData
	{
		std::array<glm::vec4, 2> vertices;
		bool                     is_vertical;

		uint32_t xy_num_blocks;
		uint32_t xy_extent[2];

		std::vector<Point> xy_edge_coords;

		CalculateEdgeData(bool is_Vertical, uint32_t num_blocks, VkExtent2D extent_2D, glm::vec4 A, glm::vec4 B);
		void calculate_edge_coordinates();
	};

    struct CalculateMipLevelData
	{
		std::array<struct CalculateEdgeData, 4U> edge_data;
		std::vector<std::vector<Point>>          mesh;
		std::vector<std::vector<MipBlock>>       mip_table;
		std::vector<float>                       ax_vertical;
		std::vector<float>                       ax_horizontal;

		VkExtent2D texture_base_dim;

		CalculateMipLevelData(std::array<struct CalculateEdgeData, 4U> &edge_data, VkExtent2D texture_base_dim);
		void calculate_mesh_coordinates();
		void calculate_mip_levels();
	};

	struct VirtualTexture virtual_texture;

	struct SimpleBuffer vertex_buffer;
	struct SimpleBuffer index_buffer;
	struct SimpleBuffer mvp_buffer;

	VkPipeline       sample_pipeline{};
	VkPipelineLayout sample_pipeline_layout{};
	
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorSet       descriptor_set;
	VkSampler             texture_sampler;

	size_t on_screen_num_vertical_blocks   = ON_SCREEN_VERTICAL_BLOCKS;
	size_t on_screen_num_horizontal_blocks = ON_SCREEN_HORIZONTAL_BLOCKS;
	double fov_degrees                     = FOV_DEGREES;
	double mip_level_margin                = MIP_LEVEL_MARGIN;


	sparse_image();
	virtual ~sparse_image();
	void cleanup_simple_buffer(struct SimpleBuffer &simple_buffer);

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
	void                      calculate_mips_table(std::array<glm::vec4, 4> vertices, uint32_t numVerticalBlocks, uint32_t numHorizontalBlocks, std::vector<std::vector<MipBlock>> &mipTable);
	void                      compare_mips_table();
	void                      calculate_required_memory_layout();
	void                      get_associated_memory_blocks(const TextureBlock &on_screen_block);
	void                      get_memory_dependency_for_the_block(size_t column, size_t row, uint8_t mip_level, std::list<size_t> &index_list);
	void                      check_mip_page_requirements(std::list<MemPageDescription> &mipgen_required_list, struct MemPageDescription mip_dependency);
	void                      bind_sparse_image();
	void                      separate_single_row_data_block(uint8_t buffer[], const VkExtent2D blockDim, VkOffset2D offset, size_t stride);
	void                      end_single_time_commands(VkCommandBuffer command_buffer);
	VkCommandBuffer           begin_single_time_command();
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
