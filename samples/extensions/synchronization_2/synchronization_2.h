/* Copyright (c) 2021, Sascha Willems
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
 * Doing compute and graphics queue synchronization using VK_KHR_synchronization2
 * Synchronization examples using this extension can be found at https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
 */

#pragma once

#include "api_vulkan_sample.h"

#if defined(__ANDROID__)
// Lower particle count on Android for performance reasons
#	define PARTICLES_PER_ATTRACTOR 3 * 1024
#else
#	define PARTICLES_PER_ATTRACTOR 4 * 1024
#endif

class Synchronization2 : public ApiVulkanSample
{
  public:
	uint32_t num_particles;
	uint32_t work_group_size  = 128;
	uint32_t shared_data_size = 1024;

	struct
	{
		Texture particle;
		Texture gradient;
	} textures;

	// Resources for the graphics part of the example
	struct
	{
		std::unique_ptr<vkb::core::Buffer> uniform_buffer;               // Contains scene matrices
		VkDescriptorSetLayout              descriptor_set_layout;        // Particle system rendering shader binding layout
		VkDescriptorSet                    descriptor_set;               // Particle system rendering shader bindings
		VkPipelineLayout                   pipeline_layout;              // Layout of the graphics pipeline
		VkPipeline                         pipeline;                     // Particle rendering pipeline
		VkSemaphore                        semaphore;                    // Execution dependency between compute & graphic submission
		uint32_t                           queue_family_index;
		struct
		{
			glm::mat4 projection;
			glm::mat4 view;
			glm::vec2 screenDim;
		} ubo;
	} graphics;

	// Resources for the compute part of the example
	struct
	{
		std::unique_ptr<vkb::core::Buffer> storage_buffer;               // (Shader) storage buffer object containing the particles
		std::unique_ptr<vkb::core::Buffer> uniform_buffer;               // Uniform buffer object containing particle system parameters
		VkQueue                            queue;                        // Separate queue for compute commands (queue family may differ from the one used for graphics)
		VkCommandPool                      command_pool;                 // Use a separate command pool (queue family may differ from the one used for graphics)
		VkCommandBuffer                    command_buffer;               // Command buffer storing the dispatch commands and barriers
		VkSemaphore                        semaphore;                    // Execution dependency between compute & graphic submission
		VkDescriptorSetLayout              descriptor_set_layout;        // Compute shader binding layout
		VkDescriptorSet                    descriptor_set;               // Compute shader bindings
		VkPipelineLayout                   pipeline_layout;              // Layout of the compute pipeline
		VkPipeline                         pipeline_calculate;           // Compute pipeline for N-Body velocity calculation (1st pass)
		VkPipeline                         pipeline_integrate;           // Compute pipeline for euler integration (2nd pass)
		VkPipeline                         blur;
		VkPipelineLayout                   pipeline_layout_blur;
		VkDescriptorSetLayout              descriptor_set_layout_blur;
		VkDescriptorSet                    descriptor_set_blur;
		uint32_t                           queue_family_index;
		struct ComputeUBO
		{                              // Compute shader uniform block object
			float   delta_time;        //		Frame delta time
			int32_t particle_count;
		} ubo;
	} compute;

	// SSBO particle declaration
	struct Particle
	{
		glm::vec4 pos;        // xyz = position, w = mass
		glm::vec4 vel;        // xyz = velocity, w = gradient texture position
	};

	Synchronization2();
	~Synchronization2();
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         load_assets();
	void         build_command_buffers() override;
	void         build_compute_command_buffer();
	void         prepare_storage_buffers();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_set();
	void         prepare_pipelines();
	void         prepare_graphics();
	void         prepare_compute();
	void         prepare_uniform_buffers();
	void         update_compute_uniform_buffers(float delta_time);
	void         update_graphics_uniform_buffers();
	void         draw();
	bool         prepare(vkb::Platform &platform) override;
	virtual void render(float delta_time) override;
	virtual void resize(const uint32_t width, const uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_synchronization_2();
