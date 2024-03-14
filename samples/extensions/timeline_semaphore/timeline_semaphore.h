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
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class TimelineSemaphore : public ApiVulkanSample
{
  public:
	TimelineSemaphore();
	~TimelineSemaphore();

  private:
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	virtual void render(float delta_time) override;
	virtual void build_command_buffers() override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
	virtual bool prepare(const vkb::ApplicationOptions &options) override;
	virtual void finish() override;

	void create_resources();
	void create_pipelines();
	void create_compute_pipeline();
	void create_graphics_pipeline();

	struct Pipelines
	{
		VkPipelineLayout compute_pipeline_layout{};
		VkPipelineLayout graphics_pipeline_layout{};
		VkPipeline       visualize_pipeline{};
		VkPipeline       compute_update_pipeline{};
		VkPipeline       compute_mutate_pipeline{};
		VkPipeline       compute_init_pipeline{};
	} pipelines;

	enum
	{
		NumAsyncFrames = 2
	};

	struct Descriptors
	{
		VkDescriptorSetLayout storage_layout;
		VkDescriptorSetLayout sampled_layout;
		VkDescriptorSet       storage_images[NumAsyncFrames];
		VkDescriptorSet       sampled_images[NumAsyncFrames];
		VkDescriptorPool      descriptor_pool;
	} descriptors{};

	std::unique_ptr<vkb::core::Sampler>   immutable_sampler;
	std::unique_ptr<vkb::core::Image>     images[NumAsyncFrames];
	std::unique_ptr<vkb::core::ImageView> image_views[NumAsyncFrames];

	VkQueue    async_queue{VK_NULL_HANDLE};
	void       prepare_queue();
	std::mutex submission_lock;

	struct Timeline
	{
		VkSemaphore semaphore;
		uint64_t    timeline;
	};
	Timeline main_thread_timeline{}, async_compute_timeline{};

	struct TimelineLock
	{
		std::condition_variable cond;
		std::mutex              lock;
		uint64_t                pending_timeline;
	};
	TimelineLock main_thread_timeline_lock{}, async_compute_timeline_lock{};

	struct TimelineWorker
	{
		std::thread      thread;
		std::atomic_bool alive;
	};
	TimelineWorker async_compute_worker;
	void           create_timeline_semaphores();
	void           create_timeline_semaphore(Timeline &timeline);

	void        create_timeline_workers();
	static void create_timeline_worker(TimelineWorker &worker, std::function<void()> thread_func);

	void async_compute_loop();

	void signal_timeline_gpu(VkQueue queue, const Timeline &timeline, TimelineLock &lock);
	void wait_timeline_gpu(VkQueue queue, const Timeline &timeline, TimelineLock &lock);
	void wait_timeline_cpu(const Timeline &timeline);
	void signal_timeline_cpu(const Timeline &timeline, TimelineLock &lock);
	void update_pending(TimelineLock &lock, uint64_t timeline);
	void wait_pending_in_order_queue(TimelineLock &lock, uint64_t timeline);
	void wait_pending(TimelineLock &lock, uint64_t timeline);
};

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_timeline_semaphore();
