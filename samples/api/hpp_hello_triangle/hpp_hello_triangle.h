/* Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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

#include <vulkan/vulkan.hpp>

#include <platform/application.h>

/**
 * @brief A self-contained (minimal use of framework) sample that illustrates
 * the rendering of a triangle
 */
class HPPHelloTriangle : public vkb::Application
{
	/**
	 * @brief Swapchain data
	 */
	struct SwapchainData
	{
		vk::Extent2D                 extent;                                 // The swapchain extent
		vk::Format                   format = vk::Format::eUndefined;        // Pixel format of the swapchain.
		vk::SwapchainKHR             swapchain;                              // The swapchain.
		std::vector<vk::ImageView>   image_views;                            // The image view for each swapchain image.
		std::vector<vk::Framebuffer> framebuffers;                           // The framebuffer for each swapchain image view.
	};

	/**
	 * @brief Per-frame data
	 */
	struct FrameData
	{
		vk::Fence         queue_submit_fence;
		vk::CommandPool   primary_command_pool;
		vk::CommandBuffer primary_command_buffer;
		vk::Semaphore     swapchain_acquire_semaphore;
		vk::Semaphore     swapchain_release_semaphore;
	};

  public:
	HPPHelloTriangle();
	virtual ~HPPHelloTriangle();

  private:
	// from vkb::Application
	virtual bool prepare(const vkb::ApplicationOptions &options) override;
	virtual bool resize(const uint32_t width, const uint32_t height) override;
	virtual void update(float delta_time) override;

	std::pair<vk::Result, uint32_t> acquire_next_image();
	vk::Device                      create_device(const std::vector<const char *> &required_device_extensions);
	vk::Pipeline                    create_graphics_pipeline();
	vk::ImageView                   create_image_view(vk::Image image);
	vk::Instance                    create_instance(std::vector<const char *> const &required_instance_extensions, std::vector<const char *> const &required_validation_layers);
	vk::RenderPass                  create_render_pass();
	vk::ShaderModule                create_shader_module(const char *path);
	vk::SwapchainKHR                create_swapchain(vk::Extent2D const &swapchain_extent, vk::SurfaceFormatKHR surface_format, vk::SwapchainKHR old_swapchain);
	void                            init_framebuffers();
	void                            init_swapchain();
	void                            render_triangle(uint32_t swapchain_index);
	void                            select_physical_device_and_surface();
	void                            teardown_framebuffers();
	void                            teardown_per_frame(FrameData &per_frame_data);

  private:
	vk::Instance               instance;                     // The Vulkan instance.
	vk::PhysicalDevice         gpu;                          // The Vulkan physical device.
	vk::Device                 device;                       // The Vulkan device.
	vk::Queue                  queue;                        // The Vulkan device queue.
	SwapchainData              swapchain_data;               // The swapchain state.
	vk::SurfaceKHR             surface;                      // The surface we will render to.
	uint32_t                   graphics_queue_index;         // The queue family index where graphics work will be submitted.
	vk::RenderPass             render_pass;                  // The renderpass description.
	vk::PipelineLayout         pipeline_layout;              // The pipeline layout for resources.
	vk::Pipeline               pipeline;                     // The graphics pipeline.
	vk::DebugUtilsMessengerEXT debug_utils_messenger;        // The debug utils messenger.
	std::vector<vk::Semaphore> recycled_semaphores;          // A set of semaphores that can be reused.
	std::vector<FrameData>     per_frame_data;               // A set of per-frame data.

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info;
#endif
};

std::unique_ptr<vkb::Application> create_hpp_hello_triangle();
