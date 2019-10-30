/* Copyright (c) 2019, Sascha Willems
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
 * Instanced mesh rendering, uses a separate vertex buffer for instanced data
 */

#pragma once

#include "api_vulkan_sample.h"

#if defined(__ANDROID__)
#	define INSTANCE_COUNT 4096
#else
#	define INSTANCE_COUNT 8192
#endif

class Instancing : public ApiVulkanSample
{
  public:
	struct Textures
	{
		Texture rocks;
		Texture planet;
	} textures;

	struct Models
	{
		std::unique_ptr<vkb::sg::SubMesh> rock;
		std::unique_ptr<vkb::sg::SubMesh> planet;
	} models;

	// Per-instance data block
	struct InstanceData
	{
		glm::vec3 pos;
		glm::vec3 rot;
		float     scale;
		uint32_t  texIndex;
	};
	// Contains the instanced data
	struct InstanceBuffer
	{
		VkBuffer               buffer = VK_NULL_HANDLE;
		VkDeviceMemory         memory = VK_NULL_HANDLE;
		size_t                 size   = 0;
		VkDescriptorBufferInfo descriptor;
	} instance_buffer;

	struct UBOVS
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec4 light_pos  = glm::vec4(0.0f, -5.0f, 0.0f, 1.0f);
		float     loc_speed  = 0.0f;
		float     glob_speed = 0.0f;
	} ubo_vs;

	struct UniformBuffers
	{
		std::unique_ptr<vkb::core::Buffer> scene;
	} uniform_buffers;

	VkPipelineLayout pipeline_layout;
	struct Pipelines
	{
		VkPipeline instanced_rocks;
		VkPipeline planet;
		VkPipeline starfield;
	} pipelines;

	VkDescriptorSetLayout descriptor_set_layout;
	struct DescriptorSets
	{
		VkDescriptorSet instanced_rocks;
		VkDescriptorSet planet;
	} descriptor_sets;

	Instancing();
	~Instancing();
	virtual void get_device_features() override;
	void         build_command_buffers() override;
	void         load_assets();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_set();
	void         prepare_pipelines();
	void         prepare_instance_data();
	void         prepare_uniform_buffers();
	void         update_uniform_buffer(float delta_time);
	void         draw();
	bool         prepare(vkb::Platform &platform) override;
	virtual void render(float delta_time) override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
	virtual void resize(const uint32_t width, const uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_instancing();
