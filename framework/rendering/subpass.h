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

#include "common/helpers.h"
#include "core/shader_module.h"
#include "rendering/pipeline_state.h"

namespace vkb
{
class RenderContext;
class CommandBuffer;

/**
 * @brief Calculates the vulkan style projection matrix
 * @param proj The projection matrix
 * @return The vulkan style projection matrix
 */
glm::mat4 vulkan_style_projection(const glm::mat4 &proj);

/**
 * @brief This class defines an interface for subpasses
 *        where they need to implement the draw function.
 *        It is used to construct a RenderPipeline
 */
class Subpass : public NonCopyable
{
  public:
	Subpass(RenderContext &render_context, ShaderSource &&vertex_shader, ShaderSource &&fragment_shader);

	virtual ~Subpass() = default;

	/**
	 * @brief Updates the render target attachments with the ones stored in this subpass
	 *        This function is called by the RenderPipeline before beginning the render
	 *        pass and before proceeding with a new subpass.
	 */
	void update_render_target_attachments();

	/**
	 * @brief Draw virtual function
	 * @param command_buffer Command buffer to use to record draw commands
	 */
	virtual void draw(CommandBuffer &command_buffer) = 0;

	RenderContext &get_render_context();

	const ShaderSource &get_vertex_shader() const;

	const ShaderSource &get_fragment_shader() const;

	DepthStencilState &get_depth_stencil_state();

	const std::vector<uint32_t> &get_input_attachments() const;

	void set_input_attachments(std::vector<uint32_t> input);

	const std::vector<uint32_t> &get_output_attachments() const;

	void set_output_attachments(std::vector<uint32_t> output);

  protected:
	RenderContext &render_context;

  private:
	ShaderSource vertex_shader;

	ShaderSource fragment_shader;

	DepthStencilState depth_stencil_state{};

	/// Default to no input attachments
	std::vector<uint32_t> input_attachments = {};

	/// Default to swapchain output attachment
	std::vector<uint32_t> output_attachments = {0};
};

}        // namespace vkb
