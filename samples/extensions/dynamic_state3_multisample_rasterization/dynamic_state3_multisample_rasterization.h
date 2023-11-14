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

class DynamicState3MultisampleRasterization : public ApiVulkanSample
{
  public:
	DynamicState3MultisampleRasterization();
	virtual ~DynamicState3MultisampleRasterization();

private:
	std::unique_ptr<vkb::core::Buffer> vertex_buffer = nullptr;
	std::unique_ptr<vkb::core::Buffer> index_buffer  = nullptr;

	std::unique_ptr<vkb::sg::Scene> scene;
	std::vector<VkDescriptorImageInfo> image_infos;
	std::map<std::string, int>         name_to_texture_id;

	struct SceneNode
	{
		std::string       name;
		vkb::sg::Node    *node;
		vkb::sg::SubMesh *sub_mesh;
	};
	std::vector<SceneNode> scene_nodes;

	struct UniformData
	{
		glm::mat4 projection;
		glm::mat4 view;
	} uniform_data;
	std::unique_ptr<vkb::core::Buffer> uniform_buffer;

	VkPipeline            pipeline;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;

	struct
	{
		glm::mat4 model_matrix;

		glm::vec4 base_color_factor;
		float metallic_factor;
		float roughness_factor;

		uint32_t  base_texture_index;
		uint32_t  normal_texture_index;
		uint32_t  pbr_texture_index;

	} push_const_block;

public:
	void         build_command_buffers() override;
	void         load_assets();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_sets();
	void         prepare_pipelines();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	void         draw();
	bool         prepare(const vkb::ApplicationOptions &options) override;
	virtual void render(float delta_time) override;
};

std::unique_ptr<vkb::VulkanSample> create_dynamic_state3_multisample_rasterization();
