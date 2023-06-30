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

#include "rendering/postprocessing_pipeline.h"
#include "rendering/render_pipeline.h"
#include "scene_graph/components/perspective_camera.h"
#include "vulkan_sample.h"

/**
 * @brief MSAA Sample
 *
 * This sample shows the benefits of multisample anti-aliasing (MSAA) and how to
 * resolve the multisampled attachments with minimum impact on performance.
 *
 * The UI controls allow the user to choose between different levels of MSAA
 * and to select whether or not to resolve the color and depth attachments
 * within the render pass.
 *
 * Resolving within the renderpass is very efficient on mobile since usually
 * tilers can resolve the multisampled attachments on writeback to
 * main memory. This means on mobile it is possible to get considerable
 * quality improvements with relatively little cost.
 *
 * The alternative to resolving within the renderpass is to have a separate
 * color resolve pass (using vkCmdResolveImage), which is much less efficient.
 *
 * Resolving on writeback was only possible with color attachments, but
 * VK_KHR_depth_stencil_resolve (promoted in Vulkan 1.2) makes it possible to
 * also resolve the depth attachment within the renderpass.
 *
 * Without this extension there is no simple alternative to resolve depth
 * in a separate pass (vkCmdResolveImage does not support depth).
 *
 * Usually the depth attachment is transient (it is not needed outside the
 * render pass) but some postprocessing effects require it as an input texture.
 * Without the extension, if MSAA is enabled in the geometry pass, the
 * multisampled depth attachment would be have to be written out to be
 * consumed by the postprocessing renderpass.
 *
 * As with unresolved color, writing out unresolved depth attachments is very
 * bandwidth intensive and therefore depth-based postprocessing was
 * usually avoided on mobile platforms.
 *
 * This sample shows how to use the extension to also resolve the depth
 * attachment on writeback and use it in a simple postprocessing pass.
 */
class MSAASample : public vkb::VulkanSample
{
  public:
	MSAASample();

	virtual ~MSAASample() = default;

	virtual bool prepare(const vkb::ApplicationOptions &options) override;

	virtual void update(float delta_time) override;

	virtual void draw(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target) override;

	void draw_gui() override;

  private:
	vkb::sg::PerspectiveCamera *camera{nullptr};

	virtual void prepare_render_context() override;

	std::unique_ptr<vkb::RenderTarget> create_render_target(vkb::core::Image &&swapchain_image);

	/**
	 * @brief Scene pipeline
	 *        Render and light the scene (optionally using MSAA)
	 */
	std::unique_ptr<vkb::RenderPipeline> scene_pipeline{};

	/**
	 * @brief Postprocessing pipeline
	 *        Read in the output color and depth attachments from the
	 *        scene subpass and use them to apply a screen-based effect
	 */
	std::unique_ptr<vkb::PostProcessingPipeline> postprocessing_pipeline{};

	/**
	 * @brief Update MSAA options and accordingly set the load/store
	 *        attachment operations for the renderpasses
	 *        This will trigger a swapchain recreation
	 */
	void update_pipelines();

	/**
	 * @brief Update pipelines given that there will be a single
	 *        renderpass for rendering the scene and GUI only
	 */
	void update_for_scene_only(bool msaa_enabled);

	/**
	 * @brief Update pipelines given that there will be two renderpasses
	 *        The first renderpass will draw the scene and save the output
	 *        color and depth attachments which will be read in by
	 *        a postprocessing renderpass
	 */
	void update_for_scene_and_postprocessing(bool msaa_enabled);

	/**
	 * @brief If true the postprocessing renderpass is enabled
	 */
	bool run_postprocessing{false};

	/**
	 * @brief Submits a postprocessing renderpass which binds full screen color
	 *        and depth attachments and uses them to apply a screen-based effect
	 *        It also draws the GUI
	 */
	void postprocessing(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target,
	                    VkImageLayout &swapchain_layout, bool msaa_enabled);

	/**
	 * @brief Enables MSAA if set to more than 1 sample per pixel
	 *        (e.g. sample count 4 enables 4X MSAA)
	 */
	VkSampleCountFlagBits sample_count{VK_SAMPLE_COUNT_1_BIT};

	/**
	 * @brief List of MSAA levels supported by the platform
	 */
	std::vector<VkSampleCountFlagBits> supported_sample_count_list{};

	/**
	 * @brief Queries the Vulkan device to construct the list of supported
	 *        sample counts
	 */
	void prepare_supported_sample_count_list();

	enum ColorResolve : int
	{
		OnWriteback  = 0,
		SeparatePass = 1
	};

	/**
	 * @brief Selects how to resolve the color attachment, either on writeback
	 *        (efficient) or in a separate pass (inefficient)
	 */
	int color_resolve_method{ColorResolve::OnWriteback};

	/**
	 * @brief Sets the multisampled color attachment as the output attachment
	 *        and configures the resolve operation to resolve_attachment
	 *        as well as the load/store operations of color attachments
	 *        Note that MSAA will not have any effect in the postprocessing
	 *        renderpass since it only renders a texture on single full-screen
	 *        triangle and MSAA only works on primitive edges
	 */
	void use_multisampled_color(std::unique_ptr<vkb::Subpass> &subpass, std::vector<vkb::LoadStoreInfo> &load_store, uint32_t resolve_attachment);

	/**
	 * @brief Sets the single-sampled output_attachment as the output attachment,
	 *        disables color resolve and updates the load/store operations of
	 *        color attachments
	 */
	void use_singlesampled_color(std::unique_ptr<vkb::Subpass> &subpass, std::vector<vkb::LoadStoreInfo> &load_store, uint32_t output_attachment);

	/**
	 * @brief Submits a transfer operation to resolve the multisampled color attachment
	 *        to the given single-sampled resolve attachment
	 *        color_layout is an in-out parameter that holds the last known layout
	 *        of the resolve attachment, and may be used for any further transitions
	 */
	void resolve_color_separate_pass(vkb::CommandBuffer &command_buffer, const std::vector<vkb::core::ImageView> &views,
	                                 uint32_t color_destination, VkImageLayout &color_layout);

	/**
	 * @brief If true, the platform supports the VK_KHR_depth_stencil_resolve extension
	 *        and therefore can resolve the depth attachment on writeback
	 */
	bool depth_writeback_resolve_supported{false};

	/**
	 * @brief If true, enable writeback depth resolve
	 *        If false the multisampled depth attachment will be stored
	 *        (only if postprocessing is enabled since the attachment is
	 *        otherwise unused)
	 */
	bool resolve_depth_on_writeback{true};

	/**
	 * @brief Store the multisampled depth attachment, resolved to a single-sampled
	 *        attachment if depth resolve on writeback is supported
	 *        Update the load/store operations of the depth attachments
	 */
	void store_multisampled_depth(std::unique_ptr<vkb::Subpass> &subpass, std::vector<vkb::LoadStoreInfo> &load_store);

	/**
	 * @brief Disables depth writeback resolve and updates the load/store operations of
	 *        the depth resolve attachment
	 */
	void disable_depth_writeback_resolve(std::unique_ptr<vkb::Subpass> &subpass, std::vector<vkb::LoadStoreInfo> &load_store);

	/**
	 * @brief Selects the depth resolve mode (e.g. min or max sample values)
	 */
	VkResolveModeFlagBits depth_resolve_mode{VK_RESOLVE_MODE_NONE};

	/**
	 * @brief List of depth resolve modes supported by the platform
	 */
	std::vector<VkResolveModeFlagBits> supported_depth_resolve_mode_list{};

	/**
	 * @brief Queries the Vulkan device to construct the list of supported
	 *        depth resolve modes
	 */
	void prepare_depth_resolve_mode_list();

	/* Helpers for managing attachments */

	uint32_t i_swapchain{0};

	uint32_t i_depth{0};

	uint32_t i_color_ms{0};

	uint32_t i_color_resolve{0};

	uint32_t i_depth_resolve{0};

	std::vector<uint32_t> color_atts{};

	std::vector<uint32_t> depth_atts{};

	std::vector<vkb::LoadStoreInfo> scene_load_store{};

	/* Helpers for managing GUI input */

	bool gui_run_postprocessing{false};

	bool last_gui_run_postprocessing{false};

	VkSampleCountFlagBits gui_sample_count{VK_SAMPLE_COUNT_1_BIT};

	VkSampleCountFlagBits last_gui_sample_count{VK_SAMPLE_COUNT_1_BIT};

	int gui_color_resolve_method{ColorResolve::OnWriteback};

	int last_gui_color_resolve_method{ColorResolve::OnWriteback};

	bool gui_resolve_depth_on_writeback{true};

	bool last_gui_resolve_depth_on_writeback{true};

	VkResolveModeFlagBits gui_depth_resolve_mode{VK_RESOLVE_MODE_NONE};

	VkResolveModeFlagBits last_gui_depth_resolve_mode{VK_RESOLVE_MODE_NONE};
};

std::unique_ptr<vkb::VulkanSample> create_msaa();
