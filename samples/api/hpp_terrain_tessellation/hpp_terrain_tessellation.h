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
	struct DescriptorSetLayouts
	{
		vk::DescriptorSetLayout terrain;
		vk::DescriptorSetLayout skysphere;
	};

	struct DescriptorSets
	{
		vk::DescriptorSet terrain;
		vk::DescriptorSet skysphere;
	};

	struct PipelineLayouts
	{
		vk::PipelineLayout terrain;
		vk::PipelineLayout skysphere;
	};

	struct Pipelines
	{
		vk::Pipeline terrain;
		vk::Pipeline wireframe = nullptr;
		vk::Pipeline skysphere;
	};

	// Pipeline statistics
	struct QueryResults
	{
		vk::Buffer       buffer;
		vk::DeviceMemory memory;
	};

	// Skysphere vertex shader stage
	struct SkySphereUBO
	{
		glm::mat4 mvp;
	};

	struct Textures
	{
		HPPTexture heightmap;
		HPPTexture skysphere;
		HPPTexture terrain_array;
	};

	struct Terrain
	{
		std::unique_ptr<vkb::core::HPPBuffer> vertices;
		std::unique_ptr<vkb::core::HPPBuffer> indices;
		uint32_t                              index_count;
	};

	// Shared values for tessellation control and evaluation stages
	struct TessellationUBO
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

	struct UniformBuffers
	{
		std::unique_ptr<vkb::core::HPPBuffer> terrain_tessellation;
		std::unique_ptr<vkb::core::HPPBuffer> skysphere_vertex;
	};

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
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

	void draw();
	void generate_terrain();
	void load_assets();
	void prepare_pipelines();
	void prepare_uniform_buffers();
	void setup_descriptor_pool();
	void setup_descriptor_set_layouts();
	void setup_descriptor_sets();
	void setup_query_result_buffer();
	void update_uniform_buffers();

  private:
	DescriptorSetLayouts                                      descriptor_set_layouts;
	DescriptorSets                                            descriptor_sets;
	vkb::Frustum                                              frustum;        // View frustum passed to tessellation control shader for culling
	PipelineLayouts                                           pipeline_layouts;
	std::array<uint64_t, 2>                                   pipeline_stats = {{0}};
	Pipelines                                                 pipelines;
	vk::QueryPool                                             query_pool = nullptr;
	QueryResults                                              query_result;
	std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> skysphere;
	bool                                                      tessellation = true;
	Textures                                                  textures;
	Terrain                                                   terrain;
	TessellationUBO                                           ubo_tess;
	SkySphereUBO                                              ubo_vs;
	UniformBuffers                                            uniform_buffers;
	bool                                                      wireframe = false;
};

std::unique_ptr<vkb::Application> create_hpp_terrain_tessellation();
