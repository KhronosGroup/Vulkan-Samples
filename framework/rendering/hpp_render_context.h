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

#pragma once

#include "rendering/hpp_render_target.h"
#include <core/hpp_device.h>
#include <core/hpp_swapchain.h>
#include <platform/window.h>
#include <rendering/hpp_render_frame.h>

namespace vkb
{
namespace rendering
{
/**
 * @brief HPPRenderContext is a transcoded version of vkb::RenderContext from vulkan to vulkan-hpp.
 *
 * See vkb::RenderContext for documentation
 */
class HPPRenderContext
{
  public:
	// The format to use for the RenderTargets if a swapchain isn't created
	static vk::Format DEFAULT_VK_FORMAT;

	/**
	 * @brief Constructor
	 * @param device A valid device
	 * @param surface A surface, nullptr if in headless mode
	 * @param window The window where the surface was created
	 * @param present_mode Requests to set the present mode of the swapchain
	 * @param present_mode_priority_list The order in which the swapchain prioritizes selecting its present mode
	 * @param surface_format_priority_list The order in which the swapchain prioritizes selecting its surface format
	 */
	HPPRenderContext(vkb::core::HPPDevice                    &device,
	                 vk::SurfaceKHR                           surface,
	                 const vkb::Window                       &window,
	                 vk::PresentModeKHR                       present_mode                 = vk::PresentModeKHR::eFifo,
	                 std::vector<vk::PresentModeKHR> const   &present_mode_priority_list   = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox},
	                 std::vector<vk::SurfaceFormatKHR> const &surface_format_priority_list = {
	                     {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}, {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}});

	HPPRenderContext(const HPPRenderContext &) = delete;

	HPPRenderContext(HPPRenderContext &&) = delete;

	virtual ~HPPRenderContext() = default;

	HPPRenderContext &operator=(const HPPRenderContext &) = delete;

	HPPRenderContext &operator=(HPPRenderContext &&) = delete;

	/**
	 * @brief Prepares the RenderFrames for rendering
	 * @param thread_count The number of threads in the application, necessary to allocate this many resource pools for each RenderFrame
	 * @param create_render_target_func A function delegate, used to create a RenderTarget
	 */
	void prepare(size_t thread_count = 1, HPPRenderTarget::CreateFunc create_render_target_func = HPPRenderTarget::DEFAULT_CREATE_FUNC);

	/**
	 * @brief Updates the swapchains extent, if a swapchain exists
	 * @param extent The width and height of the new swapchain images
	 */
	void update_swapchain(const vk::Extent2D &extent);

	/**
	 * @brief Updates the swapchains image count, if a swapchain exists
	 * @param image_count The amount of images in the new swapchain
	 */
	void update_swapchain(const uint32_t image_count);

	/**
	 * @brief Updates the swapchains image usage, if a swapchain exists
	 * @param image_usage_flags The usage flags the new swapchain images will have
	 */
	void update_swapchain(const std::set<vk::ImageUsageFlagBits> &image_usage_flags);

	/**
	 * @brief Updates the swapchains extent and surface transform, if a swapchain exists
	 * @param extent The width and height of the new swapchain images
	 * @param transform The surface transform flags
	 */
	void update_swapchain(const vk::Extent2D &extent, const vk::SurfaceTransformFlagBitsKHR transform);

	/**
	 * @returns True if a valid swapchain exists in the HPPRenderContext
	 */
	bool has_swapchain();

	/**
	 * @brief Recreates the RenderFrames, called after every update
	 */
	void recreate();

	/**
	 * @brief Recreates the swapchain
	 */
	void recreate_swapchain();

	/**
	 * @brief Prepares the next available frame for rendering
	 * @param reset_mode How to reset the command buffer
	 * @returns A valid command buffer to record commands to be submitted
	 * Also ensures that there is an active frame if there is no existing active frame already
	 */
	vkb::core::CommandBuffer<vkb::BindingType::Cpp> &begin(vkb::core::CommandBuffer<vkb::BindingType::Cpp>::ResetMode reset_mode =
	                                                           vkb::core::CommandBuffer<vkb::BindingType::Cpp>::ResetMode::ResetPool);

	/**
	 * @brief Submits the command buffer to the right queue
	 * @param command_buffer A command buffer containing recorded commands
	 */
	void submit(vkb::core::CommandBuffer<vkb::BindingType::Cpp> &command_buffer);

	/**
	 * @brief Submits multiple command buffers to the right queue
	 * @param command_buffers Command buffers containing recorded commands
	 */
	void submit(const std::vector<vkb::core::CommandBuffer<vkb::BindingType::Cpp> *> &command_buffers);

	/**
	 * @brief begin_frame
	 */
	void begin_frame();

	vk::Semaphore submit(const vkb::core::HPPQueue                                            &queue,
	                     const std::vector<vkb::core::CommandBuffer<vkb::BindingType::Cpp> *> &command_buffers,
	                     vk::Semaphore                                                         wait_semaphore,
	                     vk::PipelineStageFlags                                                wait_pipeline_stage);

	/**
	 * @brief Submits a command buffer related to a frame to a queue
	 */
	void submit(const vkb::core::HPPQueue &queue, const std::vector<vkb::core::CommandBuffer<vkb::BindingType::Cpp> *> &command_buffers);

	/**
	 * @brief Waits a frame to finish its rendering
	 */
	virtual void wait_frame();

	void end_frame(vk::Semaphore semaphore);

	/**
	 * @brief An error should be raised if the frame is not active.
	 *        A frame is active after @ref begin_frame has been called.
	 * @return The current active frame
	 */
	HPPRenderFrame &get_active_frame();

	/**
	 * @brief An error should be raised if the frame is not active.
	 *        A frame is active after @ref begin_frame has been called.
	 * @return The current active frame index
	 */
	uint32_t get_active_frame_index();

	/**
	 * @brief An error should be raised if a frame is active.
	 *        A frame is active after @ref begin_frame has been called.
	 * @return The previous frame
	 */
	HPPRenderFrame &get_last_rendered_frame();

	vk::Semaphore request_semaphore();
	vk::Semaphore request_semaphore_with_ownership();
	void          release_owned_semaphore(vk::Semaphore semaphore);

	vkb::core::HPPDevice &get_device();

	/**
	 * @brief Returns the format that the RenderTargets are created with within the HPPRenderContext
	 */
	vk::Format get_format() const;

	vkb::core::HPPSwapchain const &get_swapchain() const;

	vk::Extent2D const &get_surface_extent() const;

	uint32_t get_active_frame_index() const;

	std::vector<std::unique_ptr<HPPRenderFrame>> &get_render_frames();

	/**
	 * @brief Handles surface changes, only applicable if the render_context makes use of a swapchain
	 */
	virtual bool handle_surface_changes(bool force_update = false);

	/**
	 * @brief Returns the WSI acquire semaphore. Only to be used in very special circumstances.
	 * @return The WSI acquire semaphore.
	 */
	vk::Semaphore consume_acquired_semaphore();

  protected:
	vk::Extent2D surface_extent;

  private:
	vkb::core::HPPDevice &device;

	const vkb::Window &window;

	/// If swapchain exists, then this will be a present supported queue, else a graphics queue
	const vkb::core::HPPQueue &queue;

	std::unique_ptr<vkb::core::HPPSwapchain> swapchain;

	vkb::core::HPPSwapchainProperties swapchain_properties;

	std::vector<std::unique_ptr<HPPRenderFrame>> frames;

	vk::Semaphore acquired_semaphore;

	bool prepared{false};

	/// Current active frame index
	uint32_t active_frame_index{0};

	/// Whether a frame is active or not
	bool frame_active{false};

	HPPRenderTarget::CreateFunc create_render_target_func = HPPRenderTarget::DEFAULT_CREATE_FUNC;

	vk::SurfaceTransformFlagBitsKHR pre_transform{vk::SurfaceTransformFlagBitsKHR::eIdentity};

	size_t thread_count{1};
};

}        // namespace rendering
}        // namespace vkb
