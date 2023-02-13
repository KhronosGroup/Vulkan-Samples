/* Copyright (c) 2023, Holochip Corporation
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
 * Demonstrate and showcase a sample application using mesh shader rendering pipeline.
 */

#pragma once

#include "api_vulkan_sample.h"

#define OBJECT_INSTANCES 125

// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void *aligned_alloc(size_t size, size_t alignment);
void  aligned_free(void *data);

class MeshShader : public ApiVulkanSample
{
  public:
	struct Vertex
	{
		float pos[3]{};
		float color[3]{};
	};

	struct UniformBuffers
	{
		std::unique_ptr<vkb::core::Buffer> view{};
		std::unique_ptr<vkb::core::Buffer> dynamic{};
	} uniform_buffers{};

	struct UboVS
	{
		glm::mat4 projection{};
		glm::mat4 view{};
	} ubo_vs{};

	// One big uniform buffer that contains all matrices
	// Note that we need to manually allocate the data to cope for GPU-specific uniform buffer offset alignments
	struct UboDataDynamic
	{
		glm::mat4 *model = nullptr;
	} ubo_data_dynamic{};

	// Mesh shader selection
	bool is_mesh_shader = true;

	// Meshlet related structures
	struct MeshletInfo
	{
		uint32_t vertex_count{};
		uint32_t vertex_begin_index{};
		uint32_t primitive_count{};
		uint32_t primitive_begin_index{};
	};

	VkPipeline            pipeline              = VK_NULL_HANDLE;
	VkPipelineLayout      pipeline_layout       = VK_NULL_HANDLE;
	VkDescriptorSet       descriptor_set        = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;

	// Cube vertices and indices
	std::vector<Vertex>   cube_vertices{};
	std::vector<uint32_t> cube_indices{};

	// Vertex information
	std::unique_ptr<vkb::core::Buffer> vertex_buffer{};
	std::unique_ptr<vkb::core::Buffer> index_buffer{};
	uint32_t                           index_count = 0;

	// Basic information passing to a mesh shader
	std::vector<uint32_t>    meshlet_vertex_indices{};           // this provides information to mesh shader: how to select vertices from
	std::vector<MeshletInfo> meshlet_infos{};                    // this provides information to mesh shader: how to determine and access each vertex-index group
	std::vector<uint8_t>     meshlet_primitive_indices{};        // this provides information to mesh shader: how to form triangles from selected vertices

	std::unique_ptr<vkb::core::Buffer> meshlet_vertex_array_buffer{};        // this buffer stores total vertex information
	std::unique_ptr<vkb::core::Buffer> meshlet_vertex_index_buffer{};        // this buffer stores information how to select vertex from total vertex information

	std::unique_ptr<vkb::core::Buffer> meshlet_info_buffer{};                   // this buffer stores information how to interpolate vertex index information
	std::unique_ptr<vkb::core::Buffer> meshlet_primitive_index_buffer{};        // this buffer stores information how to create triangles

	// Store random per-object rotations
	glm::vec3 rotations[OBJECT_INSTANCES]{};
	glm::vec3 rotation_speeds[OBJECT_INSTANCES]{};

	float  animation_timer   = 0.0f;
	size_t dynamic_alignment = 0;

  private:
	void init_cube();
	void init_cube_meshlets();

  public:
	MeshShader();
	~MeshShader() override;
	void build_command_buffers() override;
	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	void setup_descriptor_set();
	void prepare_buffers();
	void prepare_pipelines();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void update_dynamic_uniform_buffer(float delta_time, bool force = false);
	void draw();
	bool prepare(vkb::Platform &platform) override;
	void render(float delta_time) override;
	bool resize(uint32_t width, uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_mesh_shader();