/* Copyright (c) 2023, Holochip Corporation
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

/*
 * Demonstrate and showcase a sample application using mesh shader rendering pipeline.
 */

#pragma once

#include "api_vulkan_sample.h"

#if defined(__ANDROID__)
#	define INSTANCE_COUNT 4096
#else
#	define INSTANCE_COUNT 8192
#endif





class MeshShader : public ApiVulkanSample
{
  public:
	struct Textures
	{
		Texture rocks{};
		Texture planet{};
	} textures{};

	struct Models
	{
		std::unique_ptr<vkb::sg::SubMesh> rock{};
		std::unique_ptr<vkb::sg::SubMesh> planet{};
	} models{};

	// Per-instance data block
	struct InstanceData
	{
		glm::vec3 pos{};
		glm::vec3 rot{};
		float     scale = 1.0f;
		uint32_t  texIndex{};
	};

	// Contains the instanced data
	struct InstanceBuffer
	{
		VkBuffer               buffer = VK_NULL_HANDLE;
		VkDeviceMemory         memory = VK_NULL_HANDLE;
		size_t                 size   = 0;
		VkDescriptorBufferInfo descriptor{};
	} instance_buffer{};

	struct UBOVS
	{
		glm::mat4 projection{};
		glm::mat4 view{};
		glm::vec4 light_pos    = glm::vec4(0.0f, -5.0f, 0.0f, 1.0f);
		float     local_speed  = 0.0f;
		float     global_speed = 0.0f;
	} ubo_vs{};

	struct UniformBuffers
	{
		std::unique_ptr<vkb::core::Buffer> scene{};
	} uniform_buffers{};


	//TODO: 1-2) check out the pipeline layout here @JEREMY:
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

	struct Pipelines
	{
		VkPipeline instanced_rocks = VK_NULL_HANDLE;
		VkPipeline planet          = VK_NULL_HANDLE;
		VkPipeline star_field      = VK_NULL_HANDLE;

		//TODO: 1) mesh shader here @JEREMY:
		VkPipeline meshlet     = VK_NULL_HANDLE;

		VkPipeline draw_cull_comp     = VK_NULL_HANDLE;
		VkPipeline task_comp     = VK_NULL_HANDLE;


	} pipelines{};

	//TODO: 2) mesh shader here @JEREMY:
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;

	struct DescriptorSets
	{
		VkDescriptorSet instanced_rocks = VK_NULL_HANDLE;
		VkDescriptorSet planet          = VK_NULL_HANDLE;

		//TODO: 2) mesh shader here @JEREMY:
		VkDescriptorSet meshlet     = VK_NULL_HANDLE;
		VkDescriptorSet draw_cull_comp     = VK_NULL_HANDLE;
		VkDescriptorSet task_comp     = VK_NULL_HANDLE;


	} descriptor_sets{};

  private:

	//TODO: **) also Computer pipeline is NEEDED, write it as a helper function OUTSIDE this class! @JEREMY

	VkPipeline create_compute_pipeline(const VkPipelineShaderStageCreateInfo& shader_stage_info, VkPipelineLayout layout);

	//TODO: **-1) add a create Compute Program function

	//TODO: **-2) add a create descriptor layout function for programs
	// Ref: setup_descriptor_set_layout()

  public:

	MeshShader();
	~MeshShader() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;
	void load_assets();
	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	//TODO: I may want to set an overload to specify it 

	void setup_descriptor_set();
	void prepare_pipelines();
	void prepare_instance_data();
	void prepare_uniform_buffers();
	void update_uniform_buffer(float delta_time);
	void draw();
	bool prepare(vkb::Platform &platform) override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	bool resize(uint32_t width, uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_mesh_shader();
