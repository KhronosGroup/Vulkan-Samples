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

#include "rendering/render_pipeline.h"
#include "scene_graph/components/perspective_camera.h"
#include "vulkan_sample.h"

/**
  * @brief The RenderSubpasses sample shows how a significant amount of bandwidth
  *        (L2 cache ext reads and writes) can be saved, by using sub-passes instead
  *        of multiple render passes. In order to highlight the difference, it
  *        implements deferred rendering with and without sub-passes, giving the
  *        user the possibility to change some key settings.
  */
class RenderSubpasses : public vkb::VulkanSample
{
  public:
	RenderSubpasses();

	bool prepare(vkb::Platform &platform) override;

	void update(float delta_time) override;

	virtual ~RenderSubpasses() = default;

	void draw_gui() override;

  private:
	/**
	 * @brief Prepares the custom render context for render subpasses
	 */
	virtual void prepare_render_context() override;

	/**
	 * @return A good pipeline
	 */
	std::unique_ptr<vkb::RenderPipeline> create_one_renderpass_two_subpasses();

	/**
	 * @return A geometry render pass which should run first
	 */
	std::unique_ptr<vkb::RenderPipeline> create_geometry_renderpass();

	/**
	 * @return A lighting render pass which should run second
	 */
	std::unique_ptr<vkb::RenderPipeline> create_lighting_renderpass();

	/**
	 * @brief Draws using the good pipeline: one render pass with two sub-passes
	 */
	void draw_render_subpasses(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target);

	/**
	 * @brief Draws using the bad practice: two separate render passes
	 */
	void draw_renderpasses(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target);

	void draw_swapchain_renderpass(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target) override;

	vkb::RenderTarget create_render_target(vkb::core::Image &&swapchain_image);

	/// Good pipeline with two subpasses within one render pass
	std::unique_ptr<vkb::RenderPipeline> render_pipeline{};

	/// 1. Bad pipeline with a geometry subpass in the first render pass
	std::unique_ptr<vkb::RenderPipeline> geometry_render_pipeline{};

	/// 2. Bad pipeline with a lighting subpass in the second render pass
	std::unique_ptr<vkb::RenderPipeline> lighting_render_pipeline{};

	vkb::sg::PerspectiveCamera *camera{};

	/**
	 * @brief Struct that contains configurations for this sample
	 *        with description, options, and current selected value
	 */
	struct Config
	{
		/**
		 * @brief Configurations type
		 */
		enum Type
		{
			RenderTechnique,
			TransientAttachments,
			GBufferSize
		} type;

		/// Used as label by the GUI
		const char *description;

		/// List of options to choose from
		std::vector<const char *> options;

		/// Index of the current selected option
		int value;
	};

	uint16_t last_render_technique{0};
	uint16_t last_transient_attachment{0};
	uint16_t last_g_buffer_size{0};

	VkFormat          albedo_format{VK_FORMAT_R8G8B8A8_UNORM};
	VkFormat          normal_format{VK_FORMAT_A2B10G10R10_UNORM_PACK32};
	VkImageUsageFlags rt_usage_flags{VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT};

	std::vector<Config> configs = {
	    {/* config      = */ Config::RenderTechnique,
	     /* description = */ "Render technique",
	     /* options     = */ {"Subpasses", "Renderpasses"},
	     /* value       = */ 0},
	    {/* config      = */ Config::TransientAttachments,
	     /* description = */ "Transient attachments",
	     /* options     = */ {"Enabled", "Disabled"},
	     /* value       = */ 0},
	    {/* config      = */ Config::GBufferSize,
	     /* description = */ "G-Buffer size",
	     /* options     = */ {"128-bit", "More"},
	     /* value       = */ 0}};
};

std::unique_ptr<vkb::VulkanSample> create_render_subpasses();
