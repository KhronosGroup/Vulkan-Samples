/* Copyright (c) 2023, Arm Limited and Contributors
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

#include "api_vulkan_sample.h"
#include "rendering/render_pipeline.h"
#include "scene_graph/components/camera.h"

#define CL_FUNCTION_DEFINITIONS
#include <open_cl_utils.h>

/// @brief Helper macro to test the result of OpenCL calls which can return an error.
#define CL_CHECK(x)                                                                    \
	do                                                                                 \
	{                                                                                  \
		cl_int res = x;                                                                \
		if (res)                                                                       \
		{                                                                              \
			LOGE("Detected OpenCL error: {}", std::to_string(res));                    \
			throw std::runtime_error("Detected OpenCL error: " + std::to_string(res)); \
		}                                                                              \
	} while (0)

class OpenCLInterop : public ApiVulkanSample
{
  public:
	OpenCLInterop();
	~OpenCLInterop() override;

	bool prepare(vkb::Platform &platform) override;

	void render(float delta_time) override;
	void view_changed() override;
	void build_command_buffers() override;

  private:
	void prepare_pipelines();
	void prepare_open_cl_resources();
	void prepare_shared_resources();
	void generate_quad();
	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	void setup_descriptor_set();
	void prepare_uniform_buffers();
	void update_uniform_buffers();

	// @todo: rename?
	void prepare_sync_objects();

#ifdef _WIN32
	HANDLE get_vulkan_memory_handle(VkDeviceMemory memory);
	HANDLE get_vulkan_semaphore_handle(VkSemaphore &sempahore);
#else
	int get_vulkan_memory_handle(VkDeviceMemory memory);
	int get_vulkan_semaphore_handle(VkSemaphore &sempahore);
#endif

	struct VertexStructure
	{
		float pos[3];
		float uv[2];
		float normal[3];
	};

	struct SharedImage
	{
		uint32_t width{0};
		uint32_t height{0};
		uint32_t depth{0};

		VkImage        image{VK_NULL_HANDLE};
		VkDeviceMemory memory{VK_NULL_HANDLE};
		VkDeviceSize   size{0};
		VkDeviceSize   allocationSize{0};
		VkSampler      sampler{VK_NULL_HANDLE};
		VkImageView    view{VK_NULL_HANDLE};

	} shared_image;

	struct UniformBufferData
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 view_pos;
	} ubo_vs;

	struct OpenCLObjects
	{
		cl_context       context{nullptr};
		cl_device_id     device_id{nullptr};
		cl_command_queue command_queue{nullptr};
		cl_program       program{nullptr};
		cl_kernel        kernel{nullptr};
		cl_mem           image{nullptr};
		cl_semaphore_khr cl_update_vk_semaphore{nullptr};
		cl_semaphore_khr vk_update_cl_semaphore{nullptr};
		bool             initialized{false};
	} opencl_objects{};

	bool first_submit{true};

	VkPipeline            pipeline{VK_NULL_HANDLE};
	VkPipelineLayout      pipeline_layout{VK_NULL_HANDLE};
	VkDescriptorSet       descriptor_set{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};

	std::unique_ptr<vkb::core::Buffer> vertex_buffer;
	std::unique_ptr<vkb::core::Buffer> index_buffer;
	uint32_t                           index_count{0};
	std::unique_ptr<vkb::core::Buffer> uniform_buffer_vs;

	// @todo: rename
	VkSemaphore cl_update_vk_semaphore{VK_NULL_HANDLE};
	VkSemaphore vk_update_cl_semaphore{VK_NULL_HANDLE};

	VkFence rendering_finished_fence{VK_NULL_HANDLE};

	float total_time_passed{0};
};

std::unique_ptr<vkb::VulkanSample> create_open_cl_interop();
