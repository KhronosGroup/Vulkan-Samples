/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "platform/window.h"

namespace vkb
{
VkFormat RenderContext::DEFAULT_VK_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;

RenderContext::RenderContext(Device                                &device,
                             VkSurfaceKHR                           surface,
                             const Window                          &window,
                             VkPresentModeKHR                       present_mode,
                             const std::vector<VkPresentModeKHR>   &present_mode_priority_list,
                             const std::vector<VkSurfaceFormatKHR> &surface_format_priority_list) :
    device{device}, window{window}, queue{device.get_suitable_graphics_queue()}, surface_extent{window.get_extent().width, window.get_extent().height}
{
	Swapchain::set_present_mode_priority_list(present_mode_priority_list);
	Swapchain::set_surface_format_priority_list(surface_format_priority_list);
	if (surface != VK_NULL_HANDLE)
	{
		VkSurfaceCapabilitiesKHR surface_properties;
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.get_gpu().get_handle(),
		                                                   surface,
		                                                   &surface_properties));

		if (surface_properties.currentExtent.width == 0xFFFFFFFF)
		{
			swapchain = std::make_unique<Swapchain>(device, surface, present_mode, surface_extent);
		}
		else
		{
			swapchain = std::make_unique<Swapchain>(device, surface, present_mode);
		}
	}
}

void RenderContext::prepare(size_t thread_count, RenderTarget::CreateFunc create_render_target_func)
{
	device.wait_idle();

	if (swapchain)
	{
		surface_extent = swapchain->get_extent();

		VkExtent3D extent{surface_extent.width, surface_extent.height, 1};

		for (auto &image_handle : swapchain->get_images())
		{
			auto swapchain_image = core::Image{
			    device, image_handle,
			    extent,
			    swapchain->get_format(),
			    swapchain->get_usage()};
			auto render_target = create_render_target_func(std::move(swapchain_image));
			frames.emplace_back(std::make_unique<RenderFrame>(device, std::move(render_target), thread_count));
		}
	}
	else
	{
		// Otherwise, create a single RenderFrame
		swapchain = nullptr;

		auto color_image = core::Image{device,
		                               VkExtent3D{surface_extent.width, surface_extent.height, 1},
		                               DEFAULT_VK_FORMAT,        // We can use any format here that we like
		                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		                               VMA_MEMORY_USAGE_GPU_ONLY};

		auto render_target = create_render_target_func(std::move(color_image));
		frames.emplace_back(std::make_unique<RenderFrame>(device, std::move(render_target), thread_count));
	}

	this->create_render_target_func = create_render_target_func;
	this->thread_count              = thread_count;
	this->prepared                  = true;
}

VkFormat RenderContext::get_format() const
{
	VkFormat format = DEFAULT_VK_FORMAT;

	if (swapchain)
	{
		format = swapchain->get_format();
	}

	return format;
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

	auto width  = extent.width;
	auto height = extent.height;
	if (transform == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR || transform == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR)
	{
		// Pre-rotation: always use native orientation i.e. if rotated, use width and height of identity transform
		std::swap(width, height);
	}

	swapchain = std::make_unique<Swapchain>(*swapchain, VkExtent2D{width, height}, transform);

	// Save the preTransform attribute for future rotations
	pre_transform = transform;

	recreate();
}

void RenderContext::update_swapchain(const VkImageCompressionFlagBitsEXT compression, const VkImageCompressionFixedRateFlagsEXT compression_fixed_rate)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains compression in headless mode, skipping.");
		return;
	}

	device.get_resource_cache().clear_framebuffers();

	swapchain = std::make_unique<Swapchain>(*swapchain, compression, compression_fixed_rate);

	recreate();
}

void RenderContext::recreate()
{
	LOGI("Recreated swapchain");

	VkExtent2D swapchain_extent = swapchain->get_extent();
	VkExtent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

	auto frame_it = frames.begin();

	for (auto &image_handle : swapchain->get_images())
	{
		core::Image swapchain_image{device, image_handle,
		                            extent,
		                            swapchain->get_format(),
		                            swapchain->get_usage()};

		auto render_target = create_render_target_func(std::move(swapchain_image));

		if (frame_it != frames.end())
		{
			(*frame_it)->update_render_target(std::move(render_target));
		}
		else
		{
			// Create a new frame if the new swapchain has more images than current frames
			frames.emplace_back(std::make_unique<RenderFrame>(device, std::move(render_target), thread_count));
		}

		++frame_it;
	}

	device.get_resource_cache().clear_framebuffers();
}

bool RenderContext::handle_surface_changes(bool force_update)
{
	if (!swapchain)
	{
		LOGW("Can't handle surface changes in headless mode, skipping.");
		return false;
	}

	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.get_gpu().get_handle(),
	                                                   swapchain->get_surface(),
	                                                   &surface_properties));

	if (surface_properties.currentExtent.width == 0xFFFFFFFF)
	{
		return false;
	}

	// Only recreate the swapchain if the dimensions have changed;
	// handle_surface_changes() is called on VK_SUBOPTIMAL_KHR,
	// which might not be due to a surface resize
	if (surface_properties.currentExtent.width != surface_extent.width ||
	    surface_properties.currentExtent.height != surface_extent.height ||
	    force_update)
	{
		// Recreate swapchain
		device.wait_idle();

		update_swapchain(surface_properties.currentExtent, pre_transform);

		surface_extent = surface_properties.currentExtent;

		return true;
	}

	return false;
}

CommandBuffer &RenderContext::begin(CommandBuffer::ResetMode reset_mode)
{
	assert(prepared && "RenderContext not prepared for rendering, call prepare()");

	if (!frame_active)
	{
		begin_frame();
	}

	if (acquired_semaphore == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Couldn't begin frame");
	}

	const auto &queue = device.get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
	return get_active_frame().request_command_buffer(queue, reset_mode);
}

void RenderContext::submit(CommandBuffer &command_buffer)
{
	submit({&command_buffer});
}

void RenderContext::submit(const std::vector<CommandBuffer *> &command_buffers)
{
	assert(frame_active && "RenderContext is inactive, cannot submit command buffer. Please call begin()");

	VkSemaphore render_semaphore = VK_NULL_HANDLE;

	if (swapchain)
	{
		assert(acquired_semaphore && "We do not have acquired_semaphore, it was probably consumed?\n");
		render_semaphore = submit(queue, command_buffers, acquired_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}
	else
	{
		submit(queue, command_buffers);
	}

	end_frame(render_semaphore);
}

void RenderContext::begin_frame()
{
	// Only handle surface changes if a swapchain exists
	if (swapchain)
	{
		handle_surface_changes();
	}

	assert(!frame_active && "Frame is still active, please call end_frame");

	assert(active_frame_index < frames.size());
	auto &prev_frame = *frames[active_frame_index];

	// We will use the acquired semaphore in a different frame context,
	// so we need to hold ownership.
	acquired_semaphore = prev_frame.request_semaphore_with_ownership();

	if (swapchain)
	{
		auto result = swapchain->acquire_next_image(active_frame_index, acquired_semaphore, VK_NULL_HANDLE);

		if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			bool swapchain_updated = handle_surface_changes(result == VK_ERROR_OUT_OF_DATE_KHR);

			if (swapchain_updated)
			{
				result = swapchain->acquire_next_image(active_frame_index, acquired_semaphore, VK_NULL_HANDLE);
			}
		}

		if (result != VK_SUCCESS)
		{
			prev_frame.reset();
			return;
		}
	}

	// Now the frame is active again
	frame_active = true;

	// Wait on all resource to be freed from the previous render to this frame
	wait_frame();
}

VkSemaphore RenderContext::submit(const Queue &queue, const std::vector<CommandBuffer *> &command_buffers, VkSemaphore wait_semaphore, VkPipelineStageFlags wait_pipeline_stage)
{
	std::vector<VkCommandBuffer> cmd_buf_handles(command_buffers.size(), VK_NULL_HANDLE);
	std::transform(command_buffers.begin(), command_buffers.end(), cmd_buf_handles.begin(), [](const CommandBuffer *cmd_buf) { return cmd_buf->get_handle(); });

	RenderFrame &frame = get_active_frame();

	VkSemaphore signal_semaphore = frame.request_semaphore();

	VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};

	submit_info.commandBufferCount = to_u32(cmd_buf_handles.size());
	submit_info.pCommandBuffers    = cmd_buf_handles.data();

	if (wait_semaphore != VK_NULL_HANDLE)
	{
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores    = &wait_semaphore;
		submit_info.pWaitDstStageMask  = &wait_pipeline_stage;
	}

	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = &signal_semaphore;

	VkFence fence = frame.request_fence();

	queue.submit({submit_info}, fence);

	return signal_semaphore;
}

void RenderContext::submit(const Queue &queue, const std::vector<CommandBuffer *> &command_buffers)
{
	std::vector<VkCommandBuffer> cmd_buf_handles(command_buffers.size(), VK_NULL_HANDLE);
	std::transform(command_buffers.begin(), command_buffers.end(), cmd_buf_handles.begin(), [](const CommandBuffer *cmd_buf) { return cmd_buf->get_handle(); });

	RenderFrame &frame = get_active_frame();

	VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};

	submit_info.commandBufferCount = to_u32(cmd_buf_handles.size());
	submit_info.pCommandBuffers    = cmd_buf_handles.data();

	VkFence fence = frame.request_fence();

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

	if (swapchain)
	{
		VkSwapchainKHR vk_swapchain = swapchain->get_handle();

		VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};

		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores    = &semaphore;
		present_info.swapchainCount     = 1;
		present_info.pSwapchains        = &vk_swapchain;
		present_info.pImageIndices      = &active_frame_index;

		VkDisplayPresentInfoKHR disp_present_info{};
		if (device.is_extension_supported(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) &&
		    window.get_display_present_info(&disp_present_info, surface_extent.width, surface_extent.height))
		{
			// Add display present info if supported and wanted
			present_info.pNext = &disp_present_info;
		}

		VkResult result = queue.present(present_info);

		if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			handle_surface_changes();
		}
	}

	// Frame is not active anymore
	if (acquired_semaphore)
	{
		release_owned_semaphore(acquired_semaphore);
		acquired_semaphore = VK_NULL_HANDLE;
	}
	frame_active = false;
}

VkSemaphore RenderContext::consume_acquired_semaphore()
{
	assert(frame_active && "Frame is not active, please call begin_frame");
	auto sem           = acquired_semaphore;
	acquired_semaphore = VK_NULL_HANDLE;
	return sem;
}

RenderFrame &RenderContext::get_active_frame()
{
	assert(frame_active && "Frame is not active, please call begin_frame");
	assert(active_frame_index < frames.size());
	return *frames[active_frame_index];
}

uint32_t RenderContext::get_active_frame_index()
{
	assert(frame_active && "Frame is not active, please call begin_frame");
	return active_frame_index;
}

RenderFrame &RenderContext::get_last_rendered_frame()
{
	assert(!frame_active && "Frame is still active, please call end_frame");
	assert(active_frame_index < frames.size());
	return *frames[active_frame_index];
}

VkSemaphore RenderContext::request_semaphore()
{
	RenderFrame &frame = get_active_frame();
	return frame.request_semaphore();
}

VkSemaphore RenderContext::request_semaphore_with_ownership()
{
	RenderFrame &frame = get_active_frame();
	return frame.request_semaphore_with_ownership();
}

void RenderContext::release_owned_semaphore(VkSemaphore semaphore)
{
	RenderFrame &frame = get_active_frame();
	frame.release_owned_semaphore(semaphore);
}

Device &RenderContext::get_device()
{
	return device;
}

void RenderContext::recreate_swapchain()
{
	device.wait_idle();
	device.get_resource_cache().clear_framebuffers();

	VkExtent2D swapchain_extent = swapchain->get_extent();
	VkExtent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

	auto frame_it = frames.begin();

	for (auto &image_handle : swapchain->get_images())
	{
		core::Image swapchain_image{device, image_handle,
		                            extent,
		                            swapchain->get_format(),
		                            swapchain->get_usage()};

		auto render_target = create_render_target_func(std::move(swapchain_image));
		(*frame_it)->update_render_target(std::move(render_target));

		++frame_it;
	}
}

bool RenderContext::has_swapchain()
{
	return swapchain != nullptr;
}

Swapchain const &RenderContext::get_swapchain() const
{
	assert(swapchain && "Swapchain is not valid");
	return *swapchain;
}

VkExtent2D const &RenderContext::get_surface_extent() const
{
	return surface_extent;
}

uint32_t RenderContext::get_active_frame_index() const
{
	return active_frame_index;
}

std::vector<std::unique_ptr<RenderFrame>> &RenderContext::get_render_frames()
{
	return frames;
}

}        // namespace vkb
