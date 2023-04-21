/* Copyright (c) 2020, Arm Limited and Contributors
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

#include <ctpl_stl.h>

#include "core/command_buffer.h"
#include "rendering/render_pipeline.h"
#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/camera.h"
#include "vulkan_sample.h"

struct alignas(16) ShadowUniform
{
	glm::mat4 shadowmap_projection_matrix;        // Projection matrix used to render shadowmap
};

/**
 * @brief Multithreading with Render Passes
 * This sample shows performance improvement when using multithreading with 
 * multiple render passes and primary level command buffers.
 */
class MultithreadingRenderPasses : public vkb::VulkanSample
{
  public:
	enum class MultithreadingMode
	{
		None                    = 0,
		PrimaryCommandBuffers   = 1,
		SecondaryCommandBuffers = 2,
	};

	MultithreadingRenderPasses();

	virtual ~MultithreadingRenderPasses() = default;

	virtual bool prepare(vkb::Platform &platform) override;

	virtual void update(float delta_time) override;

	void draw_gui() override;

	/**
     * @brief This subpass is responsible for rendering a shadowmap
     */
	class ShadowSubpass : public vkb::GeometrySubpass
	{
	  public:
		ShadowSubpass(vkb::RenderContext &render_context,
		              vkb::ShaderSource &&vertex_source,
		              vkb::ShaderSource &&fragment_source,
		              vkb::sg::Scene &    scene,
		              vkb::sg::Camera &   camera);

	  protected:
		virtual void prepare_pipeline_state(vkb::CommandBuffer &command_buffer, VkFrontFace front_face, bool double_sided_material) override;

		virtual vkb::PipelineLayout &prepare_pipeline_layout(vkb::CommandBuffer &command_buffer, const std::vector<vkb::ShaderModule *> &shader_modules) override;

		virtual void prepare_push_constants(vkb::CommandBuffer &command_buffer, vkb::sg::SubMesh &sub_mesh) override;
	};

	/**
     * @brief This subpass is responsible for rendering a Scene
     *		  It implements a custom draw function which passes shadowmap and light matrix
     */
	class MainSubpass : public vkb::ForwardSubpass
	{
	  public:
		MainSubpass(vkb::RenderContext &                             render_context,
		            vkb::ShaderSource &&                             vertex_source,
		            vkb::ShaderSource &&                             fragment_source,
		            vkb::sg::Scene &                                 scene,
		            vkb::sg::Camera &                                camera,
		            vkb::sg::Camera &                                shadowmap_camera,
		            std::vector<std::unique_ptr<vkb::RenderTarget>> &shadow_render_targets);

		virtual void prepare() override;

		virtual void draw(vkb::CommandBuffer &command_buffer) override;

	  private:
		std::unique_ptr<vkb::core::Sampler> shadowmap_sampler{};

		vkb::sg::Camera &shadowmap_camera;

		std::vector<std::unique_ptr<vkb::RenderTarget>> &shadow_render_targets;
	};

  private:
	virtual void prepare_render_context() override;

	std::unique_ptr<vkb::RenderTarget> create_shadow_render_target(uint32_t size);

	/**
     * @return Shadow render pass which should run first
     */
	std::unique_ptr<vkb::RenderPipeline> create_shadow_renderpass();

	/**
     * @return Main render pass which should run second
     */
	std::unique_ptr<vkb::RenderPipeline> create_main_renderpass();

	const uint32_t SHADOWMAP_RESOLUTION{1024};

	std::vector<std::unique_ptr<vkb::RenderTarget>> shadow_render_targets;

	/**
	 * @brief Pipeline for shadowmap rendering
	 */
	std::unique_ptr<vkb::RenderPipeline> shadow_render_pipeline{};

	/**
	 * @brief Pipeline which uses shadowmap 
	 */
	std::unique_ptr<vkb::RenderPipeline> main_render_pipeline{};

	/**
	 * @brief Subpass for shadowmap rendering  
	 */
	ShadowSubpass *shadow_subpass{};

	/**
	 * @brief Camera for shadowmap rendering (view from the light source)
	 */
	vkb::sg::Camera *shadowmap_camera{};

	/**
	 * @brief Main camera for scene rendering
	 */
	vkb::sg::Camera *camera{};

	ctpl::thread_pool thread_pool;

	uint32_t swapchain_attachment_index{0};

	uint32_t depth_attachment_index{1};

	uint32_t shadowmap_attachment_index{0};

	int multithreading_mode{0};

	/**
	 * @brief Record drawing commands using the chosen strategy
     * @param main_command_buffer Already allocated command buffer for the main pass
     * @return Single or multiple recorded command buffers
	 */
	std::vector<vkb::CommandBuffer *> record_command_buffers(vkb::CommandBuffer &main_command_buffer);

	void record_separate_primary_command_buffers(std::vector<vkb::CommandBuffer *> &command_buffers, vkb::CommandBuffer &main_command_buffer);

	void record_separate_secondary_command_buffers(std::vector<vkb::CommandBuffer *> &command_buffers, vkb::CommandBuffer &main_command_buffer);

	void record_main_pass_image_memory_barriers(vkb::CommandBuffer &command_buffer);

	void record_shadow_pass_image_memory_barrier(vkb::CommandBuffer &command_buffer);

	void record_present_image_memory_barrier(vkb::CommandBuffer &command_buffer);

	void draw_shadow_pass(vkb::CommandBuffer &command_buffer);

	void draw_main_pass(vkb::CommandBuffer &command_buffer);
};

std::unique_ptr<vkb::VulkanSample> create_multithreading_render_passes();
