/* Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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
 * Texture loading (and display) example (including mip maps), using vulkan.hpp
 */

#pragma once

#include <hpp_api_vulkan_sample.h>

class HPPTextureLoading : public HPPApiVulkanSample
{
  public:
	HPPTextureLoading();
	~HPPTextureLoading();

  private:
	// Contains all Vulkan objects that are required to store and use a texture
	// Note that this repository contains a texture class (vulkan_texture.h) that encapsulates texture loading functionality in a class that is used in subsequent demos
	struct Texture
	{
		vk::DeviceMemory device_memory;
		vk::Image        image;
		vk::ImageLayout  image_layout;
		vk::ImageView    image_view;
		vk::Sampler      sampler;
		vk::Extent2D     extent;
		uint32_t         mip_levels;

		void destroy(vk::Device device)
		{
			device.destroyImageView(image_view);
			device.destroyImage(image);
			device.destroySampler(sampler);
			device.freeMemory(device_memory);
		}
	};

	// Vertex layout for this example
	struct Vertex
	{
		float pos[3];
		float uv[2];
		float normal[3];
	};

	struct VertexShaderData
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 view_pos;
		float     lod_bias = 0.0f;
	};

  private:
	// from vkb::Application
	bool prepare(const vkb::ApplicationOptions &options) override;

	// from HPPVulkanSample
	void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;

	// from HPPApiVulkanSample
	void build_command_buffers() override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void render(float delta_time) override;
	void view_changed() override;

	vk::DescriptorPool      create_descriptor_pool();
	vk::DescriptorSetLayout create_descriptor_set_layout();
	vk::Pipeline            create_pipeline();
	void                    draw();
	void                    generate_quad();
	void                    load_texture();
	void                    prepare_uniform_buffers();
	void                    update_descriptor_set();
	void                    update_uniform_buffers();

  private:
	vk::DescriptorSet                     descriptor_set;
	vk::DescriptorSetLayout               descriptor_set_layout;
	std::unique_ptr<vkb::core::HPPBuffer> index_buffer;
	uint32_t                              index_count;
	vk::Pipeline                          pipeline;
	vk::PipelineLayout                    pipeline_layout;
	Texture                               texture;
	std::unique_ptr<vkb::core::HPPBuffer> vertex_buffer;
	VertexShaderData                      vertex_shader_data;
	std::unique_ptr<vkb::core::HPPBuffer> vertex_shader_data_buffer;
};

std::unique_ptr<vkb::Application> create_hpp_texture_loading();
