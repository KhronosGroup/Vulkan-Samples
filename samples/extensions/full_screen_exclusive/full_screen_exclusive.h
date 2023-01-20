/* Copyright (c) 2023, Holochip Corporation
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
 * Demonstrates and showcases full_screen_exclusive and related functionalities
 */

#pragma once

#include "common/vk_common.h"
#include "core/instance.h"
#include "platform/application.h"

class FullScreenExclusive : public vkb::Application
{
  private:
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	enum class SwapchainMode        // This enum class represents all for stages from full screen exclusive EXT selections
	{
		Default,
		Windowed,
		BorderlessFullscreen,
		ExclusiveFullscreen
	};

	enum class ApplicationWindowMode        // This enum class represents the application window modes
	{
		Windowed,
		Fullscreen
	};
#endif

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

#if defined(VK_USE_PLATFORM_WIN32_KHR)                                                                           // To slightly optimize the following variables, if a Windows platform is detected, then they shall be defined, otherwise not.
	HWND                                     HWND_applicationWindow{};                                           // sync the application HWND handle
	bool                                     isWindowed = true;                                                  // this is to tell if the application window is already set in the desired mode
	WINDOWPLACEMENT                          wpc{};                                                              // window placement information
	LONG                                     HWND_style          = 0;                                            // current Hwnd style
	LONG                                     HWND_style_previous = 0;                                            // previous Hwnd style
	VkSurfaceFullScreenExclusiveInfoEXT      surface_full_screen_exclusive_info_EXT{};                           // it can be created locally, however, it is a good reminder that they are declared here as a class variable
	VkSurfaceFullScreenExclusiveWin32InfoEXT surface_full_screen_exclusive_Win32_info_EXT{};                     // if using DirectX, then this variable has to be created and attach to the pNext of a VkSurfaceFullScreenExclusiveInfoEXT value
	bool                                     isFullScreenExclusive     = false;                                  // this is to tell if the screen is in full screen EXCLUSIVE or not
	ApplicationWindowMode                    application_window_status = ApplicationWindowMode::Windowed;        // declare and initialize the application window mode
	SwapchainMode                            full_screen_status        = SwapchainMode::Default;                 // declare and initialize the swapchain mode
#endif

  private:
	VkExtent2D update_current_maxImageExtent() const;                           // This detects the maximum surface resolution and return them in vkExtent2D format
	void       input_event(const vkb::InputEvent &input_event) override;        // This is to introduce a customized input events for switching application window and swapchain modes.
	void       update_application_window();                                     // This switches application window modes corresponding to the selected swapchain modes
	void       recreate();                                                      // to recreate the swapchain and related per switch of display mode

  public:
	FullScreenExclusive() = default;
	~FullScreenExclusive() override;
	void                         initialize_windows();                             // This only calls when a Windows platform is detected, and it initializes the full screen exclusive related variables
	bool                         prepare(vkb::Platform &platform) override;        // This syncs all required extensions and booleans is a Windows platform is detected
	void                         update(float delta_time) override;
	bool                         resize(uint32_t width, uint32_t height) override;
	static bool                  validate_extensions(const std::vector<const char *> &required, const std::vector<VkExtensionProperties> &available);
	static bool                  validate_layers(const std::vector<const char *> &required, const std::vector<VkLayerProperties> &available);
	static VkShaderStageFlagBits find_shader_stage(const std::string &ext);
	void                         init_instance(Context &input_context, const std::vector<const char *> &required_instance_extensions, const std::vector<const char *> &required_validation_layers);
	void                         init_device(Context &input_context, const std::vector<const char *> &required_device_extensions);
	static void                  init_per_frame(Context &input_context, PerFrame &per_frame);
	static void                  teardown_per_frame(Context &input_context, PerFrame &per_frame);
	void                         init_swapchain(Context &input_context);
	static void                  init_render_pass(Context &input_context);
	VkShaderModule               load_shader_module(Context &input_context, const char *path) const;
	void                         init_pipeline(Context &input_context);
	static VkResult              acquire_next_image(Context &input_context, uint32_t *image);
	static void                  render_triangle(Context &input_context, uint32_t swapchain_index);
	static VkResult              present_image(Context &input_context, uint32_t index);
	static void                  init_frame_buffers(Context &input_context);
	static void                  teardown_frame_buffers(Context &input_context);
	void                         teardown(Context &input_context);
};

std::unique_ptr<vkb::Application> create_full_screen_exclusive();
