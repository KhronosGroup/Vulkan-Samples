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

#include "rendering/render_pipeline.h"
#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/camera.h"
#include "timer.h"
#include "vulkan_sample.h"

/**
 * @brief Using multiple queues to achieve more parallelism on the GPU
 */
class AsyncComputeSample : public vkb::VulkanSample
{
  public:
	AsyncComputeSample();

	virtual ~AsyncComputeSample() = default;

	virtual bool prepare(vkb::Platform &platform) override;

	virtual void update(float delta_time) override;

	virtual void finish() override;

  private:
	vkb::sg::Camera *camera{nullptr};
	vkb::sg::Camera *shadow_camera{nullptr};

	virtual void draw_gui() override;

	std::chrono::system_clock::time_point start_time;

	void        render_shadow_pass();
	VkSemaphore render_forward_offscreen_pass(VkSemaphore hdr_wait_semaphore);
	VkSemaphore render_compute_post(VkSemaphore wait_graphics_semaphore, VkSemaphore wait_present_semaphore);
	VkSemaphore render_swapchain(VkSemaphore post_semaphore);
	void        setup_queues();

	void                                               prepare_render_targets();
	std::unique_ptr<vkb::RenderTarget>                 forward_render_targets[2];
	std::unique_ptr<vkb::RenderTarget>                 shadow_render_target;
	vkb::RenderPipeline                                shadow_render_pipeline;
	vkb::RenderPipeline                                forward_render_pipeline;
	std::unique_ptr<vkb::core::Sampler>                comparison_sampler;
	std::unique_ptr<vkb::core::Sampler>                linear_sampler;
	std::vector<std::unique_ptr<vkb::core::Image>>     blur_chain;
	std::vector<std::unique_ptr<vkb::core::ImageView>> blur_chain_views;

	vkb::PipelineLayout *threshold_pipeline{nullptr};
	vkb::PipelineLayout *blur_down_pipeline{nullptr};
	vkb::PipelineLayout *blur_up_pipeline{nullptr};

	const vkb::Queue *early_graphics_queue{nullptr};
	const vkb::Queue *present_graphics_queue{nullptr};
	const vkb::Queue *post_compute_queue{nullptr};

	VkSemaphore hdr_wait_semaphores[2]{};
	VkSemaphore compute_post_semaphore{};
	bool        async_enabled{false};
	bool        rotate_shadows{true};
	bool        last_async_enabled{false};
	bool        double_buffer_hdr_frames{false};
	unsigned    forward_render_target_index{};

	struct DepthMapSubpass : vkb::ForwardSubpass
	{
		DepthMapSubpass(vkb::RenderContext &render_context,
		                vkb::ShaderSource &&vertex_shader, vkb::ShaderSource &&fragment_shader,
		                vkb::sg::Scene &scene, vkb::sg::Camera &camera);
		virtual void draw(vkb::CommandBuffer &command_buffer) override;
	};

	struct ShadowMapForwardSubpass : vkb::ForwardSubpass
	{
		ShadowMapForwardSubpass(vkb::RenderContext &render_context,
		                        vkb::ShaderSource &&vertex_shader, vkb::ShaderSource &&fragment_shader,
		                        vkb::sg::Scene &scene, vkb::sg::Camera &camera, vkb::sg::Camera &shadow_camera);
		void         set_shadow_map(const vkb::core::ImageView *view, const vkb::core::Sampler *sampler);
		virtual void draw(vkb::CommandBuffer &command_buffer) override;

		const vkb::core::ImageView *shadow_view{nullptr};
		const vkb::core::Sampler *  shadow_sampler{nullptr};
		vkb::sg::Camera &           shadow_camera;
	};

	struct CompositeSubpass : vkb::Subpass
	{
		CompositeSubpass(vkb::RenderContext &render_context,
		                 vkb::ShaderSource &&vertex_shader, vkb::ShaderSource &&fragment_shader);
		void         set_texture(const vkb::core::ImageView *hdr_view, const vkb::core::ImageView *bloom_view,
		                         const vkb::core::Sampler *sampler);
		virtual void draw(vkb::CommandBuffer &command_buffer) override;
		virtual void prepare() override;

		const vkb::core::ImageView *hdr_view{nullptr};
		const vkb::core::ImageView *bloom_view{nullptr};
		const vkb::core::Sampler *  sampler{nullptr};
		vkb::PipelineLayout *       layout{nullptr};
	};

	vkb::RenderTarget &get_current_forward_render_target();
};

std::unique_ptr<vkb::VulkanSample> create_async_compute();
