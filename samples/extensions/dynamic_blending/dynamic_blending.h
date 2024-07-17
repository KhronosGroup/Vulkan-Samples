/* Copyright (c) 2023-2024, Mobica
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

class DynamicBlending : public ApiVulkanSample
{
  public:
	DynamicBlending();
	~DynamicBlending();

	void render(float delta_time) override;
	void build_command_buffers() override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	bool resize(const uint32_t width, const uint32_t height) override;

	void prepare_scene();
	void setup_descriptor_pool();
	void create_descriptor_set_layout();
	void create_descriptor_set();
	void create_pipelines();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void update_color_uniform();
	void draw();
	void initialize_operator_names();
	void update_pipeline();

  private:
	VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT blend_properties;

	bool reverse = false;

	VkCommandBuffer copy_cmd;

	struct Texture
	{
		VkImage        image;
		VkDeviceMemory memory;
		VkImageView    view;
		VkSampler      sampler;
	} texture;

	struct FacePreferences
	{
		uint16_t                            index_offset;
		uint16_t                            index_count;
		std::array<bool, 4>                 color_bit_enabled;
		std::array<std::array<float, 4>, 4> color;
	} face_preferences[2];

	struct Vertex
	{
		std::array<float, 3> pos;
		std::array<float, 2> uv;
	};

	struct CameraUbo
	{
		alignas(16) glm::mat4 projection;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 model;
	};

	struct ColorUbo
	{
		alignas(32) glm::vec4 data[8];
	};

	struct
	{
		std::unique_ptr<vkb::core::BufferC> common;
	} uniform_buffers;

	struct
	{
		std::vector<VkBlendOp>   values;
		std::vector<std::string> names;
	} blend_operator, advanced_blend_operator;

	std::vector<std::string> blend_factor_names;

	std::unique_ptr<vkb::core::BufferC> vertex_buffer;
	std::unique_ptr<vkb::core::BufferC> index_buffer;
	uint32_t                            index_count = 0;

	std::vector<Vertex> vertices;
	uint32_t            vertex_buffer_size;
	uint8_t             current_face_index = 1;

	std::unique_ptr<vkb::core::BufferC> camera_ubo;
	std::array<bool, 8>                 color_bit;

	ColorUbo                            color;
	std::unique_ptr<vkb::core::BufferC> color_ubo;

	VkDescriptorPool      descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkPipeline            pipeline;

	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT eds_feature_support;

	std::array<float, 4> clear_color = {0.5f, 0.5f, 0.5f, 1.0f};
	int32_t              current_blend_color_operator_index;
	int32_t              current_blend_alpha_operator_index;
	int32_t              current_advanced_blend_operator_index;
	int32_t              current_blend_option           = 0;
	int32_t              current_src_color_blend_factor = VK_BLEND_FACTOR_SRC_ALPHA;
	int32_t              current_dst_color_blend_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	int32_t              current_src_alpha_blend_factor = VK_BLEND_FACTOR_ZERO;
	int32_t              current_dst_alpha_blend_factor = VK_BLEND_FACTOR_ONE;

	bool blend_enable      = true;
	bool src_premultiplied = true;
	bool dst_premultiplied = true;
	bool clamp_results     = true;

	std::default_random_engine            rnd_engine;
	std::uniform_real_distribution<float> rnd_dist;

	void build_command_buffer_for_plane(VkCommandBuffer &command_buffer, FacePreferences preferences);
	void update_color();
	void randomize_color(std::array<float, 4> &color, bool alpha = false);
};

std::unique_ptr<vkb::VulkanSampleC> create_dynamic_blending();
