/* Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
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

#include <platform/hpp_application.h>

#include <platform/hpp_platform.h>

/**
 * @brief A self-contained (minimal use of framework) sample that illustrates
 * the rendering of a triangle
 */
class HPPHelloTriangle : public vkb::platform::HPPApplication
{
	/**
	 * @brief Swapchain state
	 */
	struct SwapchainDimensions
	{
		/// Width of the swapchain.
		uint32_t width = 0;

		/// Height of the swapchain.
		uint32_t height = 0;

		/// Pixel format of the swapchain.
		vk::Format format = vk::Format::eUndefined;
	};

	/**
	 * @brief Per-frame data
	 */
	struct PerFrame
	{
		vk::Device device;

		vk::Fence queue_submit_fence;

		vk::CommandPool primary_command_pool;

		vk::CommandBuffer primary_command_buffer;

		vk::Semaphore swapchain_acquire_semaphore;

		vk::Semaphore swapchain_release_semaphore;

		int32_t queue_index;
	};

	/**
	 * @brief Vulkan objects and global state
	 */
	struct Context
	{
		/// The Vulkan instance.
		vk::Instance instance;

		/// The Vulkan physical device.
		vk::PhysicalDevice gpu;

		/// The Vulkan device.
		vk::Device device;

		/// The Vulkan device queue.
		vk::Queue queue;

		/// The swapchain.
		vk::SwapchainKHR swapchain;

		/// The swapchain dimensions.
		SwapchainDimensions swapchain_dimensions;

		/// The surface we will render to.
		vk::SurfaceKHR surface;

		/// The queue family index where graphics work will be submitted.
		int32_t graphics_queue_index = -1;

		/// The image view for each swapchain image.
		std::vector<vk::ImageView> swapchain_image_views;

		/// The framebuffer for each swapchain image view.
		std::vector<vk::Framebuffer> swapchain_framebuffers;

		/// The renderpass description.
		vk::RenderPass render_pass;

		/// The graphics pipeline.
		vk::Pipeline pipeline;

		/**
		 * The pipeline layout for resources.
		 * Not used in this sample, but we still need to provide a dummy one.
		 */
		vk::PipelineLayout pipeline_layout;

		/// The debug report callback.
		vk::DebugReportCallbackEXT debug_callback;

		/// A set of semaphores that can be reused.
		std::vector<vk::Semaphore> recycled_semaphores;

		/// A set of per-frame data.
		std::vector<PerFrame> per_frame;
	};

  public:
	HPPHelloTriangle();

	virtual ~HPPHelloTriangle();

	virtual bool prepare(vkb::platform::HPPPlatform &platform) override;

	virtual void update(float delta_time) override;

	virtual bool resize(const uint32_t width, const uint32_t height) override;

	bool validate_extensions(const std::vector<const char *> &           required,
	                         const std::vector<vk::ExtensionProperties> &available);

	VkShaderStageFlagBits find_shader_stage(const std::string &ext);

	void init_instance(Context &                        context,
	                   const std::vector<const char *> &required_instance_extensions,
	                   const std::vector<const char *> &required_validation_layers);

	void select_physical_device_and_surface(vkb::platform::HPPPlatform &platform);

	void init_device(Context &                        context,
	                 const std::vector<const char *> &required_device_extensions);

	void init_per_frame(Context &context, PerFrame &per_frame);

	void teardown_per_frame(Context &context, PerFrame &per_frame);

	void init_swapchain(Context &context);

	void init_render_pass(Context &context);

	vk::ShaderModule load_shader_module(Context &context, const char *path);

	void init_pipeline(Context &context);

	vk::Result acquire_next_image(Context &context, uint32_t *image);

	void render_triangle(Context &context, uint32_t swapchain_index);

	vk::Result present_image(Context &context, uint32_t index);

	void init_framebuffers(Context &context);

	void teardown_framebuffers(Context &context);

	void teardown(Context &context);

  private:
	Context context;
};

std::unique_ptr<vkb::Application> create_hpp_hello_triangle();
