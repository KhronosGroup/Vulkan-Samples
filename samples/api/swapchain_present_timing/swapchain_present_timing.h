/* Copyright (c) 2026, NVIDIA CORPORATION
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
 * @brief A sample that implements frame pacing using time-based presentation.
 */
class SwapchainPresentTiming : public vkb::VulkanSampleC
{
	/**
	 * @brief Size of the frame data history. Only used as an example here.
	 */
	static constexpr size_t history_buffer_size = 64;

	/**
	 * @brief Maximum number of present stages we ever request.
	 */
	static constexpr size_t max_present_stages = 4;

	/**
	 * @brief A swapchain time domain is a combination of a VkTimeDomainKHR enum and an id which
	 * is used to differentiate between multiple swapchain-local opaque time domains.
	 */
	struct SwapchainTimeDomain
	{
		VkTimeDomainKHR time_domain    = VK_TIME_DOMAIN_MAX_ENUM_KHR;
		uint64_t        time_domain_id = 0;
	};

	/**
	 * @brief Timing results for a given present
	 */
	struct FrameTimingData
	{
		uint64_t present_id            = 0;
		uint64_t target_time           = 0;
		uint64_t present_ready_time    = 0;
		uint64_t present_dequeued_time = 0;
		uint64_t present_display_time  = 0;
	};

	/**
	 * @brief Per-frame render data.
	 */
	struct PerFrame
	{
		VkFence         render_fence      = VK_NULL_HANDLE;
		VkSemaphore     acquire_semaphore = VK_NULL_HANDLE;
		VkCommandPool   command_pool      = VK_NULL_HANDLE;
		VkCommandBuffer command_buffer    = VK_NULL_HANDLE;
	};

	/**
	 * @brief Per swapchain image resources.
	 */
	struct PerSwapchainImage
	{
		VkImage     image            = VK_NULL_HANDLE;
		VkImageView image_view       = VK_NULL_HANDLE;
		VkSemaphore render_semaphore = VK_NULL_HANDLE;
	};

	/**
	 * @brief Simple push constants to draw an animated shape in a fragment shader.
	 */
	struct PushConstants
	{
		glm::vec2 resolution;
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 color;
	};

  public:
	SwapchainPresentTiming();
	~SwapchainPresentTiming() override;

	void create_render_context() override;
	void prepare_render_context() override;
	void update(float delta_time) override;
	bool resize(uint32_t width, uint32_t height) override;
	void input_event(const vkb::InputEvent &input_event) override;

  private:
	/// Submission and present queue.
	const vkb::Queue *queue           = nullptr;
	VkPipelineLayout  pipeline_layout = VK_NULL_HANDLE;
	VkPipeline        pipeline        = VK_NULL_HANDLE;

	/// Surface data.
	VkSurfaceFormatKHR          surface_format               = {};
	VkExtent2D                  swapchain_extents            = {};
	VkCompositeAlphaFlagBitsKHR composite                    = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	uint32_t                    desired_swapchain_images     = 0;
	bool                        can_use_present_timing       = false;
	bool                        can_present_at_absolute_time = false;
	VkPresentStageFlagsEXT      display_present_stage        = 0;

	/// The swapchain.
	VkSwapchainKHR                 swapchain    = VK_NULL_HANDLE;
	VkPresentModeKHR               present_mode = VK_PRESENT_MODE_FIFO_KHR;
	std::vector<PerSwapchainImage> swapchain_images;

	/// Present timing utilities
	VkSwapchainTimingPropertiesEXT timing_properties         = {};
	uint64_t                       timing_properties_counter = 0;
	SwapchainTimeDomain            time_domain               = {};
	uint64_t                       time_domains_counter      = 0;

	/// Present infos
	uint64_t present_id              = 1;
	uint64_t target_present_duration = 0;

	/// Per frame resources
	std::array<PerFrame, 2> frame_resources;
	size_t                  frame_index = 0;

	/// Buffer used to query past presentation timing.
	std::array<VkPastPresentationTimingEXT, history_buffer_size> past_presentation_timings;

	/// Frame timing history. This is not directly used and is meant as an example.
	std::array<FrameTimingData, history_buffer_size> timing_history;
	size_t                                           timing_history_size = 0;

	/// FPS log.
	float    fps_timer                    = 0;
	uint32_t fps_last_logged_frame_number = 0;

	/// Time tracking
	float    cpu_time                = 0.0f;
	uint64_t display_time_ns_base    = 0;
	uint64_t display_time_ns         = 0;
	uint64_t display_time_present_id = 0;

	uint32_t get_api_version() const override;
	void     request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;
	void     request_instance_extensions(std::unordered_map<std::string, vkb::RequestMode> &requested_extensions) const override;

	void select_surface_format();
	void query_surface_capabilities();
	void query_swapchain_timing_properties();
	void select_swapchain_time_domain();
	void select_target_present_duration();
	void invalidate_timing_history();
	void query_past_presentation_timing();
	void set_present_timing_info(VkPresentTimingInfoEXT &timing_info, uint64_t present_time);

	void create_pipeline();
	void create_frame_resources(PerFrame &frame);
	void destroy_frame_resources(PerFrame &frame);
	void init_swapchain();
	void init_swapchain_image(PerSwapchainImage &perImage, VkImage image);
	bool recreate_swapchain();

	void     setup_frame();
	uint64_t render(uint32_t index, float delta_time);
	VkResult acquire_next_image(uint32_t *index);
	VkResult present_image(uint32_t index, uint64_t present_time);

	void             get_queue();
	VkPhysicalDevice get_gpu_handle();
	VkDevice         get_device_handle();
};

std::unique_ptr<vkb::Application> create_swapchain_present_timing();
