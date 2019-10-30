/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "common/utils.h"
#include "rendering/render_pipeline.h"
#include "rendering/subpasses/scene_subpass.h"
#include "scene_graph/components/camera.h"
#include "vulkan_sample.h"

/**
 * @brief Command buffer utilization
 */
class CommandBufferUsage : public vkb::VulkanSample
{
  public:
	CommandBufferUsage();

	virtual ~CommandBufferUsage() = default;

	virtual bool prepare(vkb::Platform &platform) override;

	virtual void update(float delta_time) override;

	class SceneSubpassSecondary : public vkb::SceneSubpass
	{
	  public:
		SceneSubpassSecondary(vkb::RenderContext &render_context, vkb::ShaderSource &&vertex_source, vkb::ShaderSource &&fragment_source, vkb::sg::Scene &scene, vkb::sg::Camera &camera);

		void draw(vkb::CommandBuffer &primary_command_buffer) override;

		void set_use_secondary_command_buffers(bool use_secondary);

		void set_command_buffer_reset_mode(vkb::CommandBuffer::ResetMode reset_mode);

		void set_viewport(VkViewport &viewport);

		void set_scissor(VkRect2D &scissor);

	  private:
		bool use_secondary_command_buffers{false};

		vkb::CommandBuffer::ResetMode command_buffer_reset_mode{vkb::CommandBuffer::ResetMode::ResetPool};

		VkViewport viewport{};

		VkRect2D scissor{};
	};

  private:
	SceneSubpassSecondary *scene_subpass_ptr{nullptr};

	vkb::sg::Camera *camera{nullptr};

	void render(vkb::CommandBuffer &command_buffer) override;

	void draw_swapchain_renderpass(vkb::CommandBuffer &primary_command_buffer, vkb::RenderTarget &render_target) override;

	void draw_gui() override;

	std::unique_ptr<vkb::CommandPool> command_pool{nullptr};

	bool use_secondary_command_buffers{false};

	int reuse_selection{0};
};

std::unique_ptr<vkb::VulkanSample> create_command_buffer_usage();
