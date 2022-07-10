/* Copyright (c) 2022, Arm Limited and Contributors
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

#include <components/platform/sample_main.hpp>

#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

#include <components/events/event_pipeline.hpp>
#include <components/vfs/filesystem.hpp>
#include <components/windows/glfw.hpp>

#include "device.hpp"
#include "graphics_pipeline.hpp"
#include "instance.hpp"
#include "swapchain.hpp"

using namespace components;

bool resize(Context &context, const uint32_t, const uint32_t)
{
	if (context.device == VK_NULL_HANDLE)
	{
		return false;
	}

	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.gpu, context.surface, &surface_properties));

	// Only rebuild the swapchain if the dimensions have changed
	if (surface_properties.currentExtent.width == context.swapchain_dimensions.width &&
	    surface_properties.currentExtent.height == context.swapchain_dimensions.height)
	{
		return false;
	}

	vkDeviceWaitIdle(context.device);
	teardown_framebuffers(context);

	init_swapchain(context);
	init_framebuffers(context);
	return true;
}

EXPORT_CLIB int sample_main(PlatformContext *platform_context)
{
	Context context;

	// init a platform specific window
	std::shared_ptr<windows::Window> window = std::make_shared<windows::GLFWWindow>("Hello Triangle", windows::Extent{600, 600});        // TODO: create a window based on PlatformContext

	// the event bus consumes events from other components and provides a mechanism to react to these events
	events::EventBus event_bus;

	// register window to the event bus
	event_bus.attach(window);

	// handle window closed externally
	bool should_close = false;
	event_bus.last<windows::ShouldCloseEvent>([&](const windows::ShouldCloseEvent &) {
		should_close = true;
	});

	// handle window context rect changed - we only care about the last resize event each frame
	event_bus.last<windows::ContentRectChangedEvent>([&](const windows::ContentRectChangedEvent &event) {
		resize(context, event.extent.width, event.extent.height);
	});

	// prepare
	init_instance(context, {VK_KHR_SURFACE_EXTENSION_NAME}, {});

	init_surface(platform_context, context, *window);

	context.swapchain_dimensions.width  = 600;
	context.swapchain_dimensions.height = 600;

	init_device(context, {"VK_KHR_swapchain"});

	init_swapchain(context);

	// Create the necessary objects for rendering.
	init_render_pass(context);

	auto &fs = vfs::_default(static_cast<void *>(platform_context));        // TODO: add PlatformContext to vfs

	std::shared_ptr<vfs::Blob> vertex_blob;
	if (auto err = fs.read_file("/shaders/triangle.vert", &vertex_blob))
	{
		return EXIT_FAILURE;
	}

	std::shared_ptr<vfs::Blob> fragment_blob;
	if (auto err = fs.read_file("/shaders/triangle.frag", &fragment_blob))
	{
		return EXIT_FAILURE;
	}

	init_pipeline(context, vertex_blob->binary(), fragment_blob->binary());

	init_framebuffers(context);

	while (!should_close)
	{
		// process events
		event_bus.process();

		uint32_t swapchain_index;

		auto res = acquire_next_image(context, &swapchain_index);

		// Handle outdated error in acquire.
		if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			resize(context, context.swapchain_dimensions.width, context.swapchain_dimensions.height);
			res = acquire_next_image(context, &swapchain_index);
		}

		if (res != VK_SUCCESS)
		{
			vkQueueWaitIdle(context.queue);
			continue;
		}

		// render triangle
		VkFramebuffer framebuffer = context.swapchain_framebuffers[swapchain_index];

		// Allocate or re-use a primary command buffer.
		VkCommandBuffer cmd = context.per_frame[swapchain_index].primary_command_buffer;

		// We will only submit this once before it's recycled.
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		// Begin command recording
		vkBeginCommandBuffer(cmd, &begin_info);

		// Set clear color values.
		VkClearValue clear_value;
		clear_value.color = {{0.1f, 0.1f, 0.2f, 1.0f}};

		// Begin the render pass.
		VkRenderPassBeginInfo rp_begin{};
		rp_begin.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rp_begin.renderPass               = context.render_pass;
		rp_begin.framebuffer              = framebuffer;
		rp_begin.renderArea.extent.width  = context.swapchain_dimensions.width;
		rp_begin.renderArea.extent.height = context.swapchain_dimensions.height;
		rp_begin.clearValueCount          = 1;
		rp_begin.pClearValues             = &clear_value;
		// We will add draw commands in the same command buffer.
		vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

		// Bind the graphics pipeline.
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline);

		VkViewport vp{};
		vp.width    = static_cast<float>(context.swapchain_dimensions.width);
		vp.height   = static_cast<float>(context.swapchain_dimensions.height);
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;
		// Set viewport dynamically
		vkCmdSetViewport(cmd, 0, 1, &vp);

		VkRect2D scissor{};
		scissor.extent.width  = context.swapchain_dimensions.width;
		scissor.extent.height = context.swapchain_dimensions.height;
		// Set scissor dynamically
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// Draw three vertices with one instance.
		vkCmdDraw(cmd, 3, 1, 0, 0);

		// Complete render pass.
		vkCmdEndRenderPass(cmd);

		// Complete the command buffer.
		VK_CHECK(vkEndCommandBuffer(cmd));

		// Submit it to the queue with a release semaphore.
		if (context.per_frame[swapchain_index].swapchain_release_semaphore == VK_NULL_HANDLE)
		{
			VkSemaphoreCreateInfo semaphore_info{};
			semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VK_CHECK(vkCreateSemaphore(context.device, &semaphore_info, nullptr,
			                           &context.per_frame[swapchain_index].swapchain_release_semaphore));
		}

		VkPipelineStageFlags wait_stage{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

		VkSubmitInfo info{};
		info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.commandBufferCount   = 1;
		info.pCommandBuffers      = &cmd;
		info.waitSemaphoreCount   = 1;
		info.pWaitSemaphores      = &context.per_frame[swapchain_index].swapchain_acquire_semaphore;
		info.pWaitDstStageMask    = &wait_stage;
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores    = &context.per_frame[swapchain_index].swapchain_release_semaphore;
		// Submit command buffer to graphics queue
		VK_CHECK(vkQueueSubmit(context.queue, 1, &info, context.per_frame[swapchain_index].queue_submit_fence));

		res = present_image(context, swapchain_index);

		// Handle Outdated error in present.
		if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			resize(context, context.swapchain_dimensions.width, context.swapchain_dimensions.height);
		}
		else if (res != VK_SUCCESS)
		{
			LOGE("Failed to present swapchain image.");
		}

		// sleep to not drain system resources
		std::this_thread::sleep_for(10ms);
	}

	// Don't release anything until the GPU is completely idle.
	vkDeviceWaitIdle(context.device);

	teardown_framebuffers(context);

	for (auto &per_frame : context.per_frame)
	{
		teardown_per_frame(context, per_frame);
	}

	teardown(context);

	return EXIT_SUCCESS;
}