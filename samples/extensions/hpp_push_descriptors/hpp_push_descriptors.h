/* Copyright (c) 2019-2025, Sascha Willems
 * Copyright (c) 2024-2025, NVIDIA CORPORATION. All rights reserved.
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
 * Push descriptors
 *
 * Note: Requires a device that supports the VK_KHR_push_descriptor extension
 *
 * Push descriptors apply the push constants concept to descriptor sets. So instead of creating
 * per-model descriptor sets (along with a pool for each descriptor type) for rendering multiple objects,
 * this example uses push descriptors to pass descriptor sets for per-model textures and matrices
 * at command buffer creation time.
 */

#pragma once

#include "hpp_api_vulkan_sample.h"

class HPPPushDescriptors : public HPPApiVulkanSample
{
  public:
	HPPPushDescriptors();
	~HPPPushDescriptors();

	// from vkb::Application
	bool prepare(const vkb::ApplicationOptions &options) override;

	// from vkb::VulkanSample
	void request_gpu_features(vkb::core::PhysicalDeviceCpp &gpu) override;

	// from HPPApiVulkanSample
	void build_command_buffers() override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void render(float delta_time) override;

	void create_descriptor_set_layout();
	void create_pipeline();
	void create_pipeline_layout();
	void create_uniform_buffers();
	void draw();
	void initializeCamera();
	void load_assets();
	void update_cube_uniform_buffers(float delta_time);
	void update_uniform_buffers();

  private:
	struct Cube
	{
		HPPTexture                            texture;
		std::unique_ptr<vkb::core::BufferCpp> uniform_buffer;
		glm::vec3                             rotation;
		glm::mat4                             model_mat;
	};

	struct Models
	{
		std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> cube;
	};

	struct UboScene
	{
		glm::mat4 projection;
		glm::mat4 view;
	};

	struct UniformBuffers
	{
		std::unique_ptr<vkb::core::BufferCpp> scene;
	};

  private:
	bool                    animate = true;
	std::array<Cube, 2>     cubes;
	vk::DescriptorSetLayout descriptor_set_layout;
	uint32_t                max_push_descriptors = 0;
	Models                  models;
	vk::Pipeline            pipeline;
	vk::PipelineLayout      pipeline_layout;
	UboScene                ubo_scene;
	UniformBuffers          uniform_buffers;
};

std::unique_ptr<vkb::VulkanSampleCpp> create_hpp_push_descriptors();
