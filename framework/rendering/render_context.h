/* Copyright (c) 2019-2025, Arm Limited and Contributors
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

#include "common/vk_common.h"
#include "core/device.h"
#include "core/hpp_swapchain.h"
#include "platform/window.h"
#include "rendering/hpp_render_target.h"
#include "rendering/render_frame.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
class Queue;
class RenderTarget;
class Swapchain;

namespace core
{
template <BindingType bindingType>
class CommandBuffer;

class HPPQueue;
}        // namespace core

namespace rendering
{
template <BindingType bindingType>
class RenderFrame;
using RenderFrameCpp = RenderFrame<BindingType::Cpp>;

/**
 * @brief RenderContext acts as a frame manager for the sample, with a lifetime that is the
 * same as that of the Application itself. It acts as a container for RenderFrame objects,
 * swapping between them (begin_frame, end_frame) and forwarding requests for Vulkan resources
 * to the active frame. Note that it's guaranteed that there is always an active frame.
 * More than one frame can be in-flight in the GPU, thus the need for per-frame resources.
 *
 * It requires a Device to be valid on creation, and will take control of a given Swapchain.
 *
 * For normal rendering (using a swapchain), the RenderContext can be created by passing in a
 * swapchain. A RenderFrame will then be created for each Swapchain image.
 *
 * For offscreen rendering (no swapchain), the RenderContext can be given a valid Device, and
 * a width and height. A single RenderFrame will then be created.
 */
template <vkb::BindingType bindingType>
class RenderContext
{
  public:
	// The format to use for the RenderTargets if a swapchain isn't created
	static inline vk::Format DEFAULT_VK_FORMAT = vk::Format::eR8G8B8A8Srgb;

  public:
	using QueueType        = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPQueue, vkb::Queue>::type;
	using RenderTargetType = typename std::conditional<bindingType == BindingType::Cpp, vkb::rendering::HPPRenderTarget, vkb::RenderTarget>::type;
	using SwapchainType    = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPSwapchain, vkb::Swapchain>::type;

	using Extent2DType                       = typename std::conditional<bindingType == BindingType::Cpp, vk::Extent2D, VkExtent2D>::type;
	using FormatType                         = typename std::conditional<bindingType == BindingType::Cpp, vk::Format, VkFormat>::type;
	using ImageCompressionFixedRateFlagsType = typename std::conditional<bindingType == BindingType::Cpp, vk::ImageCompressionFixedRateFlagsEXT, VkImageCompressionFixedRateFlagsEXT>::type;
	using ImageCompressionFlagsType          = typename std::conditional<bindingType == BindingType::Cpp, vk::ImageCompressionFlagsEXT, VkImageCompressionFlagsEXT>::type;
	using ImageUsageFlagBitsType             = typename std::conditional<bindingType == BindingType::Cpp, vk::ImageUsageFlagBits, VkImageUsageFlagBits>::type;
	using PipelineStageFlagsType             = typename std::conditional<bindingType == BindingType::Cpp, vk::PipelineStageFlags, VkPipelineStageFlags>::type;
	using PresentModeType                    = typename std::conditional<bindingType == BindingType::Cpp, vk::PresentModeKHR, VkPresentModeKHR>::type;
	using SemaphoreType                      = typename std::conditional<bindingType == BindingType::Cpp, vk::Semaphore, VkSemaphore>::type;
	using SurfaceFormatType                  = typename std::conditional<bindingType == BindingType::Cpp, vk::SurfaceFormatKHR, VkSurfaceFormatKHR>::type;
	using SurfaceTransformFlagBitsType       = typename std::conditional<bindingType == BindingType::Cpp, vk::SurfaceTransformFlagBitsKHR, VkSurfaceTransformFlagBitsKHR>::type;
	using SurfaceType                        = typename std::conditional<bindingType == BindingType::Cpp, vk::SurfaceKHR, VkSurfaceKHR>::type;

	/**
	 * @brief Constructor
	 * @param device A valid device
	 * @param surface A surface, VK_NULL_HANDLE if in offscreen mode
	 * @param window The window where the surface was created
	 * @param present_mode Requests to set the present mode of the swapchain
	 * @param present_mode_priority_list The order in which the swapchain prioritizes selecting its present mode
	 * @param surface_format_priority_list The order in which the swapchain prioritizes selecting its surface format
	 */
	RenderContext(vkb::core::DeviceCpp                    &device,
	              vk::SurfaceKHR                           surface,
	              const vkb::Window                       &window,
	              vk::PresentModeKHR                       present_mode                 = vk::PresentModeKHR::eFifo,
	              const std::vector<vk::PresentModeKHR>   &present_mode_priority_list   = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox},
	              const std::vector<vk::SurfaceFormatKHR> &surface_format_priority_list = {{vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
	                                                                                       {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}});
	RenderContext(vkb::core::DeviceC                    &device,
	              VkSurfaceKHR                           surface,
	              const vkb::Window                     &window,
	              VkPresentModeKHR                       present_mode                 = VK_PRESENT_MODE_FIFO_KHR,
	              const std::vector<VkPresentModeKHR>   &present_mode_priority_list   = {VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_MAILBOX_KHR},
	              const std::vector<VkSurfaceFormatKHR> &surface_format_priority_list = {{VK_FORMAT_R8G8B8A8_SRGB, VK_COLORSPACE_SRGB_NONLINEAR_KHR},
	                                                                                     {VK_FORMAT_B8G8R8A8_SRGB, VK_COLORSPACE_SRGB_NONLINEAR_KHR}});

	RenderContext(const RenderContext &) = delete;
	RenderContext(RenderContext &&)      = delete;

	virtual ~RenderContext() = default;

	RenderContext &operator=(const RenderContext &) = delete;
	RenderContext &operator=(RenderContext &&)      = delete;

  public:
	/**
	 * @brief Prepares the next available frame for rendering
	 * @param reset_mode How to reset the command buffer
	 * @returns A valid command buffer to record commands to be submitted
	 * Also ensures that there is an active frame if there is no existing active frame already
	 */
	std::shared_ptr<vkb::core::CommandBuffer<bindingType>> begin(vkb::CommandBufferResetMode reset_mode = vkb::CommandBufferResetMode::ResetPool);

	/**
	 * @brief begin_frame
	 */
	void begin_frame();

	/**
	 * @brief Returns the WSI acquire semaphore. Only to be used in very special circumstances.
	 * @return The WSI acquire semaphore.
	 */
	SemaphoreType consume_acquired_semaphore();

	void end_frame(SemaphoreType semaphore);

	/**
	 * @brief An error should be raised if the frame is not active.
	 *        A frame is active after @ref begin_frame has been called.
	 * @return The current active frame
	 */
	vkb::rendering::RenderFrame<bindingType> &get_active_frame();

	/**
	 * @brief An error should be raised if the frame is not active.
	 *        A frame is active after @ref begin_frame has been called.
	 * @return The current active frame index
	 */
	uint32_t get_active_frame_index() const;

	vkb::core::Device<bindingType> &get_device();

	/**
	 * @brief Returns the format that the RenderTargets are created with within the RenderContext
	 */
	FormatType get_format() const;

	/**
	 * @brief An error should be raised if a frame is active.
	 *        A frame is active after @ref begin_frame has been called.
	 * @return The previous frame
	 */
	vkb::rendering::RenderFrame<bindingType> &get_last_rendered_frame();

	std::vector<std::unique_ptr<vkb::rendering::RenderFrame<bindingType>>> &get_render_frames();

	Extent2DType const &get_surface_extent() const;

	SwapchainType const &get_swapchain() const;

	/**
	 * @brief Handles surface changes, only applicable if the render_context makes use of a swapchain
	 */
	virtual bool handle_surface_changes(bool force_update = false);

	/**
	 * @returns True if a valid swapchain exists in the RenderContext
	 */
	bool has_swapchain();

	/**
	 * @brief Prepares the RenderFrames for rendering
	 * @param thread_count The number of threads in the application, necessary to allocate this many resource pools for each RenderFrame
	 * @param create_render_target_func A function delegate, used to create a RenderTarget
	 */
	void prepare(size_t thread_count = 1, typename RenderTargetType::CreateFunc create_render_target_func = RenderTargetType::DEFAULT_CREATE_FUNC);

	/**
	 * @brief Recreates the RenderFrames, called after every update
	 */
	void recreate();

	/**
	 * @brief Recreates the swapchain
	 */
	void recreate_swapchain();

	void          release_owned_semaphore(SemaphoreType semaphore);
	SemaphoreType request_semaphore();
	SemaphoreType request_semaphore_with_ownership();

	/**
	 * @brief Submits the command buffer to the right queue
	 * @param command_buffer A command buffer containing recorded commands
	 */
	void submit(std::shared_ptr<vkb::core::CommandBuffer<bindingType>> command_buffer);

	/**
	 * @brief Submits multiple command buffers to the right queue
	 * @param command_buffers Command buffers containing recorded commands
	 */
	void submit(const std::vector<std::shared_ptr<vkb::core::CommandBuffer<bindingType>>> &command_buffers);

	SemaphoreType submit(const QueueType                                                           &queue,
	                     const std::vector<std::shared_ptr<vkb::core::CommandBuffer<bindingType>>> &command_buffers,
	                     SemaphoreType                                                              wait_semaphore,
	                     PipelineStageFlagsType                                                     wait_pipeline_stage);

	/**
	 * @brief Submits a command buffer related to a frame to a queue
	 */
	void submit(const QueueType &queue, const std::vector<std::shared_ptr<vkb::core::CommandBuffer<bindingType>>> &command_buffers);

	/**
	 * @brief Updates the swapchains extent, if a swapchain exists
	 * @param extent The width and height of the new swapchain images
	 */
	void update_swapchain(const Extent2DType &extent);

	/**
	 * @brief Updates the swapchains image count, if a swapchain exists
	 * @param image_count The amount of images in the new swapchain
	 */
	void update_swapchain(const uint32_t image_count);

	/**
	 * @brief Updates the swapchains image usage, if a swapchain exists
	 * @param image_usage_flags The usage flags the new swapchain images will have
	 */
	void update_swapchain(const std::set<ImageUsageFlagBitsType> &image_usage_flags);

	/**
	 * @brief Updates the swapchains extent and surface transform, if a swapchain exists
	 * @param extent The width and height of the new swapchain images
	 * @param transform The surface transform flags
	 */
	void update_swapchain(const Extent2DType &extent, const SurfaceTransformFlagBitsType transform);

	/**
	 * @brief Updates the swapchain's compression settings, if a swapchain exists
	 * @param compression The compression to use for swapchain images (default, fixed-rate, none)
	 * @param compression_fixed_rate The rate to use, if compression is fixed-rate
	 */
	void update_swapchain(const ImageCompressionFlagsType compression, const ImageCompressionFixedRateFlagsType compression_fixed_rate);

	/**
	 * @brief Waits a frame to finish its rendering
	 */
	virtual void wait_frame();

  private:
	void          initialize_swapchain(vk::SurfaceKHR surface, vk::PresentModeKHR present_movde, std::vector<vk::PresentModeKHR> const &present_mode_priority_list, std::vector<vk::SurfaceFormatKHR> const &surface_format_priority_list);
	void          submit_impl(const std::vector<std::shared_ptr<vkb::core::CommandBufferCpp>> &command_buffers);
	vk::Semaphore submit_impl(vkb::core::HPPQueue const                                       &queue,
	                          std::vector<std::shared_ptr<vkb::core::CommandBufferCpp>> const &command_buffers,
	                          vk::Semaphore                                                    wait_semaphore,
	                          vk::PipelineStageFlags                                           wait_pipeline_stage);
	void          submit_impl(vkb::core::HPPQueue const &queue, std::vector<std::shared_ptr<vkb::core::CommandBufferCpp>> const &command_buffers);
	void          update_swapchain_impl(vk::Extent2D const &extent, vk::SurfaceTransformFlagBitsKHR transform);

  private:
	vk::Semaphore                                                acquired_semaphore;
	uint32_t                                                     active_frame_index        = 0;        // Current active frame index
	HPPRenderTarget::CreateFunc                                  create_render_target_func = HPPRenderTarget::DEFAULT_CREATE_FUNC;
	vkb::core::DeviceCpp                                        &device;
	bool                                                         frame_active = false;        // Whether a frame is active or not
	std::vector<std::unique_ptr<vkb::rendering::RenderFrameCpp>> frames;
	vk::SurfaceTransformFlagBitsKHR                              pre_transform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	bool                                                         prepared      = false;
	const vkb::core::HPPQueue                                   &queue;        // If swapchain exists, then this will be a present supported queue, else a graphics queue
	vk::Extent2D                                                 surface_extent;
	std::unique_ptr<vkb::core::HPPSwapchain>                     swapchain;
	vkb::core::HPPSwapchainProperties                            swapchain_properties;
	size_t                                                       thread_count = 1;
	const vkb::Window                                           &window;
};

using RenderContextC   = RenderContext<vkb::BindingType::C>;
using RenderContextCpp = RenderContext<vkb::BindingType::Cpp>;

// Member function definitions

template <>
inline RenderContextCpp::RenderContext(vkb::core::DeviceCpp                    &device,
                                       vk::SurfaceKHR                           surface,
                                       const vkb::Window                       &window,
                                       vk::PresentModeKHR                       present_mode,
                                       std::vector<vk::PresentModeKHR> const   &present_mode_priority_list,
                                       std::vector<vk::SurfaceFormatKHR> const &surface_format_priority_list) :
    device(device),
    window(window),
    queue(device.get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0)),
    surface_extent{window.get_extent().width, window.get_extent().height}
{
	initialize_swapchain(surface, present_mode, present_mode_priority_list, surface_format_priority_list);
}

template <>
inline RenderContextC::RenderContext(vkb::core::DeviceC                    &device,
                                     VkSurfaceKHR                           surface,
                                     const Window                          &window,
                                     VkPresentModeKHR                       present_mode,
                                     const std::vector<VkPresentModeKHR>   &present_mode_priority_list,
                                     const std::vector<VkSurfaceFormatKHR> &surface_format_priority_list) :
    device(reinterpret_cast<vkb::core::DeviceCpp &>(device)),
    window(window),
    queue(reinterpret_cast<vkb::core::HPPQueue const &>(device.get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0))),
    surface_extent{window.get_extent().width, window.get_extent().height}
{
	initialize_swapchain(static_cast<vk::SurfaceKHR>(surface),
	                     static_cast<vk::PresentModeKHR>(present_mode),
	                     reinterpret_cast<std::vector<vk::PresentModeKHR> const &>(present_mode_priority_list),
	                     reinterpret_cast<std::vector<vk::SurfaceFormatKHR> const &>(surface_format_priority_list));
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::initialize_swapchain(vk::SurfaceKHR                           surface,
                                                             vk::PresentModeKHR                       present_mode,
                                                             std::vector<vk::PresentModeKHR> const   &present_mode_priority_list,
                                                             std::vector<vk::SurfaceFormatKHR> const &surface_format_priority_list)
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

template <vkb::BindingType bindingType>
inline std::shared_ptr<vkb::core::CommandBuffer<bindingType>> RenderContext<bindingType>::begin(vkb::CommandBufferResetMode reset_mode)
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
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return get_active_frame().get_command_pool(queue, reset_mode).request_command_buffer();
	}
	else
	{
		return get_active_frame().get_command_pool(reinterpret_cast<vkb::Queue const &>(queue), reset_mode).request_command_buffer();
	}
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::begin_frame()
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
	acquired_semaphore = prev_frame.get_semaphore_pool().request_semaphore_with_ownership();

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
				// Need to destroy and reallocate acquired_semaphore since it may have already been signaled
				device.get_handle().destroySemaphore(acquired_semaphore);
				acquired_semaphore                   = prev_frame.get_semaphore_pool().request_semaphore_with_ownership();
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

template <vkb::BindingType bindingType>
inline typename RenderContext<bindingType>::SemaphoreType RenderContext<bindingType>::consume_acquired_semaphore()
{
	assert(frame_active && "Frame is not active, please call begin_frame");
	return std::exchange(acquired_semaphore, nullptr);
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::end_frame(SemaphoreType semaphore)
{
	assert(frame_active && "Frame is not active, please call begin_frame");

	if (swapchain)
	{
		vk::SwapchainKHR   vk_swapchain = swapchain->get_handle();
		vk::PresentInfoKHR present_info{.waitSemaphoreCount = 1, .swapchainCount = 1, .pSwapchains = &vk_swapchain, .pImageIndices = &active_frame_index};
		if constexpr (bindingType == BindingType::Cpp)
		{
			present_info.pWaitSemaphores = &semaphore;
		}
		else
		{
			present_info.pWaitSemaphores = reinterpret_cast<vk::Semaphore *>(&semaphore);
		}

		vk::DisplayPresentInfoKHR disp_present_info;
		if (device.get_gpu().is_extension_supported(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) &&
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

template <vkb::BindingType bindingType>
inline vkb::rendering::RenderFrame<bindingType> &RenderContext<bindingType>::get_active_frame()
{
	assert(frame_active && "Frame is not active, please call begin_frame");
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *frames[active_frame_index];
	}
	else
	{
		return reinterpret_cast<vkb::rendering::RenderFrameC &>(*frames[active_frame_index]);
	}
}

template <vkb::BindingType bindingType>
inline uint32_t RenderContext<bindingType>::get_active_frame_index() const
{
	assert(frame_active && "Frame is not active, please call begin_frame");
	return active_frame_index;
}

template <vkb::BindingType bindingType>
inline vkb::core::Device<bindingType> &RenderContext<bindingType>::get_device()
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return device;
	}
	else
	{
		return reinterpret_cast<vkb::core::DeviceC &>(device);
	}
}

template <vkb::BindingType bindingType>
inline typename RenderContext<bindingType>::FormatType RenderContext<bindingType>::get_format() const
{
	vk::Format format = swapchain ? swapchain->get_format() : DEFAULT_VK_FORMAT;
	if constexpr (bindingType == BindingType::Cpp)
	{
		return format;
	}
	else
	{
		return static_cast<VkFormat>(format);
	}
}

template <vkb::BindingType bindingType>
inline vkb::rendering::RenderFrame<bindingType> &RenderContext<bindingType>::get_last_rendered_frame()
{
	assert(!frame_active && "Frame is still active, please call end_frame");
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *frames[active_frame_index];
	}
	else
	{
		return reinterpret_cast<vkb::rendering::RenderFrameC &>(*frames[active_frame_index]);
	}
}

template <vkb::BindingType bindingType>
inline std::vector<std::unique_ptr<vkb::rendering::RenderFrame<bindingType>>> &RenderContext<bindingType>::get_render_frames()
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return frames;
	}
	else
	{
		return reinterpret_cast<std::vector<std::unique_ptr<vkb::rendering::RenderFrameC>> &>(frames);
	}
}

template <vkb::BindingType bindingType>
inline typename RenderContext<bindingType>::Extent2DType const &RenderContext<bindingType>::get_surface_extent() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return surface_extent;
	}
	else
	{
		return *reinterpret_cast<VkExtent2D const *>(&surface_extent);
	}
}

template <vkb::BindingType bindingType>
inline typename RenderContext<bindingType>::SwapchainType const &RenderContext<bindingType>::get_swapchain() const
{
	assert(swapchain && "Swapchain is not valid");
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return *swapchain;
	}
	else
	{
		return reinterpret_cast<vkb::Swapchain const &>(*swapchain);
	}
}

template <vkb::BindingType bindingType>
inline bool RenderContext<bindingType>::handle_surface_changes(bool force_update)
{
	if (!swapchain)
	{
		LOGW("Can't handle surface changes. No swapchain, offscreen rendering detected, skipping.");
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
	if (surface_properties.currentExtent.width != surface_extent.width || surface_properties.currentExtent.height != surface_extent.height || force_update)
	{
		// Recreate swapchain
		device.get_handle().waitIdle();

		update_swapchain_impl(surface_properties.currentExtent, pre_transform);

		surface_extent = surface_properties.currentExtent;

		return true;
	}

	return false;
}

template <vkb::BindingType bindingType>
inline bool RenderContext<bindingType>::has_swapchain()
{
	return swapchain != nullptr;
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::prepare(size_t thread_count, typename RenderTargetType::CreateFunc create_render_target_func_)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		create_render_target_func = create_render_target_func_;
	}
	else
	{
		create_render_target_func = *reinterpret_cast<HPPRenderTarget::CreateFunc *>(&create_render_target_func_);
	}

	device.get_handle().waitIdle();

	if (swapchain)
	{
		surface_extent = swapchain->get_extent();

		vk::Extent3D extent{surface_extent.width, surface_extent.height, 1};

		for (auto &image_handle : swapchain->get_images())
		{
			vkb::core::HPPImage swapchain_image{device, image_handle, extent, swapchain->get_format(), swapchain->get_usage()};
			auto                render_target = create_render_target_func(std::move(swapchain_image));
			frames.emplace_back(std::make_unique<vkb::rendering::RenderFrameCpp>(device, std::move(render_target), thread_count));
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

		std::unique_ptr<HPPRenderTarget> render_target = create_render_target_func(std::move(color_image));
		frames.emplace_back(std::make_unique<vkb::rendering::RenderFrameCpp>(device, std::move(render_target), thread_count));
	}

	this->thread_count = thread_count;
	this->prepared     = true;
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::recreate()
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
			frames.emplace_back(std::make_unique<vkb::rendering::RenderFrameCpp>(device, std::move(render_target), thread_count));
		}

		++frame_it;
	}

	device.get_resource_cache().clear_framebuffers();
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::recreate_swapchain()
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

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::release_owned_semaphore(SemaphoreType semaphore)
{
	get_active_frame().get_semaphore_pool().release_owned_semaphore(semaphore);
}

template <vkb::BindingType bindingType>
inline typename RenderContext<bindingType>::SemaphoreType RenderContext<bindingType>::request_semaphore()
{
	return get_active_frame().get_semaphore_pool().request_semaphore();
}

template <vkb::BindingType bindingType>
inline typename RenderContext<bindingType>::SemaphoreType RenderContext<bindingType>::request_semaphore_with_ownership()
{
	return get_active_frame().get_semaphore_pool().request_semaphore_with_ownership();
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::submit(std::shared_ptr<vkb::core::CommandBuffer<bindingType>> command_buffer)
{
	std::vector<std::shared_ptr<vkb::core::CommandBuffer<bindingType>>> command_buffers(1, command_buffer);
	submit(command_buffers);
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::submit(const std::vector<std::shared_ptr<vkb::core::CommandBuffer<bindingType>>> &command_buffers)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		submit_impl(command_buffers);
	}
	else
	{
		submit_impl(reinterpret_cast<std::vector<std::shared_ptr<vkb::core::CommandBufferCpp>> const &>(command_buffers));
	}
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::submit_impl(std::vector<std::shared_ptr<vkb::core::CommandBufferCpp>> const &command_buffers)
{
	assert(frame_active && "RenderContext is inactive, cannot submit command buffer. Please call begin()");

	vk::Semaphore render_semaphore = nullptr;

	if (swapchain)
	{
		assert(acquired_semaphore && "We do not have acquired_semaphore, it was probably consumed?\n");
		render_semaphore = submit_impl(queue, command_buffers, acquired_semaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput);
	}
	else
	{
		submit_impl(queue, command_buffers);
	}

	end_frame(render_semaphore);
}

template <vkb::BindingType bindingType>
inline typename RenderContext<bindingType>::SemaphoreType
    RenderContext<bindingType>::submit(const QueueType                                                           &queue,
                                       const std::vector<std::shared_ptr<vkb::core::CommandBuffer<bindingType>>> &command_buffers,
                                       SemaphoreType                                                              wait_semaphore,
                                       PipelineStageFlagsType                                                     wait_pipeline_stage)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return submit_impl(queue, command_buffers, wait_semaphore, wait_pipeline_stage);
	}
	else
	{
		return static_cast<VkSemaphore>(submit_impl(reinterpret_cast<vkb::core::HPPQueue const &>(queue),
		                                            reinterpret_cast<std::vector<std::shared_ptr<vkb::core::CommandBufferCpp>> const &>(command_buffers),
		                                            static_cast<vk::Semaphore>(wait_semaphore),
		                                            static_cast<vk::PipelineStageFlags>(wait_pipeline_stage)));
	}
}

template <vkb::BindingType bindingType>
inline vk::Semaphore RenderContext<bindingType>::submit_impl(const vkb::core::HPPQueue                                       &queue,
                                                             const std::vector<std::shared_ptr<vkb::core::CommandBufferCpp>> &command_buffers,
                                                             vk::Semaphore                                                    wait_semaphore,
                                                             vk::PipelineStageFlags                                           wait_pipeline_stage)
{
	std::vector<vk::CommandBuffer> cmd_buf_handles(command_buffers.size(), nullptr);
	std::ranges::transform(command_buffers, cmd_buf_handles.begin(), [](auto const &cmd_buf) { return cmd_buf->get_handle(); });

	vkb::rendering::RenderFrameCpp &frame = *frames[active_frame_index];

	vk::Semaphore signal_semaphore = frame.get_semaphore_pool().request_semaphore();

	vk::SubmitInfo submit_info{.commandBufferCount   = to_u32(cmd_buf_handles.size()),
	                           .pCommandBuffers      = cmd_buf_handles.data(),
	                           .signalSemaphoreCount = 1,
	                           .pSignalSemaphores    = &signal_semaphore};

	if (wait_semaphore != nullptr)
	{
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores    = &wait_semaphore;
		submit_info.pWaitDstStageMask  = &wait_pipeline_stage;
	}

	vk::Fence fence = frame.get_fence_pool().request_fence();

	queue.get_handle().submit(submit_info, fence);

	return signal_semaphore;
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::submit(const QueueType                                                           &queue,
                                               const std::vector<std::shared_ptr<vkb::core::CommandBuffer<bindingType>>> &command_buffers)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		submit_impl(queue, command_buffers);
	}
	else
	{
		submit_impl(reinterpret_cast<vkb::core::HPPQueue const &>(queue),
		            reinterpret_cast<std::vector<std::shared_ptr<vkb::core::CommandBufferCpp>> const &>(command_buffers));
	}
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::submit_impl(vkb::core::HPPQueue const                                       &queue,
                                                    const std::vector<std::shared_ptr<vkb::core::CommandBufferCpp>> &command_buffers)
{
	std::vector<vk::CommandBuffer> cmd_buf_handles(command_buffers.size(), nullptr);
	std::ranges::transform(command_buffers, cmd_buf_handles.begin(), [](auto const &cmd_buf) { return cmd_buf->get_handle(); });

	vk::SubmitInfo submit_info{.commandBufferCount = to_u32(cmd_buf_handles.size()), .pCommandBuffers = cmd_buf_handles.data()};

	vk::Fence fence = frames[active_frame_index]->get_fence_pool().request_fence();

	queue.get_handle().submit(submit_info, fence);
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::update_swapchain(const Extent2DType &extent)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains extent. No swapchain, offscreen rendering detected, skipping.");
		return;
	}

	device.get_resource_cache().clear_framebuffers();

	swapchain = std::make_unique<vkb::core::HPPSwapchain>(*swapchain, extent);

	recreate();
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::update_swapchain(const uint32_t image_count)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains image count. No swapchain, offscreen rendering detected, skipping.");
		return;
	}

	device.get_resource_cache().clear_framebuffers();

	device.get_handle().waitIdle();

	swapchain = std::make_unique<vkb::core::HPPSwapchain>(*swapchain, image_count);

	recreate();
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::update_swapchain(const std::set<ImageUsageFlagBitsType> &image_usage_flags)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains image usage. No swapchain, offscreen rendering detected, skipping.");
		return;
	}

	device.get_resource_cache().clear_framebuffers();

	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		swapchain = std::make_unique<vkb::core::HPPSwapchain>(*swapchain, image_usage_flags);
	}
	else
	{
		swapchain = std::make_unique<vkb::core::HPPSwapchain>(reinterpret_cast<vkb::core::HPPSwapchain &>(*swapchain),
		                                                      reinterpret_cast<std::set<vk::ImageUsageFlagBits> const &>(image_usage_flags));
	}

	recreate();
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::update_swapchain(const Extent2DType &extent, const SurfaceTransformFlagBitsType transform)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		update_swapchain_impl(extent, transform);
	}
	else
	{
		update_swapchain_impl(reinterpret_cast<vk::Extent2D const &>(extent), static_cast<vk::SurfaceTransformFlagBitsKHR>(transform));
	}
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::update_swapchain_impl(vk::Extent2D const &extent, vk::SurfaceTransformFlagBitsKHR transform)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains extent and surface transform. No swapchain, offscreen rendering detected, skipping.");
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

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::update_swapchain(const ImageCompressionFlagsType          compression,
                                                         const ImageCompressionFixedRateFlagsType compression_fixed_rate)
{
	if (!swapchain)
	{
		LOGW("Can't update the swapchains compression. No swapchain, offscreen rendering detected, skipping.");
		return;
	}

	device.get_resource_cache().clear_framebuffers();

	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		swapchain = std::make_unique<vkb::core::HPPSwapchain>(*swapchain, compression, compression_fixed_rate);
	}
	else
	{
		swapchain = std::make_unique<vkb::core::HPPSwapchain>(reinterpret_cast<vkb::core::HPPSwapchain &>(*swapchain),
		                                                      static_cast<vk::ImageCompressionFlagsEXT>(compression),
		                                                      static_cast<vk::ImageCompressionFixedRateFlagsEXT>(compression_fixed_rate));
	}

	recreate();
}

template <vkb::BindingType bindingType>
inline void RenderContext<bindingType>::wait_frame()
{
	get_active_frame().reset();
}

}        // namespace rendering
}        // namespace vkb
