/* Copyright (c) 2024, Sascha Willems
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
#include "gltf_loader.h"

// Can be used to toggle between renderpasses + subpasses and dynamic rendering + local read
#define USE_DYNAMIC_RENDERING

class DynamicRenderingLocalRead : public ApiVulkanSample
{
  public:
	DynamicRenderingLocalRead();
	virtual ~DynamicRenderingLocalRead();
	void prepare_pipelines();
	void build_command_buffers() override;
	void render(float delta_time) override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

  private:
	struct Scenes
	{
		std::unique_ptr<vkb::sg::Scene> opaque;
		std::unique_ptr<vkb::sg::Scene> transparent;
	} scenes;

	struct
	{
		Texture transparent_glass;
	} textures;

	struct
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
	} shader_data_vs;

	struct Light
	{
		glm::vec4 position;
		glm::vec3 color;
		float     radius{0};
	};

	std::array<Light, 64> lights;

	struct
	{
		std::unique_ptr<vkb::core::BufferC> ubo_vs;
		std::unique_ptr<vkb::core::BufferC> ssbo_lights;
	} buffers;

	struct PushConstantSceneNode
	{
		glm::mat4 matrix;
		glm::vec4 color;
	};

	struct Pass
	{
		VkPipelineLayout      pipeline_layout{VK_NULL_HANDLE};
		VkPipeline            pipeline{VK_NULL_HANDLE};
		VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set{VK_NULL_HANDLE};
	} scene_opaque_pass, scene_transparent_pass, composition_pass;

	struct FrameBufferAttachment
	{
		VkImage        image{VK_NULL_HANDLE};
		VkDeviceMemory memory{VK_NULL_HANDLE};
		VkImageView    view{VK_NULL_HANDLE};
		VkFormat       format{VK_FORMAT_UNDEFINED};
	};
	struct Attachments
	{
		FrameBufferAttachment positionDepth, normal, albedo;
	} attachments;

	int32_t attachment_width{0};
	int32_t attachment_height{0};

	void setup_framebuffer() override;
	void setup_render_pass() override;
	void prepare_gui() override;

	void load_assets();
	void create_attachment(VkFormat format, VkImageUsageFlags usage, FrameBufferAttachment &attachment);
	void destroy_attachment(FrameBufferAttachment &attachment);
	void create_attachments();
	void prepare_buffers();
	void update_lights_buffer();
	void update_uniform_buffer();
	void prepare_layouts_and_descriptors();

	void draw_scene(std::unique_ptr<vkb::sg::Scene> &scene, VkCommandBuffer cmd, VkPipelineLayout pipeline_layout);
};

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_dynamic_rendering_local_read();
