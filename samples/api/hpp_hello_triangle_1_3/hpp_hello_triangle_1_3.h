/* Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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

#include "platform/application.h"
#include "platform/window.h"

#include <vulkan/vulkan.hpp>

/**
 * @brief A self-contained (minimal use of framework) sample that illustrates
 * the rendering of a triangle
 */
class HPPHelloTriangleV13 : public vkb::Application
{
	// Define the Vertex structure
	struct Vertex
	{
		glm::vec2 position;
		glm::vec3 color;
	};

	// Define the vertex data
	const std::vector<Vertex> vertices = {
	    {{0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},        // Vertex 1: Red
	    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},         // Vertex 2: Green
	    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}         // Vertex 3: Blue
	};

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
		vk::Fence         queue_submit_fence          = nullptr;
		vk::CommandPool   primary_command_pool        = nullptr;
		vk::CommandBuffer primary_command_buffer      = nullptr;
		vk::Semaphore     swapchain_acquire_semaphore = nullptr;
		vk::Semaphore     swapchain_release_semaphore = nullptr;
	};

	/**
	 * @brief Vulkan objects and global state
	 */
	struct Context
	{
		/// The Vulkan instance.
		vk::Instance instance = nullptr;

		/// The Vulkan physical device.
		vk::PhysicalDevice gpu = nullptr;

		/// The Vulkan device.
		vk::Device device = nullptr;

		/// The Vulkan device queue.
		vk::Queue queue = nullptr;

		/// The swapchain.
		vk::SwapchainKHR swapchain = nullptr;

		/// The swapchain dimensions.
		SwapchainDimensions swapchain_dimensions;

		/// The surface we will render to.
		vk::SurfaceKHR surface = nullptr;

		/// The queue family index where graphics work will be submitted.
		int32_t graphics_queue_index = -1;

		/// The image view for each swapchain image.
		std::vector<vk::ImageView> swapchain_image_views;

		/// The handles to the images in the swapchain.
		std::vector<vk::Image> swapchain_images;

		/// The graphics pipeline.
		vk::Pipeline pipeline = nullptr;

		/**
		 * The pipeline layout for resources.
		 * Not used in this sample, but we still need to provide a dummy one.
		 */
		vk::PipelineLayout pipeline_layout = nullptr;

		/// The debug utility messenger callback.
		vk::DebugUtilsMessengerEXT debug_callback = nullptr;

		/// A set of semaphores that can be reused.
		std::vector<vk::Semaphore> recycled_semaphores;

		/// A set of per-frame data.
		std::vector<PerFrame> per_frame;

		/// The Vulkan buffer object that holds the vertex data for the triangle.
		vk::Buffer vertex_buffer = nullptr;

		/// The device memory allocated for the vertex buffer.
		vk::DeviceMemory vertex_buffer_memory = nullptr;
	};

  public:
	HPPHelloTriangleV13() = default;

	virtual ~HPPHelloTriangleV13();

  private:
	// from vkb::Application
	virtual bool prepare(const vkb::ApplicationOptions &options) override;
	virtual bool resize(const uint32_t width, const uint32_t height) override;
	virtual void update(float delta_time) override;

	vk::Result       acquire_next_swapchain_image(uint32_t *image);
	uint32_t         find_memory_type(vk::PhysicalDevice physical_device, uint32_t type_filter, vk::MemoryPropertyFlags properties);
	void             init_device();
	void             init_instance();
	void             init_per_frame(PerFrame &per_frame);
	void             init_pipeline();
	void             init_swapchain();
	void             init_vertex_buffer();
	vk::ShaderModule load_shader_module(const std::string &path, vk::ShaderStageFlagBits shader_stage);
	vk::Result       present_image(uint32_t index);
	void             render_triangle(uint32_t swapchain_index);
	void             select_physical_device_and_surface(vkb::Window *window);
	void             teardown_per_frame(PerFrame &per_frame);
	void             transition_image_layout(vk::CommandBuffer       cmd,
	                                         vk::Image               image,
	                                         vk::ImageLayout         oldLayout,
	                                         vk::ImageLayout         newLayout,
	                                         vk::AccessFlags2        srcAccessMask,
	                                         vk::AccessFlags2        dstAccessMask,
	                                         vk::PipelineStageFlags2 srcStage,
	                                         vk::PipelineStageFlags2 dstStage);
	bool             validate_extensions(const std::vector<const char *> &required, const std::vector<vk::ExtensionProperties> &available);

  private:
	Context context;
};

std::unique_ptr<vkb::Application> create_hpp_hello_triangle_1_3();
