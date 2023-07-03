/* Copyright (c) 2023, Mobica Limited
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

/**
 * @brief Color generation toggle using VK_EXT_color_write_enable
 */

class ColorWriteEnable : public ApiVulkanSample
{
  public:
	ColorWriteEnable();
	virtual ~ColorWriteEnable();

	// Create pipeline
	void prepare_pipelines();

	// Override basic framework functionality
	void build_command_buffers() override;
	void render(float delta_time) override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void setup_render_pass() override;
	void setup_framebuffer() override;
	void prepare_gui() override;

  private:
	struct FrameBufferAttachment
	{
		VkImage        image;
		VkDeviceMemory mem;
		VkImageView    view;
		VkFormat       format;
		void           destroy(VkDevice device)
		{
			vkDestroyImageView(device, view, nullptr);
			vkDestroyImage(device, image, nullptr);
			vkFreeMemory(device, mem, nullptr);
		}
	};

	struct
	{
		FrameBufferAttachment red, green, blue;
		int32_t               width, height;
	} attachments;

	struct
	{
		VkSampler red, green, blue;
	} samplers;

	struct
	{
		VkPipeline color, composition;
	} pipelines;

	struct
	{
		VkPipelineLayout color, composition;
	} pipeline_layouts;

	struct
	{
		VkDescriptorSetLayout color, composition;
	} descriptor_set_layouts;

	struct
	{
		VkDescriptorSet composition;
	} descriptor_sets;

	void create_attachment(VkFormat format, FrameBufferAttachment *attachment);
	void create_attachments();
	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	void setup_descriptor_set();

	bool  r_bit_enabled      = true;
	bool  g_bit_enabled      = true;
	bool  b_bit_enabled      = true;
	float background_r_value = 0.5f;
	float background_g_value = 0.5f;
	float background_b_value = 0.5f;
};

std::unique_ptr<vkb::Application> create_color_write_enable();
