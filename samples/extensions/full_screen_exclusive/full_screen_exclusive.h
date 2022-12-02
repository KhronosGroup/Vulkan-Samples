/* Copyright (c) 2022, Holochip Corporation
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

/*
 * Demonstrates and showcases full_screen_exclusive related functionalities
 */

#pragma once

#include "common/vk_common.h"
#include "core/instance.h"
#include "platform/application.h"

class FullScreenExclusive : public vkb::Application
{
  private:
	struct SwapchainDimensions
	{
		uint32_t width  = 0;
		uint32_t height = 0;
		VkFormat format = VK_FORMAT_UNDEFINED;
	};

	struct PerFrame
	{
		VkDevice        device                      = VK_NULL_HANDLE;
		VkFence         queue_submit_fence          = VK_NULL_HANDLE;
		VkCommandPool   primary_command_pool        = VK_NULL_HANDLE;
		VkCommandBuffer primary_command_buffer      = VK_NULL_HANDLE;
		VkSemaphore     swapchain_acquire_semaphore = VK_NULL_HANDLE;
		VkSemaphore     swapchain_release_semaphore = VK_NULL_HANDLE;
		int32_t         queue_index{};
	};

	struct Context
	{
		VkInstance                 instance  = VK_NULL_HANDLE;
		VkPhysicalDevice           gpu       = VK_NULL_HANDLE;
		VkDevice                   device    = VK_NULL_HANDLE;
		VkQueue                    queue     = VK_NULL_HANDLE;
		VkSwapchainKHR             swapchain = VK_NULL_HANDLE;
		SwapchainDimensions        swapchain_dimensions{};
		VkSurfaceKHR               surface              = VK_NULL_HANDLE;
		int32_t                    graphics_queue_index = -1;
		std::vector<VkImageView>   swapchain_image_views{};
		std::vector<VkFramebuffer> swapchain_frame_buffers{};
		VkRenderPass               render_pass     = VK_NULL_HANDLE;
		VkPipeline                 pipeline        = VK_NULL_HANDLE;
		VkPipelineLayout           pipeline_layout = VK_NULL_HANDLE;
		VkDebugReportCallbackEXT   debug_callback  = VK_NULL_HANDLE;
		std::vector<VkSemaphore>   recycled_semaphores{};
		std::vector<PerFrame>      per_frame{};
	};

  private:
	Context                        context{};
	std::unique_ptr<vkb::Instance> vk_instance{};

	// Full Screen Exclusive related variables
	bool isFullScreenExclusive = false;        // this is to tell if the screen is in full screen exclusive or not
	int  full_screen_status    = 0;            // 0 means default, 1 means disallowed, 2 means allowed, 3 means full screen exclusive

  public:
	FullScreenExclusive();
	virtual ~FullScreenExclusive() override;
	virtual bool prepare(vkb::Platform &platform) override;
	virtual void update(float delta_time) override;                       // add gui
	virtual bool resize(uint32_t width, uint32_t height) override;        // add gui

	virtual void input_event(const vkb::InputEvent &input_event) override;        // this is to introduce a customized input events

	void recreate();

	bool                  validate_extensions(const std::vector<const char *> &required, const std::vector<VkExtensionProperties> &available);
	bool                  validate_layers(const std::vector<const char *> &required, const std::vector<VkLayerProperties> &available);
	VkShaderStageFlagBits find_shader_stage(const std::string &ext);
	void                  init_instance(Context &context, const std::vector<const char *> &required_instance_extensions, const std::vector<const char *> &required_validation_layers);
	void                  init_device(Context &context, const std::vector<const char *> &required_device_extensions);
	void                  init_per_frame(Context &context, PerFrame &per_frame);
	void                  teardown_per_frame(Context &context, PerFrame &per_frame);
	void                  init_swapchain(Context &context);
	void                  init_render_pass(Context &context);
	VkShaderModule        load_shader_module(Context &context, const char *path);
	void                  init_pipeline(Context &context);
	VkResult              acquire_next_image(Context &context, uint32_t *image);
	void                  render_triangle(Context &context, uint32_t swapchain_index);
	VkResult              present_image(Context &context, uint32_t index);
	void                  init_frame_buffers(Context &context);
	void                  teardown_frame_buffers(Context &context);
	void                  teardown(Context &context);
};

std::unique_ptr<vkb::Application> create_full_screen_exclusive();
