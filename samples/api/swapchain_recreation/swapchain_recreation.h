/* Copyright (c) 2023-2024, Google
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
#include "platform/application.h"
#include "vulkan_sample.h"

/**
 * @brief A sample that implements best practices in handling present resources and swapchain
 * recreation, for example due to window resizing or present mode changes.
 */
class SwapchainRecreation : public vkb::VulkanSample<vkb::BindingType::C>
{
	struct SwapchainObjects
	{
		std::vector<VkImage>       images;
		std::vector<VkImageView>   views;
		std::vector<VkFramebuffer> framebuffers;
	};

	/**
	 * @brief Per-frame data.  This is not per swapchain image!
	 * A queue of this data structure is used to remember the history of submissions.  To avoid
	 * the CPU getting too far ahead of the GPU, the sample paces itself by waiting for the
	 * submission before last to finish before recording commands for the new frame.  This means
	 * that frame N+1 doesn't start recording until frame N-1 finishes executing on the GPU (and
	 * likely frame N starts).  In a real application, this minimizes latency from input to
	 * screen.
	 */
	struct PerFrame
	{
		VkFence         submit_fence      = VK_NULL_HANDLE;
		VkCommandPool   command_pool      = VK_NULL_HANDLE;
		VkCommandBuffer command_buffer    = VK_NULL_HANDLE;
		VkSemaphore     acquire_semaphore = VK_NULL_HANDLE;
		VkSemaphore     present_semaphore = VK_NULL_HANDLE;

		// Garbage to clean up once the submit_fence is signaled, if any.
		std::vector<SwapchainObjects> swapchain_garbage;
	};

	struct SwapchainCleanupData
	{
		/// The old swapchain to be destroyed.
		VkSwapchainKHR swapchain = VK_NULL_HANDLE;

		/**
		 * @brief Any present semaphores that were pending recycle at the time the swapchain
		 * was recreated will be scheduled for recycling at the same time as the swapchain's
		 * destruction.
		 */
		std::vector<VkSemaphore> semaphores;
	};

	struct PresentOperationInfo
	{
		/**
		 * @brief Fence that tells when the present semaphore can be destroyed.  Without
		 * VK_EXT_swapchain_maintenance1, the fence used with the vkAcquireNextImageKHR that
		 * returns the same image index in the future is used to know when the semaphore can
		 * be recycled.
		 */
		VkFence     cleanup_fence     = VK_NULL_HANDLE;
		VkSemaphore present_semaphore = VK_NULL_HANDLE;

		/**
		 * @brief Old swapchains are scheduled to be destroyed at the same time as the last
		 * wait semaphore used to present an image to the old swapchains can be recycled.
		 */
		std::vector<SwapchainCleanupData> old_swapchains;

		/**
		 * @brief Used to associate an acquire fence with the previous present operation of
		 * the image.  Only relevant when VK_EXT_swapchain_maintenance1 is not supported;
		 * otherwise a fence is always associated with the present operation.
		 */
		uint32_t image_index = std::numeric_limits<uint32_t>::max();
	};

  public:
	SwapchainRecreation();

	~SwapchainRecreation() override;

	void create_render_context() override;

	void prepare_render_context() override;

	bool prepare(const vkb::ApplicationOptions &options) override;

	void update(float delta_time) override;

	bool resize(uint32_t width, uint32_t height) override;

	void input_event(const vkb::InputEvent &input_event) override;

  private:
	/// Submission and present queue.
	const vkb::Queue *queue = nullptr;

	/// Surface data.
	VkSurfaceFormatKHR            surface_format    = {};
	std::vector<VkPresentModeKHR> present_modes     = {};
	VkExtent2D                    swapchain_extents = {};

	/// The swapchain.
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;

	/// Swapchain data.
	VkPresentModeKHR current_present_mode = VK_PRESENT_MODE_FIFO_KHR;
	VkPresentModeKHR desired_present_mode = VK_PRESENT_MODE_FIFO_KHR;
	SwapchainObjects swapchain_objects;

	/// The render pass used for rendering.
	VkRenderPass render_pass = VK_NULL_HANDLE;

	/// The submission history.  This is a fixed-size queue, implemented as a circular buffer.
	std::array<PerFrame, 2> submit_history       = {};
	size_t                  submit_history_index = 0;

	/// The present operation history.  This is used to clean up present semaphores and old swapchains.
	std::deque<PresentOperationInfo> present_history;

	/**
	 * @brief The previous swapchain which needs to be scheduled for destruction when
	 * appropriate.  This will be done when the first image of the current swapchain is
	 * presented.  If there were older swapchains pending destruction when the swapchain is
	 * recreated, they will accumulate and be destroyed with the previous swapchain.
	 *
	 * Note that if the user resizes the window such that the swapchain is recreated every
	 * frame, this array can go grow indefinitely.
	 */
	std::vector<SwapchainCleanupData> old_swapchains;

	/// Resource pools.
	std::vector<VkSemaphore> semaphore_pool;
	std::vector<VkFence>     fence_pool;

	/// Time.
	uint32_t frame_number = 0;

	// FPS log.
	float    fps_timer                    = 0;
	uint32_t fps_last_logged_frame_number = 0;

	void get_queue();
	void query_surface_format();
	void query_present_modes();
	void adjust_desired_present_mode();
	void create_render_pass();

	void init_swapchain();
	void init_swapchain_image(uint32_t index);
	void cleanup_swapchain_objects(SwapchainObjects &garbage);
	bool recreate_swapchain();

	void setup_frame();
	void render(uint32_t index);

	VkResult acquire_next_image(uint32_t *index);
	VkResult present_image(uint32_t index);
	void     add_present_to_history(uint32_t index);
	void     cleanup_present_history();
	void     cleanup_present_info(PresentOperationInfo &present_info);
	void     cleanup_old_swapchain(SwapchainCleanupData &old_swapchain);

	void associate_fence_with_present_history(uint32_t index, VkFence acquire_fence);
	void schedule_old_swapchain_for_destruction(VkSwapchainKHR old_swapchain);

	VkSemaphore get_semaphore();
	void        recycle_semaphore(VkSemaphore semaphore);

	VkFence get_fence();
	void    recycle_fence(VkFence fence);

	VkPhysicalDevice get_gpu_handle();
	VkDevice         get_device_handle();
};

std::unique_ptr<vkb::Application> create_swapchain_recreation();
