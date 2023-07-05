/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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
		vk::Buffer               buffer;
		vk::DescriptorBufferInfo descriptor;
		vk::DeviceMemory         memory;
		size_t                   size = 0;

		void destroy(vk::Device device)
		{
			device.destroyBuffer(buffer);
			device.freeMemory(memory);
		}
	};

	// Per-instance data block
	struct InstanceData
	{
		glm::vec3 pos;
		glm::vec3 rot;
		float     scale;
		uint32_t  texIndex;
	};

	struct Model
	{
		vk::DescriptorSet                                         descriptor_set;
		std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> mesh;
		vk::Pipeline                                              pipeline;
		HPPTexture                                                texture;

		void destroy(vk::Device device)
		{
			mesh.reset();
			device.destroyPipeline(pipeline);
			device.destroySampler(texture.sampler);
		}
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

  private:
	// from vkb::Application
	bool prepare(const vkb::ApplicationOptions &options) override;
	bool resize(const uint32_t width, const uint32_t height) override;

	// from HPPVulkanSample
	void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;

	// from HPPApiVulkanSample
	void build_command_buffers() override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void render(float delta_time) override;

	vk::DescriptorPool      create_descriptor_pool();
	vk::DescriptorSetLayout create_descriptor_set_layout();
	vk::Pipeline            create_planet_pipeline();
	vk::Pipeline            create_rocks_pipeline();
	vk::Pipeline            create_starfield_pipeline();
	void                    draw();
	void                    load_assets();
	void                    initialize_camera();
	void                    prepare_instance_data();
	void                    prepare_uniform_buffers();
	void                    update_uniform_buffer(float delta_time);
	void                    update_planet_descriptor_set();
	void                    update_rocks_descriptor_set();

  private:
	vk::DescriptorSetLayout descriptor_set_layout;
	InstanceBuffer          instance_buffer;
	Model                   planet;
	Model                   rocks;
	vk::PipelineLayout      pipeline_layout;
	vk::Pipeline            starfield_pipeline;
	UBOVS                   ubo_vs;
	UniformBuffers          uniform_buffers;
};

std::unique_ptr<vkb::Application> create_hpp_instancing();
