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
 * Compute shader N-body simulation using two passes and shared compute shader memory, using vulkan.hpp
 */

#pragma once

#include <hpp_api_vulkan_sample.h>

#if defined(__ANDROID__)
// Lower particle count on Android for performance reasons
#	define PARTICLES_PER_ATTRACTOR 3 * 1024
#else
#	define PARTICLES_PER_ATTRACTOR 4 * 1024
#endif

class HPPComputeNBody : public HPPApiVulkanSample
{
  public:
	HPPComputeNBody();
	~HPPComputeNBody();

  private:
	// Resources for the compute part of the example
	struct ComputeUBO
	{                              // Compute shader uniform block object
		float   delta_time;        // Frame delta time
		int32_t particle_count;
	};

	struct Compute
	{
		vk::CommandBuffer                     command_buffer;               // Command buffer storing the dispatch commands and barriers
		vk::CommandPool                       command_pool;                 // Use a separate command pool (queue family may differ from the one used for graphics)
		vk::DescriptorSet                     descriptor_set;               // Compute shader bindings
		vk::DescriptorSetLayout               descriptor_set_layout;        // Compute shader binding layout
		vk::Pipeline                          pipeline_calculate;           // Compute pipeline for N-Body velocity calculation (1st pass)
		vk::Pipeline                          pipeline_integrate;           // Compute pipeline for euler integration (2nd pass)
		vk::PipelineLayout                    pipeline_layout;              // Layout of the compute pipeline
		vk::Queue                             queue;                        // Separate queue for compute commands (queue family may differ from the one used for graphics)
		uint32_t                              queue_family_index = ~0;
		vk::Semaphore                         semaphore;        // Execution dependency between compute & graphic submission
		uint32_t                              shared_data_size = 1024;
		std::unique_ptr<vkb::core::HPPBuffer> storage_buffer;        // (Shader) storage buffer object containing the particles
		ComputeUBO                            ubo;
		std::unique_ptr<vkb::core::HPPBuffer> uniform_buffer;        // Uniform buffer object containing particle system parameters
		uint32_t                              work_group_size = 128;

		void destroy(vk::Device device)
		{
			storage_buffer.reset();
			uniform_buffer.reset();
			device.destroyPipeline(pipeline_calculate);
			device.destroyPipeline(pipeline_integrate);
			device.destroyPipelineLayout(pipeline_layout);
			// no need to free the descriptor_set, as it's implicitly free'd with the descriptor_pool
			device.destroyDescriptorSetLayout(descriptor_set_layout);
			device.destroySemaphore(semaphore);
			device.freeCommandBuffers(command_pool, command_buffer);
			device.destroyCommandPool(command_pool);
		}
	};

	// Resources for the graphics part of the example
	struct GraphicsUBO
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec2 screenDim;
	};

	struct Graphics
	{
		vk::DescriptorSet                     descriptor_set;               // Particle system rendering shader bindings
		vk::DescriptorSetLayout               descriptor_set_layout;        // Particle system rendering shader binding layout
		vk::Pipeline                          pipeline;                     // Particle rendering pipeline
		vk::PipelineLayout                    pipeline_layout;              // Layout of the graphics pipeline
		uint32_t                              queue_family_index = ~0;
		vk::Semaphore                         semaphore;        // Execution dependency between compute & graphic submission
		GraphicsUBO                           ubo;
		std::unique_ptr<vkb::core::HPPBuffer> uniform_buffer;        // Contains scene matrices

		void destroy(vk::Device device)
		{
			uniform_buffer.reset();
			device.destroyPipeline(pipeline);
			device.destroyPipelineLayout(pipeline_layout);
			// no need to free the descriptor_set, as it's implicitly free'd with the descriptor_pool
			device.destroyDescriptorSetLayout(descriptor_set_layout);
			device.destroySemaphore(semaphore);
		}
	};

	// SSBO particle declaration
	struct Particle
	{
		glm::vec4 pos;        // xyz = position, w = mass
		glm::vec4 vel;        // xyz = velocity, w = gradient texture position
	};

	struct Textures
	{
		HPPTexture gradient;
		HPPTexture particle;

		void destroy(vk::Device device)
		{
			device.destroySampler(particle.sampler);
			device.destroySampler(gradient.sampler);
		}
	};

  private:
	bool prepare(const vkb::ApplicationOptions &options) override;
	bool resize(const uint32_t width, const uint32_t height) override;

	// from HPPVulkanSample
	void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;

	// from HPPApiVulkanSample
	void build_command_buffers() override;
	void render(float delta_time) override;

	void         build_compute_command_buffer();
	void         build_compute_transfer_command_buffer(vk::CommandBuffer command_buffer) const;
	void         build_copy_command_buffer(vk::CommandBuffer command_buffer, vk::Buffer staging_buffer, vk::DeviceSize buffer_size) const;
	vk::Pipeline create_compute_pipeline(vk::PipelineCache pipeline_cache, vk::PipelineShaderStageCreateInfo const &stage, vk::PipelineLayout layout);
	void         draw();
	void         initializeCamera();
	void         load_assets();
	void         prepare_compute();
	void         prepare_compute_storage_buffers();
	void         prepare_graphics();
	void         update_compute_descriptor_set();
	void         update_compute_uniform_buffers(float delta_time);
	void         update_graphics_descriptor_set();
	void         update_graphics_uniform_buffers();

  private:
	Compute  compute;
	Graphics graphics;
	Textures textures;
};

std::unique_ptr<vkb::Application> create_hpp_compute_nbody();
