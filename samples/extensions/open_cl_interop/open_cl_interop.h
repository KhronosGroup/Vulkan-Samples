/* Copyright (c) 2021, Arm Limited and Contributors
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

struct CLData;

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
	void   prepare_pipelines();
	void   prepare_open_cl_resources();
	void   prepare_shared_resources();
	void   generate_quad();
	void   setup_descriptor_pool();
	void   setup_descriptor_set_layout();
	void   setup_descriptor_set();
	void   prepare_uniform_buffers();
	void   update_uniform_buffers();	
	void   update_texture_from_open_cl();

	// @todo: Handle type not available outside windows
	HANDLE get_vulkan_image_handle(VkExternalMemoryHandleTypeFlagsKHR external_memory_handle_type);

	struct VertexStructure
	{
		float pos[3];
		float uv[2];
		float normal[3];
	};

	struct SharedTexture
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

	} shared_texture;

	struct UniformBufferData
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 view_pos;
	} ubo_vs;

	VkPipeline            pipeline{VK_NULL_HANDLE};
	VkPipelineLayout      pipeline_layout{VK_NULL_HANDLE};
	VkDescriptorSet       descriptor_set{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};

	std::unique_ptr<vkb::core::Buffer> vertex_buffer;
	std::unique_ptr<vkb::core::Buffer> index_buffer;
	uint32_t                           index_count{0};
	std::unique_ptr<vkb::core::Buffer> uniform_buffer_vs;

	VkFence rendering_finished_fence{VK_NULL_HANDLE};

	float total_time_passed{0};

	CLData *cl_data{nullptr};

#ifdef _WIN32
	PFN_vkGetMemoryWin32HandleKHR    vkGetMemoryWin32HandleKHR{nullptr};
	PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR{nullptr};
#else
	PFN_vkGetMemoryFdKHR    vkGetMemoryFdKHR{nullptr};
	PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR{nullptr};
#endif
};

std::unique_ptr<vkb::VulkanSample> create_open_cl_interop();
