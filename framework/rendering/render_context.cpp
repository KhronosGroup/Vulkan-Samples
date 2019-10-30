/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "render_context.h"

namespace vkb
{
RenderContext::RenderContext(Device &device, VkSurfaceKHR surface) :
    device{device},
    present_queue{device.get_queue_by_present(0)}
{
	swapchain = std::make_unique<Swapchain>(device, surface);
}

void RenderContext::request_present_mode(const VkPresentModeKHR present_mode)
{
	auto &properties        = swapchain->get_properties();
	properties.present_mode = present_mode;
}

void RenderContext::request_image_format(const VkFormat format)
{
	auto &properties                 = swapchain->get_properties();
	properties.surface_format.format = format;
}

void RenderContext::prepare(uint16_t command_pools_per_frame, RenderTarget::CreateFunc create_render_target_func)
{
	swapchain->set_present_mode_priority(present_mode_priority_list);
	swapchain->set_surface_format_priority(surface_format_priority_list);
	swapchain->create();

	surface_extent = swapchain->get_extent();

	device.wait_idle();

	VkExtent3D extent{surface_extent.width, surface_extent.height, 1};

	for (auto &image_handle : swapchain->get_images())
	{
		auto swapchain_image = core::Image{
		    device, image_handle,
		    extent,
		    swapchain->get_format(),
		    swapchain->get_usage()};
		auto render_target = create_render_target_func(std::move(swapchain_image));
		frames.emplace_back(RenderFrame{device, std::move(render_target), command_pools_per_frame});
	}

	this->command_pools_per_frame = command_pools_per_frame;
	this->create_render_target    = create_render_target_func;
}

void RenderContext::update_swapchain(const VkExtent2D &extent)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains extent in headless mode, skipping.");
		return;
	}

	device.get_resource_cache().clear_framebuffers();

	swapchain = std::make_unique<Swapchain>(*swapchain, extent);

	recreate();
}

void RenderContext::update_swapchain(const uint32_t image_count)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains image count in headless mode, skipping.");
		return;
	}

	device.get_resource_cache().clear_framebuffers();

	device.wait_idle();

	swapchain = std::make_unique<Swapchain>(*swapchain, image_count);

	recreate();
}

void RenderContext::update_swapchain(const std::set<VkImageUsageFlagBits> &image_usage_flags)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains image usage in headless mode, skipping.");
		return;
	}

	device.get_resource_cache().clear_framebuffers();

	swapchain = std::make_unique<Swapchain>(*swapchain, image_usage_flags);

	recreate();
}

void RenderContext::update_swapchain(const VkExtent2D &extent, const VkSurfaceTransformFlagBitsKHR transform)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains extent and surface transform in headless mode, skipping.");
		return;
	}

	device.get_resource_cache().clear_framebuffers();

	swapchain = std::make_unique<Swapchain>(*swapchain, extent, transform);

	recreate();
}

void RenderContext::recreate()
{
	VkExtent2D swapchain_extent = swapchain->get_extent();
	VkExtent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

	auto frame_it = frames.begin();

	for (auto &image_handle : swapchain->get_images())
	{
		core::Image swapchain_image{device, image_handle,
		                            extent,
		                            swapchain->get_format(),
		                            swapchain->get_usage()};

		auto render_target = create_render_target(std::move(swapchain_image));

		if (frame_it != frames.end())
		{
			frame_it->update_render_target(std::move(render_target));
		}
		else
		{
			// Create a new frame if the new swapchain has more images than current frames
			frames.emplace_back(RenderFrame{device, std::move(render_target), command_pools_per_frame});
		}

		++frame_it;
	}
}

void RenderContext::set_present_mode_priority(const std::vector<VkPresentModeKHR> &new_present_mode_priority_list)
{
	this->present_mode_priority_list = new_present_mode_priority_list;
}

void RenderContext::set_surface_format_priority(const std::vector<VkSurfaceFormatKHR> &new_surface_format_priority_list)
{
	this->surface_format_priority_list = new_surface_format_priority_list;
}

VkSemaphore RenderContext::begin_frame()
{
	assert(swapchain->is_valid() && "RenderContext isn't prepared, please call prepare");

	handle_surface_changes();

	assert(!frame_active && "Frame is still active, please call end_frame");

	auto &prev_frame = frames.at(active_frame_index);

	auto fence = prev_frame.get_fence_pool().request_fence();

	auto aquired_semaphore = prev_frame.get_semaphore_pool().request_semaphore();

	auto result = swapchain->acquire_next_image(active_frame_index, aquired_semaphore, fence);

	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		handle_surface_changes();

		result = swapchain->acquire_next_image(active_frame_index, aquired_semaphore, fence);
	}

	if (result != VK_SUCCESS)
	{
		prev_frame.reset();

		return VK_NULL_HANDLE;
	}

	// Now the frame is active again
	frame_active = true;

	wait_frame();

	return aquired_semaphore;
}

VkSemaphore RenderContext::submit(const Queue &queue, const CommandBuffer &command_buffer, VkSemaphore wait_semaphore, VkPipelineStageFlags wait_pipeline_stage)
{
	RenderFrame &frame = get_active_frame();

	VkSemaphore signal_semaphore = frame.get_semaphore_pool().request_semaphore();

	VkCommandBuffer cmd_buf = command_buffer.get_handle();

	VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};

	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &cmd_buf;
	submit_info.waitSemaphoreCount   = 1;
	submit_info.pWaitSemaphores      = &wait_semaphore;
	submit_info.pWaitDstStageMask    = &wait_pipeline_stage;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = &signal_semaphore;

	VkFence fence = frame.get_fence_pool().request_fence();

	queue.submit({submit_info}, fence);

	return signal_semaphore;
}

void RenderContext::submit(const Queue &queue, const CommandBuffer &command_buffer)
{
	RenderFrame &frame = get_active_frame();

	VkCommandBuffer cmd_buf = command_buffer.get_handle();

	VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &cmd_buf;

	VkFence fence = frame.get_fence_pool().request_fence();

	queue.submit({submit_info}, fence);
}

void RenderContext::wait_frame()
{
	RenderFrame &frame = get_active_frame();
	frame.reset();
}

void RenderContext::end_frame(VkSemaphore semaphore)
{
	assert(frame_active && "Frame is not active, please call begin_frame");

	VkSwapchainKHR vk_swapchain = swapchain->get_handle();

	VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};

	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores    = &semaphore;
	present_info.swapchainCount     = 1;
	present_info.pSwapchains        = &vk_swapchain;
	present_info.pImageIndices      = &active_frame_index;

	VkResult result = present_queue.present(present_info);

	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		handle_surface_changes();
	}
	// Frame is not active anymore
	frame_active = false;
}

RenderFrame &RenderContext::get_active_frame()
{
	assert(frame_active && "Frame is not active, please call begin_frame");
	return frames.at(active_frame_index);
}

RenderFrame &RenderContext::get_last_rendered_frame()
{
	assert(!frame_active && "Frame is still active, please call end_frame");
	return frames.at(active_frame_index);
}

CommandBuffer &RenderContext::request_frame_command_buffer(const Queue &queue, CommandBuffer::ResetMode reset_mode, VkCommandBufferLevel level, uint16_t pool_index)
{
	auto &command_pools = get_active_frame().get_command_pools(queue, reset_mode);

	return command_pools.at(pool_index)->request_command_buffer(level);
}

VkSemaphore RenderContext::request_semaphore()
{
	RenderFrame &frame = get_active_frame();
	return frame.get_semaphore_pool().request_semaphore();
}

Device &RenderContext::get_device()
{
	return device;
}

void RenderContext::update_swapchain(std::unique_ptr<Swapchain> &&new_swapchain)
{
	device.wait_idle();
	device.get_resource_cache().clear_framebuffers();

	swapchain = std::move(new_swapchain);

	VkExtent2D swapchain_extent = swapchain->get_extent();
	VkExtent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

	auto frame_it = frames.begin();

	for (auto &image_handle : swapchain->get_images())
	{
		core::Image swapchain_image{device, image_handle,
		                            extent,
		                            swapchain->get_format(),
		                            swapchain->get_usage()};

		auto render_target = create_render_target(std::move(swapchain_image));
		frame_it->update_render_target(std::move(render_target));

		++frame_it;
	}
}

void RenderContext::handle_surface_changes()
{
	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.get_physical_device(),
	                                                   swapchain->get_surface(),
	                                                   &surface_properties));

	// Only recreate the swapchain if the dimensions have changed;
	// handle_surface_changes() is called on VK_SUBOPTIMAL_KHR,
	// which might not be due to a surface resize
	if (surface_properties.currentExtent.width != surface_extent.width ||
	    surface_properties.currentExtent.height != surface_extent.height)
	{
		// Recreate swapchain
		device.wait_idle();

		auto new_swapchain = std::make_unique<vkb::Swapchain>(
		    get_swapchain(),
		    surface_properties.currentExtent);

		update_swapchain(std::move(new_swapchain));

		surface_extent = surface_properties.currentExtent;
	}
}

Swapchain &RenderContext::get_swapchain()
{
	assert(swapchain && "Swapchain is not valid");
	return *swapchain;
}

VkExtent2D RenderContext::get_surface_extent() const
{
	return surface_extent;
}

uint32_t RenderContext::get_active_frame_index() const
{
	return active_frame_index;
}

std::vector<RenderFrame> &RenderContext::get_render_frames()
{
	return frames;
}
}        // namespace vkb
