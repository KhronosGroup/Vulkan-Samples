/* Copyright (c) 2021-2023, Arm Limited and Contributors
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

#include "timeline_semaphore.h"

static constexpr unsigned grid_width  = 64;
static constexpr unsigned grid_height = 64;

// A simple variant of std::lock_guard which takes a condition for when to lock.
// We need this since we will only lock vkQueueSubmit when the worker thread needs to submit to the main thread as well.
// Otherwise, we will submit lock-free since we only need to externally synchronize the same VkQueue.
class ConditionalLockGuard
{
  public:
	ConditionalLockGuard(std::mutex &lock_, bool cond_) :
	    lock(lock_), cond(cond_)
	{
		if (cond)
		{
			lock.lock();
		}
	}

	~ConditionalLockGuard()
	{
		if (cond)
		{
			lock.unlock();
		}
	}

  private:
	std::mutex &lock;
	bool        cond;
};

TimelineSemaphore::TimelineSemaphore()
{
	title = "Timeline semaphore";

	// Need to enable timeline semaphore extension.
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
}

TimelineSemaphore::~TimelineSemaphore()
{
	if (device)
	{
		VkDevice vk_device = get_device().get_handle();
		vkDestroyPipelineLayout(vk_device, pipelines.compute_pipeline_layout, nullptr);
		vkDestroyPipelineLayout(vk_device, pipelines.graphics_pipeline_layout, nullptr);
		vkDestroyPipeline(vk_device, pipelines.visualize_pipeline, nullptr);
		vkDestroyPipeline(vk_device, pipelines.compute_update_pipeline, nullptr);
		vkDestroyPipeline(vk_device, pipelines.compute_mutate_pipeline, nullptr);
		vkDestroyPipeline(vk_device, pipelines.compute_init_pipeline, nullptr);
		vkDestroyDescriptorSetLayout(vk_device, descriptors.sampled_layout, nullptr);
		vkDestroyDescriptorSetLayout(vk_device, descriptors.storage_layout, nullptr);
		vkDestroyDescriptorPool(vk_device, descriptors.descriptor_pool, nullptr);

		vkDestroySemaphore(vk_device, main_thread_timeline.semaphore, nullptr);
		vkDestroySemaphore(vk_device, async_compute_timeline.semaphore, nullptr);
	}
}

void TimelineSemaphore::build_command_buffers()
{
}

void TimelineSemaphore::on_update_ui_overlay(vkb::Drawer &)
{
}

void TimelineSemaphore::finish()
{
	if (!device)
	{
		return;
	}

	// Draining queues which submit out-of-order can be quite tricky, since QueueWaitIdle can deadlock for threads which want to run ahead.
	// If we call Submit waiting for a semaphore which is yet to be signalled,
	// QueueWaitIdle will not finish until a signal in another thread happens.
	// Here's an approach we can use to safely tear down the queue.

	// Drain the main thread timeline.
	// The async queue might be stalled waiting on the main queue to finish rendering a future frame which it never completes,
	// but we might never hit that count, since we're tearing down the application now.
	wait_timeline_cpu(main_thread_timeline);

	// Now we're guaranteed that the graphics timeline is at N and the async compute queue is blocked at N + num_frames + 1, waiting for N + 1 to finish.
	// Since we're not reading any more in graphics queue, we can bump the timeline on CPU towards infinity.
	// On the next loop iteration, we will exit the rendering loop and QueueWaitIdle will not be blocked on async thread anymore.
	// Just bump the timeline by INT32_MAX which is min-spec for maxTimelineSemaphoreValueDifference.
	// This is a useful way to mark a timeline semaphore as "permanently" signalled.
	main_thread_timeline.timeline += std::numeric_limits<int32_t>::max();

	// Order matters here, this works kinda like a condition variable.
	// If the timeline update is observed, we should see that the worker is not alive anymore.
	async_compute_worker.alive = false;
	signal_timeline_cpu(main_thread_timeline, main_thread_timeline_lock);

	// This will now complete in finite time.
	if (async_compute_worker.thread.joinable())
	{
		async_compute_worker.thread.join();
	}
}

void TimelineSemaphore::create_timeline_semaphore(Timeline &timeline)
{
	// A timeline semaphore is still a semaphore, but it is of TIMELINE type rather than BINARY.
	VkSemaphoreCreateInfo        create_info = vkb::initializers::semaphore_create_info();
	VkSemaphoreTypeCreateInfoKHR type_create_info{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR};

	type_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;
	type_create_info.initialValue  = 0;
	create_info.pNext              = &type_create_info;

	VK_CHECK(vkCreateSemaphore(get_device().get_handle(), &create_info, nullptr, &timeline.semaphore));

	timeline.timeline = 0;
}

void TimelineSemaphore::create_timeline_semaphores()
{
	create_timeline_semaphore(main_thread_timeline);
	create_timeline_semaphore(async_compute_timeline);
}

void TimelineSemaphore::create_timeline_worker(TimelineWorker &worker, std::function<void()> thread_func)
{
	worker.alive  = true;
	worker.thread = std::thread(std::move(thread_func));
}

// Normally, signal and wait would be merged into a single submit info,
// but this would have made the sample a bit harder to read and reason about.
// For this reason, we split up signals, waits and executions.
void TimelineSemaphore::signal_timeline_gpu(VkQueue signal_queue, const Timeline &timeline, TimelineLock &lock)
{
	VkSubmitInfo submit         = vkb::initializers::submit_info();
	submit.pSignalSemaphores    = &timeline.semaphore;
	submit.signalSemaphoreCount = 1;

	// When N semaphores are provided and at least one of them is a timeline semaphore,
	// we must pass an auxillary pNext struct which provides which timeline values to use.
	VkTimelineSemaphoreSubmitInfoKHR timeline_info{VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR};
	timeline_info.signalSemaphoreValueCount = 1;
	timeline_info.pSignalSemaphoreValues    = &timeline.timeline;

	submit.pNext = &timeline_info;

	// VkQueue needs to be externally synchronized in vkQueueSubmit if async_queue == queue.
	{
		ConditionalLockGuard holder{submission_lock, async_queue == queue};
		VK_CHECK(vkQueueSubmit(signal_queue, 1, &submit, VK_NULL_HANDLE));
	}

	// This is a special case to handle a scenario where async_queue == queue as well.
	// Out-of-order submit is not possible with a single queue since the queue will deadlock itself.
	// Very few implementations only support one queue, but the sample should run on all implementations.
	// We also need this to handle the fact that we currently cannot use out-of-order submissions with swapchain.
	update_pending(lock, timeline.timeline);
}

void TimelineSemaphore::wait_timeline_gpu(VkQueue wait_queue, const Timeline &timeline, TimelineLock &lock)
{
	if (timeline.timeline == 0)
	{
		// No-op.
		return;
	}

	// This is a special case to handle a scenario where async_queue == queue as well.
	// Out-of-order submit is not possible with a single queue since the queue will deadlock itself.
	// Very few implementations only support one queue, but the sample should run on all implementations.
	wait_pending_in_order_queue(lock, timeline.timeline);

	const VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	VkSubmitInfo submit       = vkb::initializers::submit_info();
	submit.pWaitSemaphores    = &timeline.semaphore;
	submit.pWaitDstStageMask  = &wait_stages;
	submit.waitSemaphoreCount = 1;

	VkTimelineSemaphoreSubmitInfoKHR timeline_info{VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR};
	timeline_info.waitSemaphoreValueCount = 1;
	timeline_info.pWaitSemaphoreValues    = &timeline.timeline;

	submit.pNext = &timeline_info;

	// VkQueue needs to be externally synchronized in vkQueueSubmit if async_queue == queue.
	{
		ConditionalLockGuard holder{submission_lock, async_queue == queue};
		VK_CHECK(vkQueueSubmit(wait_queue, 1, &submit, VK_NULL_HANDLE));
	}
}

void TimelineSemaphore::wait_timeline_cpu(const Timeline &timeline)
{
	// There is no distinction between fences and semaphores anymore.
	// We can freely wait for a timeline semaphore on host.
	// There is also no external synchronization requirement like with VkFence!
	// This allows for a free flowing synchronization implementation which makes multithreading even nicer.

	VkSemaphoreWaitInfoKHR wait_info{VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO_KHR};
	wait_info.pSemaphores    = &timeline.semaphore;
	wait_info.semaphoreCount = 1;
	wait_info.pValues        = &timeline.timeline;
	VK_CHECK(vkWaitSemaphoresKHR(device->get_handle(), &wait_info, UINT64_MAX));
}

void TimelineSemaphore::signal_timeline_cpu(const Timeline &timeline, TimelineLock &lock)
{
	VkSemaphoreSignalInfoKHR signal_info{VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO_KHR};
	signal_info.semaphore = timeline.semaphore;
	signal_info.value     = timeline.timeline;
	VK_CHECK(vkSignalSemaphoreKHR(device->get_handle(), &signal_info));

	// This is a special case to handle a scenario where async_queue == queue as well.
	// Out-of-order submit is not possible with a single queue since the queue will deadlock itself.
	// Very few implementations only support one queue, but the sample should run on all implementations.
	update_pending(lock, timeline.timeline);
}

void TimelineSemaphore::update_pending(TimelineLock &lock, uint64_t timeline)
{
	// To support out-of-order signal and wait with a single queue we must do some workarounds.
	// Normally, an application should not bother with multiple async queues
	// if they have to be hammered onto one VkQueue in the end,
	// but it can be useful to know about these problem scenarios up front.
	//
	// The other case where we need to ensure some kind of ordering is when waiting on a binary semaphore.
	// Binary semaphores still have the requirement that all dependencies must already have been submitted,
	// and we must still use binary semaphores for swapchain.
	//
	// To make the single queue scenario work, we must be able to guarantee that a wait is submitted after a signal,
	// since we cannot signal on a queue once it is blocked by a wait.
	// The only way to do this is to hold back submissions and ensure submissions happen in a forward-progress order.
	//
	// In this sample, we can achieve this with a condition variable where we wait until
	// a pending signal has been submitted, but this approach does not work in all cases.
	// It works here since we have a dedicated submission thread.
	// It is always possible to add submission threads which may or may not be practical.
	//
	// This is called after signalling the timeline, which lets other submission threads know that it is safe to wait on
	// any timeline value that is <= pending_timeline.
	std::lock_guard<std::mutex> holder{lock.lock};
	lock.pending_timeline = timeline;
	lock.cond.notify_one();
}

void TimelineSemaphore::wait_pending(TimelineLock &lock, uint64_t timeline)
{
	// See update_pending(). This is called before submitting a wait to the single VkQueue.
	std::unique_lock<std::mutex> holder{lock.lock};
	lock.cond.wait(holder, [&lock, timeline]() -> bool {
		return lock.pending_timeline >= timeline;
	});
}

void TimelineSemaphore::wait_pending_in_order_queue(TimelineLock &lock, uint64_t timeline)
{
	if (async_queue == queue)
	{
		wait_pending(lock, timeline);
	}
}

// We want to achieve a pipeline where we're doing these in a double-buffered fashion:
// - Do async compute work, write buffer frame % 2, read buffer (frame - 1) % 2.
// - Blit results in main thread, read buffer frame % 2, write swapchain.

// What we're trying to demonstrate here is:
// - Out-of-order submission using threads which synchronize GPU work with each other using timeline semaphores.
//   In this sample we have a dedicated worker thread which submits work to async compute,
//   and the only synchronization with main thread happens via timeline semaphores.
// - Waiting for timeline semaphore on CPU to replace redundant fence objects.
// - Multiple waits on the same timeline. We don't need to worry about allocating and managing binary semaphores in complex scenarios.
//   We can wait on the same timeline values as many times as we want, and we avoid all resource management problems that binary semaphores have.

void TimelineSemaphore::async_compute_loop()
{
	uint64_t iteration = 0;

	vkb::Timer timer;
	timer.start();

	// We're going to be recording commands on a thread, so make sure we have our own command pool.
	VkCommandPool pool = device->create_command_pool(device->get_queue_family_index(VK_QUEUE_COMPUTE_BIT),
	                                                 VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	// Pre-allocate N command buffers. We will however re-record them every iteration.
	VkCommandBufferAllocateInfo alloc_info =
	    vkb::initializers::command_buffer_allocate_info(pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, NumAsyncFrames);
	VkCommandBuffer cmds[NumAsyncFrames];
	VK_CHECK(vkAllocateCommandBuffers(device->get_handle(), &alloc_info, cmds));

	while (async_compute_worker.alive)
	{
		iteration++;
		unsigned        frame_index = iteration % NumAsyncFrames;
		VkCommandBuffer cmd         = cmds[frame_index];

		if (iteration >= NumAsyncFrames)
		{
			// Wait for main thread to be done reading from the buffer, before we clobber it.
			wait_timeline_gpu(async_queue, {main_thread_timeline.semaphore, iteration - NumAsyncFrames}, main_thread_timeline_lock);

			// We're going to re-record command buffers, wait on host here. This also ensures we don't endlessly submit commands to the async queues.
			// The signalling of async compute timeline is gated somewhat on the main thread submitting work to the swapchain.
			wait_timeline_cpu({async_compute_timeline.semaphore, iteration - NumAsyncFrames});
		}

		// Wait for last iteration to complete since we're going to read from the results.
		// Could use pipeline barrier here certainly, but this is a sample
		// where we can show how free-flowing queue synchronization can be.
		wait_timeline_gpu(async_queue, async_compute_timeline, async_compute_timeline_lock);

		auto begin_info  = vkb::initializers::command_buffer_begin_info();
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CHECK(vkResetCommandBuffer(cmd, 0));
		VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute_pipeline_layout, 0, 1,
		                        &descriptors.storage_images[frame_index],
		                        0, nullptr);

		if (iteration == 1)
		{
			// On the first iteration, we initialize the game of life.
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute_init_pipeline);
		}
		else
		{
			auto elapsed = static_cast<float>(timer.elapsed());

			// Either we iterate the game every second, or we mutate it by changing colors gradually
			// to make something more aesthetically interesting.
			if (elapsed > 1.0f)
			{
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute_update_pipeline);
				timer.lap();
			}
			else
			{
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute_mutate_pipeline);
				vkCmdPushConstants(cmd, pipelines.compute_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT,
				                   0, sizeof(elapsed), &elapsed);
			}

			// Bind previous iteration's texture.
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute_pipeline_layout, 1, 1,
			                        &descriptors.sampled_images[(frame_index + (NumAsyncFrames - 1)) % NumAsyncFrames],
			                        0, nullptr);
		}

		VkImageMemoryBarrier image_barrier = vkb::initializers::image_memory_barrier();
		image_barrier.srcAccessMask        = 0;
		image_barrier.dstAccessMask        = VK_ACCESS_SHADER_WRITE_BIT;
		image_barrier.image                = images[frame_index]->get_handle();
		image_barrier.subresourceRange     = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		image_barrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
		image_barrier.newLayout            = VK_IMAGE_LAYOUT_GENERAL;

		// The semaphore takes care of srcStageMask.
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		                     0, 0, nullptr, 0, nullptr, 1, &image_barrier);

		vkCmdDispatch(cmd, grid_width / 8, grid_height / 8, 1);

		image_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		image_barrier.dstAccessMask = 0;
		image_barrier.oldLayout     = VK_IMAGE_LAYOUT_GENERAL;
		image_barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// The semaphore takes care of dstStageMask.
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		                     0, 0, nullptr, 0, nullptr, 1, &image_barrier);

		VK_CHECK(vkEndCommandBuffer(cmd));

		auto submit_info               = vkb::initializers::submit_info();
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers    = &cmd;

		// VkQueue needs to be externally synchronized in vkQueueSubmit if async_queue == queue.
		{
			ConditionalLockGuard holder{submission_lock, async_queue == queue};
			VK_CHECK(vkQueueSubmit(async_queue, 1, &submit_info, VK_NULL_HANDLE));
		}

		// Kicks shading work in main queue.
		async_compute_timeline.timeline = iteration;
		signal_timeline_gpu(async_queue, async_compute_timeline, async_compute_timeline_lock);
	}

	// This QueueWaitIdle can be precarious.
	// See TimelineSemaphore::finish() comments for why this is the case.
	{
		ConditionalLockGuard holder{submission_lock, async_queue == queue};
		vkQueueWaitIdle(async_queue);
	}

	// This also frees command buffers allocated from the pool.
	vkDestroyCommandPool(device->get_handle(), pool, nullptr);
}

void TimelineSemaphore::create_timeline_workers()
{
	create_timeline_worker(async_compute_worker, [this]() { async_compute_loop(); });
}

void TimelineSemaphore::prepare_queue()
{
	// Attempt to find a queue which is async compute.
	// If we cannot find that queue family, at least try to find a queue which is not the "main" queue.
	// If we have different queues we can safely use out of order signal and wait which is a core part of this sample.

	auto    &device       = get_device();
	uint32_t family_index = device.get_queue_family_index(VK_QUEUE_COMPUTE_BIT);
	uint32_t num_queues   = device.get_num_queues_for_queue_family(family_index);

	for (uint32_t i = 0; i < num_queues; i++)
	{
		auto &candidate = device.get_queue(family_index, i);
		if (candidate.get_handle() != queue)
		{
			async_queue = candidate.get_handle();
			break;
		}
	}

	if (!async_queue)
	{
		// Fallback path. Cannot use out-of-order signal and wait here since the queue will deadlock itself.
		// If this happens we need to add some locks and condition variables to make things work.
		// See comments in TimelineSemaphore::update_pending().
		async_queue = queue;
	}
}

void TimelineSemaphore::create_resources()
{
	uint32_t queue_families[2]{};
	uint32_t num_queue_families{};

	// Need CONCURRENT usage here since we will sample from the image
	// in both graphics and compute queues.
	if (device->get_queue_family_index(VK_QUEUE_COMPUTE_BIT) !=
	    device->get_queue_by_present(0).get_family_index())
	{
		queue_families[0]  = device->get_queue_by_present(0).get_family_index();
		queue_families[1]  = device->get_queue_family_index(VK_QUEUE_COMPUTE_BIT);
		num_queue_families = 2;
	}

	for (int i = 0; i < NumAsyncFrames; i++)
	{
		images[i] = std::make_unique<vkb::core::Image>(*device, VkExtent3D{grid_width, grid_height, 1},
		                                               VK_FORMAT_R8G8B8A8_UNORM,
		                                               VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		                                               VMA_MEMORY_USAGE_GPU_ONLY,
		                                               VK_SAMPLE_COUNT_1_BIT,
		                                               1, 1, VK_IMAGE_TILING_OPTIMAL,
		                                               0, num_queue_families, queue_families);

		image_views[i] = std::make_unique<vkb::core::ImageView>(*images[i], VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM);
	}

	// Boilerplate where we create a STORAGE_IMAGE descriptor set and SAMPLED_IMAGE descriptor set.

	auto sampler_create_info         = vkb::initializers::sampler_create_info();
	sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.minFilter    = VK_FILTER_NEAREST;
	sampler_create_info.magFilter    = VK_FILTER_NEAREST;
	sampler_create_info.maxLod       = VK_LOD_CLAMP_NONE;
	sampler_create_info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	immutable_sampler                = std::make_unique<vkb::core::Sampler>(*device, sampler_create_info);
	VkSampler vk_immutable_sampler   = immutable_sampler->get_handle();

	VkDescriptorSetLayoutBinding storage_binding            = vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);
	VkDescriptorSetLayoutBinding sampled_binding            = vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL, 0);
	sampled_binding.pImmutableSamplers                      = &vk_immutable_sampler;
	VkDescriptorSetLayoutCreateInfo storage_set_layout_info = vkb::initializers::descriptor_set_layout_create_info(&storage_binding, 1);
	VkDescriptorSetLayoutCreateInfo sampled_set_layout_info = vkb::initializers::descriptor_set_layout_create_info(&sampled_binding, 1);

	VK_CHECK(vkCreateDescriptorSetLayout(device->get_handle(), &storage_set_layout_info, nullptr, &descriptors.storage_layout));
	VK_CHECK(vkCreateDescriptorSetLayout(device->get_handle(), &sampled_set_layout_info, nullptr, &descriptors.sampled_layout));

	VkDescriptorPoolSize pool_sizes[2] = {
	    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, NumAsyncFrames},
	    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, NumAsyncFrames},
	};
	VkDescriptorPoolCreateInfo pool_info = vkb::initializers::descriptor_pool_create_info(2, pool_sizes, NumAsyncFrames * 2);
	VK_CHECK(vkCreateDescriptorPool(device->get_handle(), &pool_info, nullptr, &descriptors.descriptor_pool));

	VkDescriptorSetAllocateInfo storage_alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptors.descriptor_pool, &descriptors.storage_layout, 1);
	VkDescriptorSetAllocateInfo sampled_alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptors.descriptor_pool, &descriptors.sampled_layout, 1);
	for (int i = 0; i < NumAsyncFrames; i++)
	{
		VK_CHECK(vkAllocateDescriptorSets(device->get_handle(), &storage_alloc_info, &descriptors.storage_images[i]));
		VK_CHECK(vkAllocateDescriptorSets(device->get_handle(), &sampled_alloc_info, &descriptors.sampled_images[i]));

		auto general_info  = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, image_views[i]->get_handle(), VK_IMAGE_LAYOUT_GENERAL);
		auto readonly_info = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, image_views[i]->get_handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		const VkWriteDescriptorSet writes[2] = {
		    vkb::initializers::write_descriptor_set(descriptors.storage_images[i], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, &general_info),
		    vkb::initializers::write_descriptor_set(descriptors.sampled_images[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &readonly_info),
		};
		vkUpdateDescriptorSets(device->get_handle(), 2, writes, 0, nullptr);
	}
}

void TimelineSemaphore::create_compute_pipeline()
{
	VkDescriptorSetLayout layouts[2]  = {descriptors.storage_layout, descriptors.sampled_layout};
	auto                  layout_info = vkb::initializers::pipeline_layout_create_info(layouts, 2);

	VkPushConstantRange range          = vkb::initializers::push_constant_range(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(float), 0);
	layout_info.pushConstantRangeCount = 1;
	layout_info.pPushConstantRanges    = &range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_info, nullptr, &pipelines.compute_pipeline_layout));
	VkComputePipelineCreateInfo info = vkb::initializers::compute_pipeline_create_info(pipelines.compute_pipeline_layout);

	info.stage = load_shader("timeline_semaphore/game_of_life_update.comp", VK_SHADER_STAGE_COMPUTE_BIT);
	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &pipelines.compute_update_pipeline));

	info.stage = load_shader("timeline_semaphore/game_of_life_mutate.comp", VK_SHADER_STAGE_COMPUTE_BIT);
	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &pipelines.compute_mutate_pipeline));

	info.stage = load_shader("timeline_semaphore/game_of_life_init.comp", VK_SHADER_STAGE_COMPUTE_BIT);
	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &pipelines.compute_init_pipeline));
}

void TimelineSemaphore::create_graphics_pipeline()
{
	auto layout_info = vkb::initializers::pipeline_layout_create_info(&descriptors.sampled_layout, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_info, nullptr, &pipelines.graphics_pipeline_layout));

	VkGraphicsPipelineCreateInfo info = vkb::initializers::pipeline_create_info(pipelines.graphics_pipeline_layout, render_pass);

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_GREATER);
	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState>      dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables);

	info.pVertexInputState   = &vertex_input_state;
	info.pInputAssemblyState = &input_assembly_state;
	info.pRasterizationState = &rasterization_state;
	info.pColorBlendState    = &color_blend_state;
	info.pDepthStencilState  = &depth_stencil_state;
	info.pViewportState      = &viewport_state;
	info.pMultisampleState   = &multisample_state;
	info.pDynamicState       = &dynamic_state;

	VkPipelineShaderStageCreateInfo stages[2];
	info.pStages    = stages;
	info.stageCount = 2;

	stages[0] = load_shader("timeline_semaphore/render.vert", VK_SHADER_STAGE_VERTEX_BIT);
	stages[1] = load_shader("timeline_semaphore/render.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &pipelines.visualize_pipeline));
}

void TimelineSemaphore::create_pipelines()
{
	create_compute_pipeline();
	create_graphics_pipeline();
}

bool TimelineSemaphore::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	create_resources();
	create_pipelines();
	prepare_queue();
	create_timeline_semaphores();
	create_timeline_workers();

	prepared = true;
	return true;
}

void TimelineSemaphore::render(float delta_time)
{
	ApiVulkanSample::prepare_frame();

	VK_CHECK(vkWaitForFences(get_device().get_handle(), 1, &wait_fences[current_buffer], VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(get_device().get_handle(), 1, &wait_fences[current_buffer]));

	VkViewport viewport = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
	VkRect2D   scissor  = {{0, 0}, {width, height}};

	// Simple fix for 1:1 pixel aspect ratio.
	if (viewport.width > viewport.height)
	{
		viewport.x += 0.5f * (viewport.width - viewport.height);
		viewport.width = viewport.height;
	}
	else if (viewport.height > viewport.width)
	{
		viewport.y += 0.5f * (viewport.height - viewport.width);
		viewport.height = viewport.width;
	}

	auto cmd         = draw_cmd_buffers[current_buffer];
	auto begin_info  = vkb::initializers::command_buffer_begin_info();
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &begin_info);

	VkRenderPassBeginInfo render_pass_begin    = vkb::initializers::render_pass_begin_info();
	render_pass_begin.renderPass               = render_pass;
	render_pass_begin.renderArea.extent.width  = width;
	render_pass_begin.renderArea.extent.height = height;
	render_pass_begin.clearValueCount          = 2;
	VkClearValue clears[2]                     = {};
	clears[0].color.float32[0]                 = 0.033f;
	clears[0].color.float32[1]                 = 0.073f;
	clears[0].color.float32[2]                 = 0.133f;
	render_pass_begin.pClearValues             = clears;
	render_pass_begin.framebuffer              = framebuffers[current_buffer];

	vkCmdBeginRenderPass(cmd, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.visualize_pipeline);
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	main_thread_timeline.timeline++;
	uint32_t frame_index = main_thread_timeline.timeline % NumAsyncFrames;
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.graphics_pipeline_layout,
	                        0, 1, &descriptors.sampled_images[frame_index], 0, nullptr);
	vkCmdDraw(cmd, 3, 1, 0, 0);

	draw_ui(cmd);

	vkCmdEndRenderPass(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Wait for the async queue to have completed rendering.
	wait_timeline_gpu(queue, {async_compute_timeline.semaphore, main_thread_timeline.timeline}, async_compute_timeline_lock);

	// Need to hold the conditional lock during submit_frame as well since vkQueuePresentKHR uses the main queue as well.
	{
		ConditionalLockGuard holder{submission_lock, async_queue == queue};
		VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, wait_fences[current_buffer]));

		// Before we call present, which uses a binary semaphore, we must ensure that all dependent submissions
		// have been submitted, so that the presenting queue is unblocked at the time of calling.
		wait_pending(async_compute_timeline_lock, main_thread_timeline.timeline);

		ApiVulkanSample::submit_frame();
	}

	// Let async queue know it is safe to clobber the image since main queue is done reading it.
	signal_timeline_gpu(queue, main_thread_timeline, main_thread_timeline_lock);
}

void TimelineSemaphore::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Need to enable the timelineSemaphore feature.
	auto &features = gpu.request_extension_features<VkPhysicalDeviceTimelineSemaphoreFeaturesKHR>(
	    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR);
	features.timelineSemaphore = VK_TRUE;
}

std::unique_ptr<vkb::VulkanSample> create_timeline_semaphore()
{
	return std::make_unique<TimelineSemaphore>();
}
