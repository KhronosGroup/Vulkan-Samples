/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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
 * Dynamic terrain tessellation, using vulkan.hpp
 */

#pragma once

#include <hpp_api_vulkan_sample.h>

#include <geometry/frustum.h>

class HPPTerrainTessellation : public HPPApiVulkanSample
{
  public:
	HPPTerrainTessellation();
	~HPPTerrainTessellation();

  private:
	struct SkySphere
	{
		vk::DescriptorSet       descriptor_set;
		vk::DescriptorSetLayout descriptor_set_layout;
		vk::PipelineLayout      pipeline_layout;
		vk::Pipeline            pipeline;

		std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> geometry;

		HPPTexture texture;

		glm::mat4                             transform;
		std::unique_ptr<vkb::core::HPPBuffer> transform_buffer;

		void destroy(vk::Device device)
		{
			device.destroyPipeline(pipeline);
			device.destroyPipelineLayout(pipeline_layout);
			device.destroyDescriptorSetLayout(descriptor_set_layout);
			// the descriptor_set is implicitly freed by destroying its DescriptorPool
			device.destroySampler(texture.sampler);
		}
	};

	struct Statistics
	{
		bool query_supported = false;

		vk::QueryPool query_pool;

		std::array<uint64_t, 2> results = {{0}};

		void destroy(vk::Device device)
		{
			if (query_supported)
			{
				device.destroyQueryPool(query_pool);
			}
		}
	};

	// Shared values for tessellation control and evaluation stages
	struct Tessellation
	{
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::vec4 light_pos = glm::vec4(-48.0f, -40.0f, 46.0f, 0.0f);
		glm::vec4 frustum_planes[6];
		float     displacement_factor = 32.0f;
		float     tessellation_factor = 0.75f;
		glm::vec2 viewport_dim;
		// Desired size of tessellated quad patch edge
		float tessellated_edge_size = 20.0f;
	};

	struct Terrain
	{
		vk::DescriptorSet       descriptor_set;
		vk::DescriptorSetLayout descriptor_set_layout;
		vk::PipelineLayout      pipeline_layout;
		vk::Pipeline            pipeline;

		std::unique_ptr<vkb::core::HPPBuffer> vertices;
		std::unique_ptr<vkb::core::HPPBuffer> indices;
		uint32_t                              index_count;

		HPPTexture height_map;
		HPPTexture terrain_array;

		bool sampler_anisotropy_supported = false;

		std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;

		Tessellation                          tessellation;
		std::unique_ptr<vkb::core::HPPBuffer> tessellation_buffer;
		bool                                  tessellation_enabled = true;

		void destroy(vk::Device device)
		{
			device.destroyPipeline(pipeline);
			device.destroyPipelineLayout(pipeline_layout);
			device.destroyDescriptorSetLayout(descriptor_set_layout);
			// the descriptor_set is implicitly freed by destroying its DescriptorPool
			device.destroySampler(height_map.sampler);
			device.destroySampler(terrain_array.sampler);
		}
	};

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	struct Wireframe
	{
		bool supported = false;
		bool enabled   = false;

		vk::Pipeline pipeline;

		void destroy(vk::Device device)
		{
			device.destroyPipeline(pipeline);
		}
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
	vk::DescriptorSetLayout create_sky_sphere_descriptor_set_layout();
	vk::Pipeline            create_sky_sphere_pipeline();
	vk::DescriptorSetLayout create_terrain_descriptor_set_layout();
	vk::Pipeline            create_terrain_pipeline(vk::PolygonMode polygon_mode);
	void                    draw();
	void                    generate_terrain();
	void                    load_assets();
	void                    prepare_camera();
	void                    prepare_sky_sphere();
	void                    prepare_statistics();
	void                    prepare_terrain();
	void                    prepare_uniform_buffers();
	void                    prepare_wireframe();
	void                    update_uniform_buffers();
	void                    update_sky_sphere_descriptor_set();
	void                    update_terrain_descriptor_set();

  private:
	vkb::Frustum frustum;        // View frustum passed to tessellation control shader for culling
	SkySphere    sky_sphere;
	Statistics   statistics;
	Terrain      terrain;
	Wireframe    wireframe;
};

std::unique_ptr<vkb::Application> create_hpp_terrain_tessellation();
