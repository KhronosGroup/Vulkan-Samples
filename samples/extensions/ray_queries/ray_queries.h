/* Copyright (c) 2021-2024, Holochip Corporation
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
 * Calculate shadows by extending a standard pipeline with ray queries
 */

#pragma once

#include "api_vulkan_sample.h"
#include <core/acceleration_structure.h>

namespace vkb
{
namespace sg
{
class Scene;
class Node;
class Mesh;
class SubMesh;
class Camera;
}        // namespace sg
}        // namespace vkb

class RayQueries : public ApiVulkanSample
{
  public:
	RayQueries();
	~RayQueries() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void render(float delta_time) override;
	bool prepare(const vkb::ApplicationOptions &options) override;

  private:
	struct GlobalUniform
	{
		glm::mat4x4 view;
		glm::mat4x4 proj;
		alignas(16) glm::vec3 camera_position;
		alignas(16) glm::vec3 light_position;
	} global_uniform;

	struct Vertex
	{
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 normal;
	};

	struct Model
	{
		std::vector<Vertex>                  vertices;
		std::vector<std::array<uint32_t, 3>> indices;
	} model;

	std::chrono::high_resolution_clock::time_point start_time{std::chrono::high_resolution_clock::now()};

	// Buffers
	std::unique_ptr<vkb::core::Buffer> vertex_buffer{nullptr};
	std::unique_ptr<vkb::core::Buffer> index_buffer{nullptr};
	std::unique_ptr<vkb::core::Buffer> uniform_buffer{nullptr};

	// Ray tracing structures
	VkPhysicalDeviceAccelerationStructureFeaturesKHR  acceleration_structure_features{};
	std::unique_ptr<vkb::core::AccelerationStructure> top_level_acceleration_structure    = nullptr;
	std::unique_ptr<vkb::core::AccelerationStructure> bottom_level_acceleration_structure = nullptr;
	uint64_t                                          get_buffer_device_address(VkBuffer buffer);
	void                                              create_top_level_acceleration_structure();
	void                                              create_bottom_level_acceleration_structure();

	VkPipeline            pipeline{VK_NULL_HANDLE};
	VkPipelineLayout      pipeline_layout{VK_NULL_HANDLE};
	VkDescriptorSet       descriptor_set{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};

	void build_command_buffers() override;
	void create_uniforms();
	void load_scene();
	void create_descriptor_pool();
	void create_descriptor_sets();
	void prepare_pipelines();
	void update_uniform_buffers();
	void draw();
};

std::unique_ptr<vkb::VulkanSampleC> create_ray_queries();
