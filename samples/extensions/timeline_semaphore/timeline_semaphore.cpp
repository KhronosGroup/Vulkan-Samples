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

#include "timeline_semaphore.h"
#include "common/vk_initializers.h"

// What we're trying to demonstrate here is:
// - Out-of-order submission using threads which synchronize GPU work with each other using timeline semaphores.
//   In this sample we have a dedicated worker threads for submitting work to the compute and graphics pipelines respectively,
//   and the only synchronization with main thread happens via timeline semaphores.
// - Waiting for timeline semaphore on CPU to replace redundant fence objects.
// - Multiple waits on the same timeline. We don't need to worry about allocating and managing binary semaphores in complex scenarios.
//   We can wait on the same timeline values as many times as we want, and we avoid all resource management problems that binary semaphores have.

namespace
{
static constexpr unsigned grid_width  = 64;
static constexpr unsigned grid_height = 64;

}        // namespace

TimelineSemaphore::TimelineSemaphore()
{
	title = "Timeline Semaphore";

	add_device_extension(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
}

TimelineSemaphore::~TimelineSemaphore()
{
	if (prepared)
	{
		finish_timeline_workers();
	}

	if (has_device())
	{
		VkDevice vk_device = get_device().get_handle();
		vkDestroyCommandPool(vk_device, graphics.command_pool, nullptr);
		vkDestroyPipelineLayout(vk_device, graphics.pipeline_layout, nullptr);
		vkDestroyPipeline(vk_device, graphics.pipeline, nullptr);

		vkDestroyCommandPool(vk_device, compute.command_pool, nullptr);
		vkDestroyPipelineLayout(vk_device, compute.pipeline_layout, nullptr);
		vkDestroyPipeline(vk_device, compute.update_pipeline, nullptr);
		vkDestroyPipeline(vk_device, compute.mutate_pipeline, nullptr);
		vkDestroyPipeline(vk_device, compute.init_pipeline, nullptr);

		vkDestroyDescriptorSetLayout(vk_device, shared.storage_layout, nullptr);
		vkDestroyDescriptorSetLayout(vk_device, shared.sampled_layout, nullptr);
		vkDestroyDescriptorPool(vk_device, shared.descriptor_pool, nullptr);

		vkDestroySemaphore(vk_device, timeline.semaphore, nullptr);
	}
}

void TimelineSemaphore::setup_shared_resources()
{
	// Descriptor pool
	{
		VkDescriptorPoolSize pool_sizes[2] = {
		    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, NumAsyncFrames},
		    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, NumAsyncFrames},
		};

		VkDescriptorPoolCreateInfo pool_info = vkb::initializers::descriptor_pool_create_info(2, pool_sizes, 2 * NumAsyncFrames);
		VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &pool_info, nullptr, &shared.descriptor_pool));
	}

	// Sampler
	{
		auto sampler_create_info         = vkb::initializers::sampler_create_info();
		sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_create_info.minFilter    = VK_FILTER_NEAREST;
		sampler_create_info.magFilter    = VK_FILTER_NEAREST;
		sampler_create_info.maxLod       = VK_LOD_CLAMP_NONE;
		sampler_create_info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		shared.immutable_sampler         = std::make_unique<vkb::core::Sampler>(get_device(), sampler_create_info);
	}

	// Images and image views
	{
		const auto            present_index = get_device().get_queue_by_present(0).get_family_index();
		auto                  sharing_mode  = VK_SHARING_MODE_CONCURRENT;
		std::vector<uint32_t> queue_families{compute.queue_family_index};

		if (graphics.queue_family_index != compute.queue_family_index)
		{
			queue_families.push_back(graphics.queue_family_index);
		}

		if (compute.queue_family_index != present_index && graphics.queue_family_index != present_index)
		{
			queue_families.push_back(present_index);
		}

		if (queue_families.size() <= 1)
		{
			sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		}

		for (int i = 0; i < NumAsyncFrames; ++i)
		{
			// Need CONCURRENT usage here since we will sample from the image in both graphics and compute queues.
			shared.images[i] = std::make_unique<vkb::core::Image>(get_device(), vkb::core::ImageBuilder(VkExtent3D{grid_width, grid_height, 1})
			                                                                        .with_format(VK_FORMAT_R8G8B8A8_UNORM)
			                                                                        .with_usage(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
			                                                                        .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
			                                                                        .with_sample_count(VK_SAMPLE_COUNT_1_BIT)
			                                                                        .with_mip_levels(1)
			                                                                        .with_array_layers(1)
			                                                                        .with_tiling(VK_IMAGE_TILING_OPTIMAL)
			                                                                        .with_queue_families(static_cast<uint32_t>(queue_families.size()), queue_families.data())
			                                                                        .with_sharing_mode(sharing_mode));

			shared.image_views[i] = std::make_unique<vkb::core::ImageView>(*shared.images[i], VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM);
		}
	}

	// Descriptor layouts
	{
		VkDescriptorSetLayoutBinding storage_binding = vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);
		VkDescriptorSetLayoutBinding sampled_binding = vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL, 0);

		VkSampler vk_immutable_sampler     = shared.immutable_sampler->get_handle();
		sampled_binding.pImmutableSamplers = &vk_immutable_sampler;

		VkDescriptorSetLayoutCreateInfo storage_set_layout_info = vkb::initializers::descriptor_set_layout_create_info(&storage_binding, 1);
		VkDescriptorSetLayoutCreateInfo sampled_set_layout_info = vkb::initializers::descriptor_set_layout_create_info(&sampled_binding, 1);

		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &storage_set_layout_info, nullptr, &shared.storage_layout));
		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &sampled_set_layout_info, nullptr, &shared.sampled_layout));
	}

	// Descriptor sets
	{
		VkDescriptorSetAllocateInfo storage_alloc_info = vkb::initializers::descriptor_set_allocate_info(shared.descriptor_pool, &shared.storage_layout, 1);
		VkDescriptorSetAllocateInfo sampled_alloc_info = vkb::initializers::descriptor_set_allocate_info(shared.descriptor_pool, &shared.sampled_layout, 1);

		for (int i = 0; i < NumAsyncFrames; ++i)
		{
			VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &storage_alloc_info, &shared.storage_descriptor_sets[i]));
			VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &sampled_alloc_info, &shared.sampled_descriptor_sets[i]));

			auto general_info  = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, shared.image_views[i]->get_handle(), VK_IMAGE_LAYOUT_GENERAL);
			auto readonly_info = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, shared.image_views[i]->get_handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			const VkWriteDescriptorSet writes[2] = {
			    vkb::initializers::write_descriptor_set(shared.storage_descriptor_sets[i], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, &general_info),
			    vkb::initializers::write_descriptor_set(shared.sampled_descriptor_sets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &readonly_info),
			};

			vkUpdateDescriptorSets(get_device().get_handle(), 2, writes, 0, nullptr);
		}
	}
}

void TimelineSemaphore::build_command_buffers()
{
	// Unused, but required to resolve pure virtual function inherited from ApiVulkanSample
}

void TimelineSemaphore::create_timeline_semaphore()
{
	// A timeline semaphore is still a semaphore, but it is of TIMELINE type rather than BINARY.
	VkSemaphoreCreateInfo        create_info = vkb::initializers::semaphore_create_info();
	VkSemaphoreTypeCreateInfoKHR type_create_info{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR};

	type_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;
	type_create_info.initialValue  = 0;
	create_info.pNext              = &type_create_info;

	VK_CHECK(vkCreateSemaphore(get_device().get_handle(), &create_info, nullptr, &timeline.semaphore));

	timeline.frame = 0;
}

void TimelineSemaphore::start_timeline_workers()
{
	graphics_worker.alive  = true;
	graphics_worker.thread = std::thread([this]() { do_graphics_work(); });

	compute_worker.alive  = true;
	compute_worker.thread = std::thread([this]() { do_compute_work(); });
}

void TimelineSemaphore::finish_timeline_workers()
{
	graphics_worker.alive = false;
	compute_worker.alive  = false;

	// The MAX_STAGES value is used to unblock all threads that are waiting on a timeline stage
	signal_timeline(Timeline::MAX_STAGES);

	if (graphics_worker.thread.joinable())
	{
		graphics_worker.thread.join();
	}

	if (compute_worker.thread.joinable())
	{
		compute_worker.thread.join();
	}
}

// Signal the timeline from the host.
void TimelineSemaphore::signal_timeline(const Timeline::Stages stage)
{
	VkSemaphoreSignalInfo signalInfo;
	signalInfo.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
	signalInfo.pNext     = NULL;
	signalInfo.semaphore = timeline.semaphore;
	signalInfo.value     = get_timeline_stage_value(stage);

	VK_CHECK(vkSignalSemaphoreKHR(get_device().get_handle(), &signalInfo));
}

// Wait on the timeline from the host.
void TimelineSemaphore::wait_on_timeline(const Timeline::Stages stage)
{
	const uint64_t waitValue = get_timeline_stage_value(stage);

	VkSemaphoreWaitInfo waitInfo;
	waitInfo.sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	waitInfo.pNext          = NULL;
	waitInfo.flags          = 0;
	waitInfo.semaphoreCount = 1;
	waitInfo.pSemaphores    = &timeline.semaphore;
	waitInfo.pValues        = &waitValue;

	VK_CHECK(vkWaitSemaphoresKHR(get_device().get_handle(), &waitInfo, UINT64_MAX));
}

// Sends the MAX_STAGES signal for the current frame, then increments the frame counter
void TimelineSemaphore::signal_next_frame()
{
	VkSemaphoreSignalInfo signalInfo;
	signalInfo.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
	signalInfo.pNext     = NULL;
	signalInfo.semaphore = timeline.semaphore;
	signalInfo.value     = get_timeline_stage_value(Timeline::MAX_STAGES);

	timeline.frame++;

	VK_CHECK(vkSignalSemaphoreKHR(get_device().get_handle(), &signalInfo));
}

// Waits for the timeline to reach MAX_STAGES for the current frame
void TimelineSemaphore::wait_for_next_frame()
{
	// MAX_STAGES is used as it provides a boundary value between the stages of this frame and the next
	const uint64_t waitValue = (timeline.frame + 1) * Timeline::MAX_STAGES;

	VkSemaphoreWaitInfo waitInfo;
	waitInfo.sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	waitInfo.pNext          = NULL;
	waitInfo.flags          = 0;
	waitInfo.semaphoreCount = 1;
	waitInfo.pSemaphores    = &timeline.semaphore;
	waitInfo.pValues        = &waitValue;

	VK_CHECK(vkWaitSemaphoresKHR(get_device().get_handle(), &waitInfo, UINT64_MAX));
}

// Calculates the timeline value for the specified stage in the current frame
uint64_t TimelineSemaphore::get_timeline_stage_value(const Timeline::Stages stage)
{
	return (timeline.frame * Timeline::MAX_STAGES) + stage;
}

void TimelineSemaphore::do_compute_work()
{
	compute.timer.start();

	while (compute_worker.alive)
	{
		// Wait for the main thread to signal that the workers can prepare and submit their work
		wait_on_timeline(Timeline::submit);

		auto elapsed = static_cast<float>(compute.timer.elapsed());

		build_compute_command_buffers(elapsed);

		uint64_t                      signal_value  = get_timeline_stage_value(Timeline::draw);
		VkTimelineSemaphoreSubmitInfo timeline_info = vkb::initializers::timeline_semaphore_submit_info(0, nullptr, 1, &signal_value);

		VkSubmitInfo submit_info         = vkb::initializers::submit_info();
		submit_info.pNext                = &timeline_info;
		submit_info.commandBufferCount   = 1;
		submit_info.pCommandBuffers      = &compute.command_buffer;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores    = &timeline.semaphore;

		// If the threads are being killed, we need to skip the queue submission to allow the program to exit gracefully
		if (compute_worker.alive)
		{
			VK_CHECK(vkQueueSubmit(compute.queue, 1, &submit_info, VK_NULL_HANDLE));
		}

		wait_for_next_frame();
	}
}

void TimelineSemaphore::setup_compute_pipeline()
{
	VkDescriptorSetLayout layouts[2]  = {shared.storage_layout, shared.sampled_layout};
	auto                  layout_info = vkb::initializers::pipeline_layout_create_info(layouts, 2);

	VkPushConstantRange range          = vkb::initializers::push_constant_range(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(float), 0);
	layout_info.pushConstantRangeCount = 1;
	layout_info.pPushConstantRanges    = &range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_info, nullptr, &compute.pipeline_layout));
	VkComputePipelineCreateInfo info = vkb::initializers::compute_pipeline_create_info(compute.pipeline_layout);

	info.stage = load_shader("timeline_semaphore/game_of_life_update.comp", VK_SHADER_STAGE_COMPUTE_BIT);
	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &compute.update_pipeline));

	info.stage = load_shader("timeline_semaphore/game_of_life_mutate.comp", VK_SHADER_STAGE_COMPUTE_BIT);
	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &compute.mutate_pipeline));

	info.stage = load_shader("timeline_semaphore/game_of_life_init.comp", VK_SHADER_STAGE_COMPUTE_BIT);
	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &compute.init_pipeline));
}

void TimelineSemaphore::setup_compute_resources()
{
	// Get compute queue
	compute.queue_family_index = get_device().get_queue_family_index(VK_QUEUE_COMPUTE_BIT);
	vkGetDeviceQueue(get_device().get_handle(), compute.queue_family_index, 0, &compute.queue);

	compute.command_pool = get_device().create_command_pool(compute.queue_family_index, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	VkCommandBufferAllocateInfo alloc_info =
	    vkb::initializers::command_buffer_allocate_info(compute.command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

	VK_CHECK(vkAllocateCommandBuffers(get_device().get_handle(), &alloc_info, &compute.command_buffer));
}

void TimelineSemaphore::setup_game_of_life()
{
	auto begin_info  = vkb::initializers::command_buffer_begin_info();
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkResetCommandBuffer(compute.command_buffer, 0));
	VK_CHECK(vkBeginCommandBuffer(compute.command_buffer, &begin_info));

	for (int i = 0; i < NumAsyncFrames; ++i)
	{
		vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline_layout, 0, 1, &shared.storage_descriptor_sets[i], 0, nullptr);

		//  On the first iteration, we initialize the game of life.
		vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.init_pipeline);

		VkImageMemoryBarrier image_barrier = vkb::initializers::image_memory_barrier();
		image_barrier.srcAccessMask        = 0;
		image_barrier.dstAccessMask        = VK_ACCESS_SHADER_WRITE_BIT;
		image_barrier.image                = shared.images[i]->get_handle();
		image_barrier.subresourceRange     = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		image_barrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
		image_barrier.newLayout            = VK_IMAGE_LAYOUT_GENERAL;

		// The semaphore takes care of srcStageMask.
		vkCmdPipelineBarrier(compute.command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);

		vkCmdDispatch(compute.command_buffer, grid_width / 8, grid_height / 8, 1);

		image_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		image_barrier.dstAccessMask = 0;
		image_barrier.oldLayout     = VK_IMAGE_LAYOUT_GENERAL;
		image_barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// The semaphore takes care of dstStageMask.
		vkCmdPipelineBarrier(compute.command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);
	}

	VK_CHECK(vkEndCommandBuffer(compute.command_buffer));

	VkSubmitInfo submit_info       = vkb::initializers::submit_info();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &compute.command_buffer;

	VK_CHECK(vkQueueSubmit(compute.queue, 1, &submit_info, VK_NULL_HANDLE));

	VK_CHECK(get_device().wait_idle());
}

void TimelineSemaphore::build_compute_command_buffers(const float elapsed)
{
	auto begin_info  = vkb::initializers::command_buffer_begin_info();
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkResetCommandBuffer(compute.command_buffer, 0));
	VK_CHECK(vkBeginCommandBuffer(compute.command_buffer, &begin_info));

	auto frame_index = timeline.frame % NumAsyncFrames;
	auto prev_index  = (timeline.frame - 1) % NumAsyncFrames;

	vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline_layout, 0, 1, &shared.storage_descriptor_sets[frame_index], 0, nullptr);

	if (elapsed > 1.0f)
	{
		vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.update_pipeline);
		compute.timer.lap();
	}
	else
	{
		vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.mutate_pipeline);
		vkCmdPushConstants(compute.command_buffer, compute.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT,
		                   0, sizeof(elapsed), &elapsed);
	}

	// Bind previous iteration's texture.
	vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline_layout, 1, 1, &shared.sampled_descriptor_sets[prev_index], 0, nullptr);

	VkImageMemoryBarrier image_barrier = vkb::initializers::image_memory_barrier();
	image_barrier.srcAccessMask        = 0;
	image_barrier.dstAccessMask        = VK_ACCESS_SHADER_WRITE_BIT;
	image_barrier.image                = shared.images[frame_index]->get_handle();
	image_barrier.subresourceRange     = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	image_barrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
	image_barrier.newLayout            = VK_IMAGE_LAYOUT_GENERAL;

	// The semaphore takes care of srcStageMask.
	vkCmdPipelineBarrier(compute.command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);

	vkCmdDispatch(compute.command_buffer, grid_width / 8, grid_height / 8, 1);

	image_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	image_barrier.dstAccessMask = 0;
	image_barrier.oldLayout     = VK_IMAGE_LAYOUT_GENERAL;
	image_barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// The semaphore takes care of dstStageMask.
	vkCmdPipelineBarrier(compute.command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);

	VK_CHECK(vkEndCommandBuffer(compute.command_buffer));
}

void TimelineSemaphore::do_graphics_work()
{
	while (graphics_worker.alive)
	{
		// Wait for the main thread to signal that the workers can prepare and submit their work
		wait_on_timeline(Timeline::submit);

		build_graphics_command_buffer();

		uint64_t                      wait_values[]       = {get_timeline_stage_value(Timeline::draw), 0};
		VkPipelineStageFlags          wait_stage_masks[]  = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		VkSemaphore                   wait_semaphores[]   = {timeline.semaphore, semaphores.acquired_image_ready};
		uint64_t                      signal_values[]     = {get_timeline_stage_value(Timeline::present), 0};
		VkSemaphore                   signal_semaphores[] = {timeline.semaphore, semaphores.render_complete};
		VkTimelineSemaphoreSubmitInfo timeline_info       = vkb::initializers::timeline_semaphore_submit_info(2, wait_values, 2, signal_values);

		VkSubmitInfo submit_info         = vkb::initializers::submit_info();
		submit_info.pNext                = &timeline_info;
		submit_info.waitSemaphoreCount   = 2;
		submit_info.pWaitSemaphores      = wait_semaphores;
		submit_info.pWaitDstStageMask    = wait_stage_masks;
		submit_info.signalSemaphoreCount = 2;
		submit_info.pSignalSemaphores    = signal_semaphores;
		submit_info.commandBufferCount   = 1;
		submit_info.pCommandBuffers      = &graphics.command_buffer;

		if (compute.queue == graphics.queue)
		{
			// If compute.queue == queue, we need synchronise access to the queue AND ensure that submissions are made in order
			// (otherwise the queue will deadlock itself). So we wait for the "draw" stage to be signalled on the host, before
			// submitting the work.
			wait_on_timeline(Timeline::draw);
		}

		// If the threads are being killed, we need to skip the queue submission to allow the program to exit gracefully
		if (graphics_worker.alive)
		{
			VK_CHECK(vkQueueSubmit(graphics.queue, 1, &submit_info, VK_NULL_HANDLE));
		}

		wait_for_next_frame();
	}
}

void TimelineSemaphore::setup_graphics_resources()
{
	graphics.queue_family_index = get_device().get_queue_family_index(VK_QUEUE_GRAPHICS_BIT);
	graphics.queue              = queue;

	graphics.command_pool = get_device().create_command_pool(graphics.queue_family_index, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	VkCommandBufferAllocateInfo alloc_info =
	    vkb::initializers::command_buffer_allocate_info(graphics.command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

	VK_CHECK(vkAllocateCommandBuffers(get_device().get_handle(), &alloc_info, &graphics.command_buffer));
}

void TimelineSemaphore::setup_graphics_pipeline()
{
	auto layout_info = vkb::initializers::pipeline_layout_create_info(&shared.sampled_layout, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_info, nullptr, &graphics.pipeline_layout));

	VkGraphicsPipelineCreateInfo info = vkb::initializers::pipeline_create_info(graphics.pipeline_layout, render_pass);

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
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), VK_NULL_HANDLE, 1, &info, nullptr, &graphics.pipeline));
}

void TimelineSemaphore::build_graphics_command_buffer()
{
	auto       frame_index = timeline.frame % NumAsyncFrames;
	VkViewport viewport    = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
	VkRect2D   scissor     = {{0, 0}, {width, height}};

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

	VK_CHECK(vkResetCommandBuffer(graphics.command_buffer, 0));

	auto begin_info  = vkb::initializers::command_buffer_begin_info();
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(graphics.command_buffer, &begin_info));

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

	vkCmdBeginRenderPass(graphics.command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(graphics.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipeline);
	vkCmdSetViewport(graphics.command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(graphics.command_buffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(graphics.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipeline_layout, 0, 1, &shared.sampled_descriptor_sets[frame_index], 0, nullptr);
	vkCmdDraw(graphics.command_buffer, 3, 1, 0, 0);

	draw_ui(graphics.command_buffer);

	vkCmdEndRenderPass(graphics.command_buffer);

	VK_CHECK(vkEndCommandBuffer(graphics.command_buffer));
}

void TimelineSemaphore::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Need to enable the timelineSemaphore feature.
	REQUEST_REQUIRED_FEATURE(gpu,
	                         VkPhysicalDeviceTimelineSemaphoreFeaturesKHR,
	                         VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR,
	                         timelineSemaphore);
}

bool TimelineSemaphore::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	setup_compute_resources();
	setup_graphics_resources();
	setup_shared_resources();

	setup_compute_pipeline();
	setup_graphics_pipeline();

	setup_game_of_life();

	create_timeline_semaphore();

	start_timeline_workers();

	prepared = true;

	return true;
}

void TimelineSemaphore::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}

	ApiVulkanSample::prepare_frame();

	// Signal to the worker threads that they can submit their work
	signal_timeline(Timeline::submit);

	// Wait for the worker threads to signal that the frame is ready to present
	wait_on_timeline(Timeline::present);

	ApiVulkanSample::submit_frame();

	// Signal to the worker threads that they can proceed to the next frame's work
	signal_next_frame();
}

std::unique_ptr<vkb::Application> create_timeline_semaphore()
{
	return std::make_unique<TimelineSemaphore>();
}
