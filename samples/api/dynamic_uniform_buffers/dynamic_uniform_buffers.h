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
 * Demonstrates the use of dynamic uniform buffers.
 *
 * Instead of using one uniform buffer per-object, this example allocates one big uniform buffer
 * with respect to the alignment reported by the device via minUniformBufferOffsetAlignment that
 * contains all matrices for the objects in the scene.
 *
 * The used descriptor type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC then allows to set a dynamic
 * offset used to pass data from the single uniform buffer to the connected shader binding point.
 */

#pragma once

#include "api_vulkan_sample.h"

#define OBJECT_INSTANCES 125

class DynamicUniformBuffers : public ApiVulkanSample
{
  private:
	static void *aligned_alloc(size_t size, size_t alignment);
	static void  aligned_free(void *data);

  public:
	struct Vertex
	{
		float pos[3];
		float color[3];
	};

	std::unique_ptr<vkb::core::Buffer> vertex_buffer;
	std::unique_ptr<vkb::core::Buffer> index_buffer;
	uint32_t                           index_count;

	struct UniformBuffers
	{
		std::unique_ptr<vkb::core::Buffer> view;
		std::unique_ptr<vkb::core::Buffer> dynamic;
	} uniform_buffers;

	struct UboVS
	{
		glm::mat4 projection;
		glm::mat4 view;
	} ubo_vs;

	// Store random per-object rotations
	glm::vec3 rotations[OBJECT_INSTANCES];
	glm::vec3 rotation_speeds[OBJECT_INSTANCES];

	// One big uniform buffer that contains all matrices
	// Note that we need to manually allocate the data to cope for GPU-specific uniform buffer offset alignments
	struct UboDataDynamic
	{
		glm::mat4 *model = nullptr;
	} ubo_data_dynamic;

	VkPipeline            pipeline;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;

	float animation_timer = 0.0f;

	size_t dynamic_alignment;

	DynamicUniformBuffers();
	~DynamicUniformBuffers();
	void         build_command_buffers() override;
	void         generate_cube();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_set();
	void         prepare_pipelines();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	void         update_dynamic_uniform_buffer(float delta_time, bool force = false);
	void         draw();
	bool         prepare(vkb::Platform &platform) override;
	virtual void render(float delta_time) override;
	virtual void resize(const uint32_t width, const uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_dynamic_uniform_buffers();
