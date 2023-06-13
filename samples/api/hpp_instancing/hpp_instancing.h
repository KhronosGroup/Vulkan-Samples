/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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
 * Instanced mesh rendering, uses a separate vertex buffer for instanced data, using vulkan.hpp
 */

#pragma once

#include <hpp_api_vulkan_sample.h>

#if defined(__ANDROID__)
#	define INSTANCE_COUNT 4096
#else
#	define INSTANCE_COUNT 8192
#endif

class HPPInstancing : public HPPApiVulkanSample
{
  public:
	HPPInstancing();
	~HPPInstancing();

  private:
	// Contains the instanced data
	struct InstanceBuffer
	{
		vk::Buffer               buffer = nullptr;
		vk::DeviceMemory         memory = nullptr;
		size_t                   size   = 0;
		vk::DescriptorBufferInfo descriptor;
	};

	// Per-instance data block
	struct InstanceData
	{
		glm::vec3 pos;
		glm::vec3 rot;
		float     scale;
		uint32_t  texIndex;
	};

	struct Models
	{
		std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> rock;
		std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> planet;
	};

	struct Textures
	{
		HPPTexture rocks;
		HPPTexture planet;
	};

	struct UBOVS
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec4 light_pos  = glm::vec4(0.0f, -5.0f, 0.0f, 1.0f);
		float     loc_speed  = 0.0f;
		float     glob_speed = 0.0f;
	};

	struct UniformBuffers
	{
		std::unique_ptr<vkb::core::HPPBuffer> scene;
	};

	struct Pipelines
	{
		vk::Pipeline instanced_rocks;
		vk::Pipeline planet;
		vk::Pipeline starfield;
	};

	struct DescriptorSets
	{
		vk::DescriptorSet instanced_rocks;
		vk::DescriptorSet planet;
	};

  private:

	bool prepare(const vkb::ApplicationOptions &options) override;
	bool resize(const uint32_t width, const uint32_t height) override;

	// from HPPVulkanSample
	void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;

	// from HPPApiVulkanSample
	void build_command_buffers() override;
	void on_update_ui_overlay(vkb::HPPDrawer &drawer) override;
	void render(float delta_time) override;

	void draw();
	void load_assets();
	void prepare_instance_data();
	void prepare_pipelines();
	void prepare_uniform_buffers();
	void setup_descriptor_pool();
	void setup_descriptor_set();
	void setup_descriptor_set_layout();
	void update_uniform_buffer(float delta_time);

  private:
	vk::DescriptorSetLayout descriptor_set_layout;
	DescriptorSets          descriptor_sets;
	InstanceBuffer          instance_buffer;
	Models                  models;
	vk::PipelineLayout      pipeline_layout;
	Pipelines               pipelines;
	Textures                textures;
	UBOVS                   ubo_vs;
	UniformBuffers          uniform_buffers;
};

std::unique_ptr<vkb::Application> create_hpp_instancing();
