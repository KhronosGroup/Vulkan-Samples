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
 * Runtime mip map generation, using vulkan.hpp
 */

#pragma once

#include <hpp_api_vulkan_sample.h>
#include <ktx.h>

class HPPTextureMipMapGeneration : public HPPApiVulkanSample
{
  public:
	HPPTextureMipMapGeneration();
	~HPPTextureMipMapGeneration();

  private:
	struct Texture
	{
		vk::Image        image;
		vk::DeviceMemory device_memory;
		vk::ImageView    view;
		vk::Extent2D     extent;
		uint32_t         mip_levels;
	};

	struct UBO
	{
		glm::mat4 projection;
		glm::mat4 model;
		float     lod_bias      = 0.0f;
		int32_t   sampler_index = 2;
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

	void                    check_format_features(vk::Format) const;
	vk::DescriptorPool      create_descriptor_pool();
	vk::DescriptorSetLayout create_descriptor_set_layout();
	vk::Pipeline            create_pipeline();
	void                    draw();
	void                    load_assets();
	void                    prepare_camera();
	void                    prepare_uniform_buffers();
	void                    update_descriptor_set();
	void                    update_uniform_buffers(float delta_time = 0.0f);

  private:
	vk::DescriptorSet       descriptor_set;
	vk::DescriptorSetLayout descriptor_set_layout;
	vk::Pipeline            pipeline;
	vk::PipelineLayout      pipeline_layout;
	bool                    rotate_scene = false;
	// To demonstrate mip mapping and filtering this example uses separate samplers
	std::vector<std::string>                                  sampler_names{"No mip maps", "Mip maps (bilinear)", "Mip maps (anisotropic)"};
	std::array<vk::Sampler, 3>                                samplers;
	std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> scene;
	Texture                                                   texture;
	UBO                                                       ubo;
	std::unique_ptr<vkb::core::HPPBuffer>                     uniform_buffer;
};

std::unique_ptr<vkb::Application> create_hpp_texture_mipmap_generation();
