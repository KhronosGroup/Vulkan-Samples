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

class DynamicMultisampleRasterization : public ApiVulkanSample
{
  public:
	DynamicMultisampleRasterization();
	virtual ~DynamicMultisampleRasterization();

  private:
	std::unique_ptr<vkb::core::Buffer> vertex_buffer = nullptr;
	std::unique_ptr<vkb::core::Buffer> index_buffer  = nullptr;

	std::unique_ptr<vkb::sg::Scene>    scene;
	std::vector<VkDescriptorImageInfo> image_infos;
	std::map<std::string, int32_t>     name_to_texture_id;

	struct SceneNode
	{
		vkb::sg::Node    *node;
		vkb::sg::SubMesh *sub_mesh;
	};
	std::vector<SceneNode> scene_nodes_opaque;
	std::vector<SceneNode> scene_nodes_opaque_flipped;
	std::vector<SceneNode> scene_nodes_transparent;
	std::vector<SceneNode> scene_nodes_transparent_flipped;

	struct UniformData
	{
		glm::mat4 projection;
		glm::mat4 view;
	} uniform_data;
	std::unique_ptr<vkb::core::Buffer> uniform_buffer;

	VkPipeline            pipeline;
	VkPipeline            pipeline_inversed_rasterizer;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;

	// GUI
	VkPipeline            pipeline_gui;
	VkPipelineLayout      pipeline_layout_gui;
	VkDescriptorSet       descriptor_set_gui;
	VkDescriptorSetLayout descriptor_set_layout_gui;
	VkDescriptorPool      descriptor_pool_gui;

	/**
	 * @brief List of MSAA levels supported by the platform
	 */
	std::vector<VkSampleCountFlagBits> supported_sample_count_list{};

	/**
	 * @brief Enables MSAA if set to more than 1 sample per pixel
	 *        (e.g. sample count 4 enables 4X MSAA)
	 */
	VkSampleCountFlagBits sample_count{VK_SAMPLE_COUNT_1_BIT};

	VkSampleCountFlagBits gui_sample_count{VK_SAMPLE_COUNT_1_BIT};

	VkSampleCountFlagBits last_gui_sample_count{VK_SAMPLE_COUNT_1_BIT};

	bool sample_count_prepared = false;

	struct
	{
		glm::mat4 model_matrix;

		glm::vec4 base_color_factor;
		float     metallic_factor;
		float     roughness_factor;

		int32_t base_texture_index;
		int32_t normal_texture_index;
		int32_t pbr_texture_index;

	} push_const_block;

	struct GUI_settings
	{
		int                      gui_sample_count = 0;
		std::vector<std::string> sample_counts;
	} gui_settings;

	struct
	{
		VkImage        image;
		VkDeviceMemory mem;
		VkImageView    view;
	} color_attachment;

	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extended_dynamic_state_3_features{};

  public:
	virtual void build_command_buffers() override;
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	virtual bool prepare(const vkb::ApplicationOptions &options) override;
	virtual void setup_render_pass() override;
	virtual void render(float delta_time) override;
	virtual void setup_framebuffer() override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
	virtual bool resize(const uint32_t _width, const uint32_t _height) override;
	virtual void setup_depth_stencil() override;
	virtual void prepare_gui() override;
	void         load_assets();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_sets();
	void         prepare_pipelines();
	void         prepare_gui_pipeline();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	void         draw();
	void         prepare_supported_sample_count_list();
	void         setup_color_attachment();
	void         draw_ui(VkCommandBuffer &);
	void         update_resources();
	void         draw_node(VkCommandBuffer &, SceneNode &);
};

std::unique_ptr<vkb::VulkanSample> create_dynamic_multisample_rasterization();
