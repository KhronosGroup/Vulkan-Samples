/* Copyright (c) 2019-2020, Sascha Willems
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
 * Dynamic terrain tessellation
 */

#pragma once

#include "api_vulkan_sample.h"
#include "core/buffer.h"
#include "geometry/frustum.h"

class TerrainTessellation : public ApiVulkanSample
{
  public:
	bool wireframe    = false;
	bool tessellation = true;

	struct
	{
		Texture heightmap;
		Texture skysphere;
		Texture terrain_array;
	} textures;

	std::unique_ptr<vkb::sg::SubMesh> skysphere;

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	struct Terrain
	{
		std::unique_ptr<vkb::core::Buffer> vertices;
		std::unique_ptr<vkb::core::Buffer> indices;
		uint32_t                           index_count;
	} terrain;

	struct
	{
		std::unique_ptr<vkb::core::Buffer> terrain_tessellation;
		std::unique_ptr<vkb::core::Buffer> skysphere_vertex;
	} uniform_buffers;

	// Shared values for tessellation control and evaluation stages
	struct
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
	} ubo_tess;

	// Skysphere vertex shader stage
	struct
	{
		glm::mat4 mvp;
	} ubo_vs;

	struct Pipelines
	{
		VkPipeline terrain;
		VkPipeline wireframe = VK_NULL_HANDLE;
		VkPipeline skysphere;
	} pipelines;

	struct
	{
		VkDescriptorSetLayout terrain;
		VkDescriptorSetLayout skysphere;
	} descriptor_set_layouts;

	struct
	{
		VkPipelineLayout terrain;
		VkPipelineLayout skysphere;
	} pipeline_layouts;

	struct
	{
		VkDescriptorSet terrain;
		VkDescriptorSet skysphere;
	} descriptor_sets;

	// Pipeline statistics
	struct
	{
		VkBuffer       buffer;
		VkDeviceMemory memory;
	} query_result;
	VkQueryPool query_pool        = VK_NULL_HANDLE;
	uint64_t    pipeline_stats[2] = {0};

	// View frustum passed to tessellation control shader for culling
	vkb::Frustum frustum;

	TerrainTessellation();
	~TerrainTessellation();
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         setup_query_result_buffer();
	void         get_query_results();
	void         load_assets();
	void         build_command_buffers() override;
	void         generate_terrain();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layouts();
	void         setup_descriptor_sets();
	void         prepare_pipelines();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	void         draw();
	bool         prepare(const vkb::ApplicationOptions &options) override;
	virtual void render(float delta_time) override;
	virtual void view_changed() override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
};

std::unique_ptr<vkb::Application> create_terrain_tessellation();
