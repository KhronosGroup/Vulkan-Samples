/* Copyright (c) 2019-2023, Sascha Willems
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

#include "api_vulkan_sample.h"

class PushDescriptors : public ApiVulkanSample
{
  public:
	bool animate = true;

	PFN_vkCmdPushDescriptorSetKHR               vkCmdPushDescriptorSetKHR;
	VkPhysicalDevicePushDescriptorPropertiesKHR push_descriptor_properties{};

	struct Cube
	{
		Texture                            texture;
		std::unique_ptr<vkb::core::Buffer> uniform_buffer;
		glm::vec3                          rotation;
		glm::mat4                          model_mat;
	};
	std::array<Cube, 2> cubes;

	struct Models
	{
		std::unique_ptr<vkb::sg::SubMesh> cube;
	} models;

	struct UniformBuffers
	{
		std::unique_ptr<vkb::core::Buffer> scene;
	} uniform_buffers;

	struct UboScene
	{
		glm::mat4 projection;
		glm::mat4 view;
	} ubo_scene;

	VkPipeline            pipeline;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSetLayout descriptor_set_layout;

	PushDescriptors();
	~PushDescriptors();
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         build_command_buffers() override;
	void         load_assets();
	void         setup_descriptor_set_layout();
	void         prepare_pipelines();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	void         update_cube_uniform_buffers(float delta_time);
	void         draw();
	bool         prepare(const vkb::ApplicationOptions &options) override;
	virtual void render(float delta_time) override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
};

std::unique_ptr<vkb::VulkanSample> create_push_descriptors();
