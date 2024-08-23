/* Copyright (c) 2023-2024, Qualcomm Innovation Center, Inc. All rights reserved.
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
#include "glsl_compiler.h"
#include <core/acceleration_structure.h>

#include <json.hpp>

using json = nlohmann::json;

namespace vkb
{
namespace sg
{
class Scene;
class Node;
class Mesh;
class SubMesh;
class Camera;
}        // namespace sg
}        // namespace vkb

class MobileNerfRayQuery : public ApiVulkanSample
{
  public:
	MobileNerfRayQuery();
	~MobileNerfRayQuery() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void render(float delta_time) override;
	bool prepare(const vkb::ApplicationOptions &options) override;

  private:
	struct GlobalUniform
	{
		alignas(16) glm::vec3 camera_position;
		alignas(16) glm::vec3 camera_side;
		alignas(16) glm::vec3 camera_up;
		alignas(16) glm::vec3 camera_lookat;
		alignas(8) glm::vec2 img_dim;
		alignas(4) float tan_half_fov;
	} global_uniform;

#define WEIGHTS_0_COUNT (176)
#define WEIGHTS_1_COUNT (256)
// The third layer weights' size is changed from 48 to 64 to make sure a 16 bytes alignement
// #define WEIGHTS_2_COUNT (48)
#define WEIGHTS_2_COUNT (64)
#define BIAS_0_COUNT (16)
#define BIAS_1_COUNT (16)
// The third layer bias' size is changed from 3 to 4 to make sure a 16 bytes alignement
#define BIAS_2_COUNT (4)

	// some typedef for each model
	struct MLP_Weights
	{
		float data[WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + WEIGHTS_2_COUNT +
		           BIAS_0_COUNT + BIAS_1_COUNT + BIAS_2_COUNT];        // Array of floats
	};

	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 tex_coord;
	};

	struct InstancingInfo
	{
		glm::ivec3 dim;
		glm::vec3  interval;
	};

	struct FrameBufferAttachment
	{
		VkSampler      sampler;
		VkDeviceMemory memory;
		VkImage        image = VK_NULL_HANDLE;
		VkImageView    view;
		VkFormat       format;
		uint32_t       width;
		uint32_t       height;
	};

	struct Model
	{
		int model_index;
		int sub_model_num;

		std::vector<Vertex>                  vertices;
		std::vector<std::array<uint32_t, 3>> indices;

		// Feature maps
		Texture texture_input_0, texture_input_1;

		// Each model has its vertex buffer and index buffer. In ray query, they are storage buffers.
		std::unique_ptr<vkb::core::Buffer> vertex_buffer{nullptr};
		std::unique_ptr<vkb::core::Buffer> index_buffer{nullptr};

		// Each model has its BLAS
		std::unique_ptr<vkb::core::AccelerationStructure> bottom_level_acceleration_structure{nullptr};
	} model;

	std::vector<Model> models;

	// MLPs for each model
	std::vector<MLP_Weights>                        mlp_weight_vector;
	std::vector<std::unique_ptr<vkb::core::Buffer>> weights_buffers;

	// Global uniform buffer
	std::unique_ptr<vkb::core::Buffer> uniform_buffer;

	std::vector<VkFramebuffer> framebuffers_nerf;
	VkRenderPass               render_pass_nerf{VK_NULL_HANDLE};

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

	VkPipeline       pipeline{VK_NULL_HANDLE};
	VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};

	std::vector<VkDescriptorSet> descriptor_set_common{VK_NULL_HANDLE};
	std::vector<VkDescriptorSet> descriptor_set_vertices{VK_NULL_HANDLE};
	std::vector<VkDescriptorSet> descriptor_set_indices{VK_NULL_HANDLE};
	std::vector<VkDescriptorSet> descriptor_set_feature1{VK_NULL_HANDLE};
	std::vector<VkDescriptorSet> descriptor_set_feature2{VK_NULL_HANDLE};

	VkDescriptorSetLayout descriptor_set_layout_common{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout_vertices{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout_indices{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout_feature1{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout_feature2{VK_NULL_HANDLE};

	// Ray tracing structures
	std::unique_ptr<vkb::core::AccelerationStructure> top_level_acceleration_structure{nullptr};

	// For loading mobile nerf assets map
	json                     asset_map;
	int                      num_models;
	bool                     combo_mode  = false;
	bool                     do_rotation = false;
	std::vector<std::string> model_path;
	glm::vec3                camera_pos = glm::vec3(-2.2f, 2.2f, 2.2f);

	// Currently combo mode translation are hard-coded
	glm::mat4x4 combo_model_transform[4] = {
	    glm::translate(glm::vec3(0.5, 0.75, 0)), glm::translate(glm::vec3(0.5, 0.25, 0)),
	    glm::translate(glm::vec3(0, -0.25, 0.5)), glm::translate(glm::vec3(0, -0.75, -0.5))};

	// For instancing
	InstancingInfo instancing_info;

	// Viewport Setting
	float    fov                    = 60.0f;
	uint32_t view_port_width        = width;
	uint32_t view_port_height       = height;
	bool     use_native_screen_size = false;

	// Feature map format
	VkFormat feature_map_format = VK_FORMAT_R16G16B16A16_SFLOAT;

	void read_json_map();
	void load_shaders();
	void create_uniforms();
	void create_static_object_buffers(int models_entry);
	void load_scene(int model_index, int sub_model_index, int models_entry);
	void initialize_mlp_uniform_buffers(int model_index);
	void update_uniform_buffer();
	void update_weights_buffers();

	void setup_framebuffers();
	void update_render_pass();

	void create_pipeline_layout();
	void create_descriptor_pool();
	void create_descriptor_sets();
	void prepare_pipelines();

	void build_command_buffers() override;
	void draw();

	uint64_t get_buffer_device_address(VkBuffer buffer);
	void     create_top_level_acceleration_structure();
	void     create_bottom_level_acceleration_structure(int model_entry);

	void create_texture(int model_index, int sub_model_index, int models_entry);
	void create_texture_helper(std::string const &texturePath, Texture &texture_input);
};

std::unique_ptr<vkb::VulkanSampleC> create_mobile_nerf_rayquery();
