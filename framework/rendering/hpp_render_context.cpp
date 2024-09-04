/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_render_context.h"

#include <core/hpp_image.h>

namespace vkb
{
namespace rendering
{
vk::Format HPPRenderContext::DEFAULT_VK_FORMAT = vk::Format::eR8G8B8A8Srgb;

HPPRenderContext::HPPRenderContext(vkb::core::HPPDevice                    &device,
                                   vk::SurfaceKHR                           surface,
                                   const vkb::Window                       &window,
                                   vk::PresentModeKHR                       present_mode,
                                   std::vector<vk::PresentModeKHR> const   &present_mode_priority_list,
                                   std::vector<vk::SurfaceFormatKHR> const &surface_format_priority_list) :
    device{device}, window{window}, queue{device.get_suitable_graphics_queue()}, surface_extent{window.get_extent().width, window.get_extent().height}
{
	if (surface)
	{
		vk::SurfaceCapabilitiesKHR surface_properties = device.get_gpu().get_handle().getSurfaceCapabilitiesKHR(surface);

		if (surface_properties.currentExtent.width == 0xFFFFFFFF)
		{
			swapchain =
			    std::make_unique<vkb::core::HPPSwapchain>(device, surface, present_mode, present_mode_priority_list, surface_format_priority_list, surface_extent);
		}
		else
		{
			swapchain = std::make_unique<vkb::core::HPPSwapchain>(device, surface, present_mode, present_mode_priority_list, surface_format_priority_list);
		}
	}
}

void HPPRenderContext::prepare(size_t thread_count, vkb::rendering::HPPRenderTarget::CreateFunc create_render_target_func)
{
	device.get_handle().waitIdle();

	if (swapchain)
	{
		surface_extent = swapchain->get_extent();

		vk::Extent3D extent{surface_extent.width, surface_extent.height, 1};

		for (auto &image_handle : swapchain->get_images())
		{
			auto swapchain_image = core::HPPImage{device, image_handle, extent, swapchain->get_format(), swapchain->get_usage()};
			auto render_target   = create_render_target_func(std::move(swapchain_image));
			frames.emplace_back(std::make_unique<vkb::rendering::HPPRenderFrame>(device, std::move(render_target), thread_count));
		}
	}
	else
	{
		// Otherwise, create a single RenderFrame
		swapchain = nullptr;

		auto color_image = vkb::core::HPPImage{device,
		                                       vk::Extent3D{surface_extent.width, surface_extent.height, 1},
		                                       DEFAULT_VK_FORMAT,        // We can use any format here that we like
		                                       vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
		                                       VMA_MEMORY_USAGE_GPU_ONLY};

		auto render_target = create_render_target_func(std::move(color_image));
		frames.emplace_back(std::make_unique<vkb::rendering::HPPRenderFrame>(device, std::move(render_target), thread_count));
	}

	this->create_render_target_func = create_render_target_func;
	this->thread_count              = thread_count;
	this->prepared                  = true;
}

vk::Format HPPRenderContext::get_format() const
{
	return swapchain ? swapchain->get_format() : DEFAULT_VK_FORMAT;
}

void HPPRenderContext::update_swapchain(const vk::Extent2D &extent)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains extent in headless mode, skipping.");
		return;
	}

	device.get_resource_cache().clear_framebuffers();

	swapchain = std::make_unique<vkb::core::HPPSwapchain>(*swapchain, extent);

	recreate();
}

void HPPRenderContext::update_swapchain(const uint32_t image_count)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains image count in headless mode, skipping.");
		return;
	}

	device.get_resource_cache().clear_framebuffers();

	device.get_handle().waitIdle();

	swapchain = std::make_unique<vkb::core::HPPSwapchain>(*swapchain, image_count);

	recreate();
}

void HPPRenderContext::update_swapchain(const std::set<vk::ImageUsageFlagBits> &image_usage_flags)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains image usage in headless mode, skipping.");
		return;
	}

	device.get_resource_cache().clear_framebuffers();

	swapchain = std::make_unique<vkb::core::HPPSwapchain>(*swapchain, image_usage_flags);

	recreate();
}

void HPPRenderContext::update_swapchain(const vk::Extent2D &extent, const vk::SurfaceTransformFlagBitsKHR transform)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains extent and surface transform in headless mode, skipping.");
		return;
	}

	device.get_resource_cache().clear_framebuffers();

	auto width  = extent.width;
	auto height = extent.height;
	if (transform == vk::SurfaceTransformFlagBitsKHR::eRotate90 || transform == vk::SurfaceTransformFlagBitsKHR::eRotate270)
	{
		// Pre-rotation: always use native orientation i.e. if rotated, use width and height of identity transform
		std::swap(width, height);
	}

	swapchain = std::make_unique<vkb::core::HPPSwapchain>(*swapchain, vk::Extent2D{width, height}, transform);

	// Save the preTransform attribute for future rotations
	pre_transform = transform;

	recreate();
}

void HPPRenderContext::recreate()
{
	LOGI("Recreated swapchain");

	vk::Extent2D swapchain_extent = swapchain->get_extent();
	vk::Extent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

	auto frame_it = frames.begin();

	for (auto &image_handle : swapchain->get_images())
	{
		vkb::core::HPPImage swapchain_image{device, image_handle, extent, swapchain->get_format(), swapchain->get_usage()};

		auto render_target = create_render_target_func(std::move(swapchain_image));

		if (frame_it != frames.end())
		{
			(*frame_it)->update_render_target(std::move(render_target));
		}
		else
		{
			// Create a new frame if the new swapchain has more images than current frames
			frames.emplace_back(std::make_unique<vkb::rendering::HPPRenderFrame>(device, std::move(render_target), thread_count));
		}

		++frame_it;
	}

	device.get_resource_cache().clear_framebuffers();
}

bool HPPRenderContext::handle_surface_changes(bool force_update)
{
	if (!swapchain)
	{
		LOGW("Can't handle surface changes in headless mode, skipping.");
		return false;
	}

	vk::SurfaceCapabilitiesKHR surface_properties = device.get_gpu().get_handle().getSurfaceCapabilitiesKHR(swapchain->get_surface());

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
		device.get_handle().waitIdle();

		update_swapchain(surface_properties.currentExtent, pre_transform);

		surface_extent = surface_properties.currentExtent;

		return true;
	}

	return false;
}

vkb::core::HPPCommandBuffer &HPPRenderContext::begin(vkb::core::HPPCommandBuffer::ResetMode reset_mode)
{
	assert(prepared && "HPPRenderContext not prepared for rendering, call prepare()");

	if (!frame_active)
	{
		begin_frame();
	}

	if (!acquired_semaphore)
	{
		throw std::runtime_error("Couldn't begin frame");
	}

	const auto &queue = device.get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);
	return get_active_frame().request_command_buffer(queue, reset_mode);
}

void HPPRenderContext::submit(vkb::core::HPPCommandBuffer &command_buffer)
{
	submit({&command_buffer});
}

void HPPRenderContext::submit(const std::vector<vkb::core::HPPCommandBuffer *> &command_buffers)
{
	assert(frame_active && "HPPRenderContext is inactive, cannot submit command buffer. Please call begin()");

	vk::Semaphore render_semaphore;

	if (swapchain)
	{
		assert(acquired_semaphore && "We do not have acquired_semaphore, it was probably consumed?\n");
		render_semaphore = submit(queue, command_buffers, acquired_semaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput);
	}
	else
	{
		submit(queue, command_buffers);
	}

	end_frame(render_semaphore);
}

void HPPRenderContext::begin_frame()
{
	// Only handle surface changes if a swapchain exists
	if (swapchain)
	{
		handle_surface_changes();
	}

	assert(!frame_active && "Frame is still active, please call end_frame");

	auto &prev_frame = *frames[active_frame_index];

	// We will use the acquired semaphore in a different frame context,
	// so we need to hold ownership.
	acquired_semaphore = prev_frame.request_semaphore_with_ownership();

	if (swapchain)
	{
		vk::Result result;
		try
		{
			std::tie(result, active_frame_index) = swapchain->acquire_next_image(acquired_semaphore);
		}
		catch (vk::OutOfDateKHRError & /*err*/)
		{
			result = vk::Result::eErrorOutOfDateKHR;
		}

		if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
		{
#if defined(PLATFORM__MACOS)
			// On Apple platforms, force swapchain update on both eSuboptimalKHR and eErrorOutOfDateKHR
			// eSuboptimalKHR may occur on macOS/iOS following changes to swapchain other than extent/size
			bool swapchain_updated = handle_surface_changes(true);
#else
			bool swapchain_updated = handle_surface_changes(result == vk::Result::eErrorOutOfDateKHR);
#endif

			if (swapchain_updated)
			{
				std::tie(result, active_frame_index) = swapchain->acquire_next_image(acquired_semaphore);
			}
		}

		if (result != vk::Result::eSuccess)
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

vk::Semaphore HPPRenderContext::submit(const vkb::core::HPPQueue                        &queue,
                                       const std::vector<vkb::core::HPPCommandBuffer *> &command_buffers,
                                       vk::Semaphore                                     wait_semaphore,
                                       vk::PipelineStageFlags                            wait_pipeline_stage)
{
	std::vector<vk::CommandBuffer> cmd_buf_handles(command_buffers.size(), nullptr);
	std::transform(command_buffers.begin(), command_buffers.end(), cmd_buf_handles.begin(), [](const vkb::core::HPPCommandBuffer *cmd_buf) { return cmd_buf->get_handle(); });

	vkb::rendering::HPPRenderFrame &frame = get_active_frame();

	vk::Semaphore signal_semaphore = frame.request_semaphore();

	vk::SubmitInfo submit_info(nullptr, nullptr, cmd_buf_handles, signal_semaphore);
	if (wait_semaphore)
	{
		submit_info.setWaitSemaphores(wait_semaphore);
		submit_info.pWaitDstStageMask = &wait_pipeline_stage;
	}

	vk::Fence fence = frame.request_fence();

	queue.get_handle().submit(submit_info, fence);

	return signal_semaphore;
}

void HPPRenderContext::submit(const vkb::core::HPPQueue &queue, const std::vector<vkb::core::HPPCommandBuffer *> &command_buffers)
{
	std::vector<vk::CommandBuffer> cmd_buf_handles(command_buffers.size(), nullptr);
	std::transform(command_buffers.begin(), command_buffers.end(), cmd_buf_handles.begin(), [](const vkb::core::HPPCommandBuffer *cmd_buf) { return cmd_buf->get_handle(); });

	vkb::rendering::HPPRenderFrame &frame = get_active_frame();

	vk::SubmitInfo submit_info(nullptr, nullptr, cmd_buf_handles);

	vk::Fence fence = frame.request_fence();

	queue.get_handle().submit(submit_info, fence);
}

void HPPRenderContext::wait_frame()
{
	get_active_frame().reset();
}

void HPPRenderContext::end_frame(vk::Semaphore semaphore)
{
	assert(frame_active && "Frame is not active, please call begin_frame");

	if (swapchain)
	{
		vk::SwapchainKHR   vk_swapchain = swapchain->get_handle();
		vk::PresentInfoKHR present_info(semaphore, vk_swapchain, active_frame_index);

		vk::DisplayPresentInfoKHR disp_present_info;
		if (device.is_extension_supported(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) &&
		    window.get_display_present_info(reinterpret_cast<VkDisplayPresentInfoKHR *>(&disp_present_info), surface_extent.width, surface_extent.height))
		{
			// Add display present info if supported and wanted
			present_info.pNext = &disp_present_info;
		}

		vk::Result result;
		try
		{
			result = queue.present(present_info);
		}
		catch (vk::OutOfDateKHRError & /*err*/)
		{
			result = vk::Result::eErrorOutOfDateKHR;
		}

		if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
		{
			handle_surface_changes();
		}
	}

	// Frame is not active anymore
	if (acquired_semaphore)
	{
		release_owned_semaphore(acquired_semaphore);
		acquired_semaphore = nullptr;
	}
	frame_active = false;
}

vk::Semaphore HPPRenderContext::consume_acquired_semaphore()
{
	assert(frame_active && "Frame is not active, please call begin_frame");
	return std::exchange(acquired_semaphore, nullptr);
}

vkb::rendering::HPPRenderFrame &HPPRenderContext::get_active_frame()
{
	assert(frame_active && "Frame is not active, please call begin_frame");
	return *frames[active_frame_index];
}

uint32_t HPPRenderContext::get_active_frame_index()
{
	assert(frame_active && "Frame is not active, please call begin_frame");
	return active_frame_index;
}

vkb::rendering::HPPRenderFrame &HPPRenderContext::get_last_rendered_frame()
{
	assert(!frame_active && "Frame is still active, please call end_frame");
	return *frames[active_frame_index];
}

vk::Semaphore HPPRenderContext::request_semaphore()
{
	return get_active_frame().request_semaphore();
}

vk::Semaphore HPPRenderContext::request_semaphore_with_ownership()
{
	return get_active_frame().request_semaphore_with_ownership();
}

void HPPRenderContext::release_owned_semaphore(vk::Semaphore semaphore)
{
	get_active_frame().release_owned_semaphore(semaphore);
}

vkb::core::HPPDevice &HPPRenderContext::get_device()
{
	return device;
}

void HPPRenderContext::recreate_swapchain()
{
	device.get_handle().waitIdle();
	device.get_resource_cache().clear_framebuffers();

	vk::Extent2D swapchain_extent = swapchain->get_extent();
	vk::Extent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

	auto frame_it = frames.begin();

	for (auto &image_handle : swapchain->get_images())
	{
		vkb::core::HPPImage swapchain_image{device, image_handle, extent, swapchain->get_format(), swapchain->get_usage()};
		auto                render_target = create_render_target_func(std::move(swapchain_image));
		(*frame_it)->update_render_target(std::move(render_target));

		++frame_it;
	}
}

bool HPPRenderContext::has_swapchain()
{
	return swapchain != nullptr;
}

vkb::core::HPPSwapchain const &HPPRenderContext::get_swapchain() const
{
	assert(swapchain && "Swapchain is not valid");
	return *swapchain;
}

vk::Extent2D const &HPPRenderContext::get_surface_extent() const
{
	return surface_extent;
}

uint32_t HPPRenderContext::get_active_frame_index() const
{
	return active_frame_index;
}

std::vector<std::unique_ptr<vkb::rendering::HPPRenderFrame>> &HPPRenderContext::get_render_frames()
{
	return frames;
}

}        // namespace rendering
}        // namespace vkb
