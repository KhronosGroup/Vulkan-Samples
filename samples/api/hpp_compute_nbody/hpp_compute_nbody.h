/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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
 * Compute shader N-body simulation using two passes and shared compute shader memory, using vulkan.hpp
 */

#pragma once

#include "hpp_api_vulkan_sample.h"

#if defined(__ANDROID__)
// Lower particle count on Android for performance reasons
#	define PARTICLES_PER_ATTRACTOR 3 * 1024
#else
#	define PARTICLES_PER_ATTRACTOR 4 * 1024
#endif

class HPPComputeNBody : public HPPApiVulkanSample
{
  public:
	uint32_t num_particles;
	uint32_t work_group_size  = 128;
	uint32_t shared_data_size = 1024;

	struct
	{
		HPPTexture particle;
		HPPTexture gradient;
	} textures;

	// Resources for the graphics part of the example
	struct
	{
		std::unique_ptr<vkb::core::HPPBuffer> uniform_buffer;               // Contains scene matrices
		vk::DescriptorSetLayout               descriptor_set_layout;        // Particle system rendering shader binding layout
		vk::DescriptorSet                     descriptor_set;               // Particle system rendering shader bindings
		vk::PipelineLayout                    pipeline_layout;              // Layout of the graphics pipeline
		vk::Pipeline                          pipeline;                     // Particle rendering pipeline
		vk::Semaphore                         semaphore;                    // Execution dependency between compute & graphic submission
		uint32_t                              queue_family_index;
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
		std::unique_ptr<vkb::core::HPPBuffer> storage_buffer;               // (Shader) storage buffer object containing the particles
		std::unique_ptr<vkb::core::HPPBuffer> uniform_buffer;               // Uniform buffer object containing particle system parameters
		vk::Queue                             queue;                        // Separate queue for compute commands (queue family may differ from the one used for graphics)
		vk::CommandPool                       command_pool;                 // Use a separate command pool (queue family may differ from the one used for graphics)
		vk::CommandBuffer                     command_buffer;               // Command buffer storing the dispatch commands and barriers
		vk::Semaphore                         semaphore;                    // Execution dependency between compute & graphic submission
		vk::DescriptorSetLayout               descriptor_set_layout;        // Compute shader binding layout
		vk::DescriptorSet                     descriptor_set;               // Compute shader bindings
		vk::PipelineLayout                    pipeline_layout;              // Layout of the compute pipeline
		vk::Pipeline                          pipeline_calculate;           // Compute pipeline for N-Body velocity calculation (1st pass)
		vk::Pipeline                          pipeline_integrate;           // Compute pipeline for euler integration (2nd pass)
		vk::Pipeline                          blur;
		vk::PipelineLayout                    pipeline_layout_blur;
		vk::DescriptorSetLayout               descriptor_set_layout_blur;
		vk::DescriptorSet                     descriptor_set_blur;
		uint32_t                              queue_family_index;
		struct ComputeUBO
		{                              // Compute shader uniform block object
			float   delta_time;        // Frame delta time
			int32_t particle_count;
		} ubo;
	} compute;

	// SSBO particle declaration
	struct Particle
	{
		glm::vec4 pos;        // xyz = position, w = mass
		glm::vec4 vel;        // xyz = velocity, w = gradient texture position
	};

	HPPComputeNBody();
	~HPPComputeNBody();
	virtual void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;
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
	bool         prepare(vkb::platform::HPPPlatform &platform) override;
	virtual void render(float delta_time) override;
	virtual bool resize(const uint32_t width, const uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_hpp_compute_nbody();
