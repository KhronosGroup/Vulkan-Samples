/* Copyright (c) 2023, Sascha Willems
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
 * Using descriptor buffers replacing descriptor sets with VK_EXT_descriptor_buffer
 * This render multiple cubes passing uniform buffers and combined image samples to the GPU using descriptor buffers instead of descriptor sets
 * This allows for a more bindless design
 */

#pragma once

#include "api_vulkan_sample.h"

class DescriptorBufferBasic : public ApiVulkanSample
{
  public:
	bool animate = true;

	std::unique_ptr<vkb::core::Buffer> resource_descriptor_buffer;
	std::unique_ptr<vkb::core::Buffer> image_descriptor_buffer;

	VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptor_buffer_properties{};

	VkDeviceSize uniform_descriptor_offset;
	VkDeviceSize image_descriptor_offset;

	VkDescriptorSetLayout descriptor_set_layout_buffer{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout_image{VK_NULL_HANDLE};

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

	VkPipeline       pipeline{VK_NULL_HANDLE};
	VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};

	DescriptorBufferBasic();
	~DescriptorBufferBasic() override;
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         build_command_buffers() override;
	void         load_assets();
	void         setup_descriptor_set_layout();
	void         prepare_pipelines();
	void         prepare_descriptor_buffer();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	void         update_cube_uniform_buffers(float delta_time);
	void         draw();
	bool         prepare(vkb::Platform &platform) override;
	void         render(float delta_time) override;
	void         on_update_ui_overlay(vkb::Drawer &drawer) override;
};

std::unique_ptr<vkb::VulkanSample> create_descriptor_buffer_basic();
