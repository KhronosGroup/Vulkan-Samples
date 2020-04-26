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

#include "rendering/subpass.h"

namespace vkb
{
namespace sg
{
class Camera;
class Scene;
}        // namespace sg

/**
 * @brief Uniform structure for fragment shader
 */
struct alignas(16) PostprocessingUniform
{
	glm::vec2 near_far;
};

/**
 * @brief Post-Processing subpass, binds two full screen attachments (color and depth)
 *        to run screen based postprocessing effects
 *        If multisampling was used to save the color and depth attachments, color must
 *        have been resolved first
 *        Depth is allowed to be multisampled, and this subpass will create two shader
 *        variants to cope with both cases. It is however not recommended to store
 *        multisampled depth attachments, always resolve before storing if possible
 */
class PostProcessingSubpass : public Subpass
{
  public:
	PostProcessingSubpass(RenderContext &render_context, ShaderSource &&vertex_shader, ShaderSource &&fragment_shader, sg::Scene &scene, sg::Camera &camera);

	virtual void prepare() override;

	void draw(CommandBuffer &command_buffer) override;

	void set_full_screen_color(uint32_t attachment);

	void set_full_screen_depth(uint32_t attachment);

	void set_ms_depth(bool enable);

  private:
	sg::Camera &camera;

	sg::Scene &scene;

	std::unique_ptr<core::Sampler> color_sampler{nullptr};

	std::unique_ptr<core::Sampler> depth_sampler{nullptr};

	uint32_t full_screen_color{0};

	uint32_t full_screen_depth{0};

	/**
	 * @brief If true the full screen depth texture is multisampled.
	 *        Used to select the fragment shader variant that binds
	 *        the appropriate texture sampler for depth
	 */
	bool ms_depth{false};

	/**
	 * @brief Variant where depth is not multisampled
	 */
	ShaderVariant postprocessing_variant;

	/**
	 * @brief Variant where depth is multisampled and requires
	 *        a multisample texture sampler
	 */
	ShaderVariant postprocessing_variant_ms_depth;
};

}        // namespace vkb
