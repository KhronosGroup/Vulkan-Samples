/* Copyright (c) 2021-2024, Arm Limited and Contributors
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

#pragma once

#include "api_vulkan_sample.h"

class TimelineSemaphore : public ApiVulkanSample
{
  public:
	static const uint32_t NumAsyncFrames = 2;

	// Resources for the graphics worker
	struct GraphicsResources
	{
		VkQueue         queue;
		VkCommandPool   command_pool;
		VkCommandBuffer command_buffer;

		VkPipelineLayout pipeline_layout;
		VkPipeline       pipeline;

		uint32_t queue_family_index;
	} graphics;

	// Resources for the compute worker
	struct ComputeResources
	{
		VkQueue         queue;
		VkCommandPool   command_pool;
		VkCommandBuffer command_buffer;

		VkPipelineLayout pipeline_layout;
		VkPipeline       init_pipeline;
		VkPipeline       update_pipeline;
		VkPipeline       mutate_pipeline;

		vkb::Timer timer;
		uint32_t   queue_family_index;
	} compute;

	// Resources used by both workers for storing/sampling images
	struct SharedResources
	{
		VkDescriptorSetLayout storage_layout;
		VkDescriptorSetLayout sampled_layout;
		VkDescriptorSet       storage_descriptor_sets[NumAsyncFrames];
		VkDescriptorSet       sampled_descriptor_sets[NumAsyncFrames];
		VkDescriptorPool      descriptor_pool;

		std::unique_ptr<vkb::core::Sampler>   immutable_sampler;
		std::unique_ptr<vkb::core::Image>     images[NumAsyncFrames];
		std::unique_ptr<vkb::core::ImageView> image_views[NumAsyncFrames];
	} shared;

	struct Timeline
	{
		// The stages of the timeline are enumerated, to make it easier to read which stage we are signalling/waiting on, and to allow
		// the stages to be reused without needing to recreate the semaphore.
		enum Stages
		{
			submit = 1,        // Worker threads can create and submit their command buffers,
			draw,              // The graphics worker can draw the current frame
			present,           // The main thread can present the frame to the display
			MAX_STAGES
		};

		VkSemaphore semaphore;
		uint64_t    frame;        // Number of iterations through the timeline stages
	} timeline;

	struct TimelineWorker
	{
		std::thread      thread;
		std::atomic_bool alive;
	};

	TimelineWorker graphics_worker, compute_worker;

	TimelineSemaphore();
	~TimelineSemaphore();

	void setup_shared_resources();
	void build_command_buffers() override;

	// Timeline operations
	void     create_timeline_semaphore();
	void     start_timeline_workers();
	void     finish_timeline_workers();
	void     signal_timeline(const Timeline::Stages stage);
	void     wait_on_timeline(const Timeline::Stages stage);
	void     signal_next_frame();
	void     wait_for_next_frame();
	uint64_t get_timeline_stage_value(const Timeline::Stages stage);

	// Compute Work
	void do_compute_work();
	void setup_compute_pipeline();
	void setup_compute_resources();
	void setup_game_of_life();
	void build_compute_command_buffers(const float elapsed = 0.0f);

	// Graphics Work
	void do_graphics_work();
	void setup_graphics_resources();
	void setup_graphics_pipeline();
	void build_graphics_command_buffer();

	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	virtual bool prepare(const vkb::ApplicationOptions &options) override;
	virtual void render(float delta_time) override;
};

std::unique_ptr<vkb::Application> create_timeline_semaphore();
