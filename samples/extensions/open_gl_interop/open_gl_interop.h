/* Copyright (c) 2020, Bradley Austin Davis
 * Copyright (c) 2020, Arm Limited
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
#include "timer.h"

class OpenGLWindow;

#if defined(WIN32)
using Handle = HANDLE;
#else
using Handle                          = int;
constexpr Handle INVALID_HANDLE_VALUE = static_cast<Handle>(-1);
#endif

// Vertex layout for this example
struct VertexStructure
{
	float pos[3];
	float uv[2];
	float normal[3];
};

class OffscreenContext;
struct GLData;

class OpenGLInterop : public ApiVulkanSample
{
  public:
	OpenGLInterop();
	~OpenGLInterop() override;

	bool prepare(vkb::Platform &platform) override;

	void render(float delta_time) override;
	void build_command_buffers() override;
	void view_changed() override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

  private:
	void prepare_shared_resources();
	void generate_quad();
	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	void setup_descriptor_set();
	void prepare_pipelines();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void draw();

	vkb::Timer        timer;
	OffscreenContext *gl_context{nullptr};
	GLData *          gl_data{nullptr};

	struct ShareHandles
	{
		Handle memory{INVALID_HANDLE_VALUE};
		Handle gl_ready{INVALID_HANDLE_VALUE};
		Handle gl_complete{INVALID_HANDLE_VALUE};
	} shareHandles;

	struct SharedTexture
	{
		VkImage        image{VK_NULL_HANDLE};
		VkDeviceMemory memory{VK_NULL_HANDLE};
		VkDeviceSize   size{0};
		VkDeviceSize   allocationSize{0};
		VkSampler      sampler{VK_NULL_HANDLE};
		VkImageView    view{VK_NULL_HANDLE};
	} sharedTexture;

	struct Semaphores
	{
		VkSemaphore gl_ready{VK_NULL_HANDLE};
		VkSemaphore gl_complete{VK_NULL_HANDLE};
	} sharedSemaphores;

	std::unique_ptr<vkb::core::Buffer> vertex_buffer;
	std::unique_ptr<vkb::core::Buffer> index_buffer;
	uint32_t                           index_count;

	std::unique_ptr<vkb::core::Buffer> uniform_buffer_vs;

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
};

std::unique_ptr<vkb::VulkanSample> create_open_gl_interop();
