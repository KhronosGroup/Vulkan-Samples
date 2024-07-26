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

class MobileNerf : public ApiVulkanSample
{
  public:
	MobileNerf();
	~MobileNerf() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void render(float delta_time) override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	bool resize(const uint32_t width, const uint32_t height) override;

  private:
	struct GlobalUniform
	{
		glm::mat4x4 model;
		glm::mat4x4 view;
		glm::mat4x4 proj;
		alignas(16) glm::vec3 camera_position;
		alignas(16) glm::vec3 camera_side;
		alignas(16) glm::vec3 camera_up;
		alignas(16) glm::vec3 camera_lookat;
		alignas(8) glm::vec2 img_dim;
		alignas(4) float tan_half_fov;
	} global_uniform;

	struct PushConstants
	{
		unsigned int weight_idx;
	} push_constants;

#define WEIGHTS_0_COUNT (176)
#define WEIGHTS_1_COUNT (256)
// The third layer weights' size is changed from 48 to 64 to make sure a 16 bytes alignement
#define WEIGHTS_2_COUNT (64)
#define BIAS_0_COUNT (16)
#define BIAS_1_COUNT (16)
// The third layer bias' size is changed from 3 to 4 to make sure a 16 bytes alignement
#define BIAS_2_COUNT (4)

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

	struct InstanceData
	{
		glm::vec3 pos_offset;
	};

	struct FrameBufferAttachment
	{
		using ImagePtr = std::unique_ptr<vkb::core::Image>;
		ImagePtr    image;
		VkSampler   sampler;
		VkImageView view;

		operator bool() const
		{
			return image && image->get_handle() != VK_NULL_HANDLE;
		}

		void destroy();
	};

	struct Model
	{
		std::vector<Vertex>                  vertices;
		std::vector<std::array<uint32_t, 3>> indices;

		Texture texture_input_0, texture_input_1;

		// Vulkan Buffers for each model
		std::unique_ptr<vkb::core::Buffer> vertex_buffer{nullptr};
		std::unique_ptr<vkb::core::Buffer> index_buffer{nullptr};

		// Each model will have its own pipeline and descriptor set
		VkPipeline pipeline_first_pass{VK_NULL_HANDLE};
		// We make the descriptor set a vector for the forward mode
		// Deferred mode will only have one set of descriptor per model
		std::vector<VkDescriptorSet> descriptor_set_first_pass{VK_NULL_HANDLE};

		int sub_model_num;
		int model_index;
	};

	std::vector<Model> models;

	// MLPs for each model
	std::vector<MLP_Weights>                        mlp_weight_vector;
	std::vector<std::unique_ptr<vkb::core::Buffer>> weights_buffers;

	// Uniform buffer for each model
	std::vector<std::unique_ptr<vkb::core::Buffer>> uniform_buffers;

	// Buffer to store instance data
	std::unique_ptr<vkb::core::Buffer> instance_buffer{nullptr};

	// Common
	void read_json_map();
	void load_shaders();
	void build_command_buffers() override;
	void create_uniforms();
	void create_static_object_buffers(int model_index, int sub_model_index, int models_entry);
	void prepare_instance_data();
	void load_scene(int model_index, int sub_model_index, int models_entry);
	void initialize_mlp_uniform_buffers(int model_index);
	void update_uniform_buffers();
	void update_weights_buffers();

	void     create_texture(int model_index, int sub_model_index, int models_entry);
	void     create_texture_helper(std::string const &texturePath, Texture &texture);
	VkFormat feature_map_format = VK_FORMAT_R16G16B16A16_SFLOAT;

	// Helper function
	void setup_attachment(VkFormat format, VkImageUsageFlags usage, FrameBufferAttachment &attachment);

	void draw();

	// First pass
	VkDescriptorSetLayout                          descriptor_set_first_pass_layout{VK_NULL_HANDLE};
	VkPipelineLayout                               pipeline_first_pass_layout{VK_NULL_HANDLE};
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages_first_pass;
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages_second_pass;

	void create_descriptor_pool();
	void create_pipeline_layout_fist_pass();
	void create_descriptor_sets_first_pass(Model &model);
	void prepare_pipelines();

	unsigned int color_attach_0_idx;
	unsigned int color_attach_1_idx;
	unsigned int color_attach_2_idx;
	unsigned int color_attach_3_idx;
	unsigned int depth_attach_idx;
	unsigned int swapchain_attach_idx;

	struct Attachments_baseline
	{
		FrameBufferAttachment feature_0, feature_1, feature_2, weights_idx;
	};

	std::vector<Attachments_baseline> frameAttachments;
	std::vector<VkFramebuffer>        nerf_framebuffers;
	VkRenderPass                      render_pass_nerf{VK_NULL_HANDLE};

	// For the baseline (mlp in frag shader) second pass
	VkPipeline                   pipeline_baseline{VK_NULL_HANDLE};
	VkPipelineLayout             pipeline_layout_baseline{VK_NULL_HANDLE};
	VkDescriptorSetLayout        descriptor_set_layout_baseline{VK_NULL_HANDLE};
	std::vector<VkDescriptorSet> descriptor_set_baseline{VK_NULL_HANDLE};

	void update_render_pass_nerf_baseline();
	void create_pipeline_layout_baseline();
	void create_descriptor_sets_baseline();
	void setup_nerf_framebuffer_baseline();
	void update_descriptor_sets_baseline();
	void build_command_buffers_baseline();

	// For creating the forward mode rendepass
	void update_render_pass_nerf_forward();

	// For loading nerf assets map
	json                     asset_map;
	std::vector<std::string> model_path;
	std::vector<bool>        using_original_nerf_models;
	bool                     combo_mode   = false;
	bool                     use_deferred = false;
	bool                     do_rotation  = false;

	glm::vec3 camera_pos = glm::vec3(-2.2f, 2.2f, 2.2f);

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
};

std::unique_ptr<vkb::VulkanSampleC> create_mobile_nerf();
