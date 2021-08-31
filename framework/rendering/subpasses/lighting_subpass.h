/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "buffer_pool.h"
#include "common/glm.h"
#include "rendering/subpass.h"

// This value is per type of light that we feed into the shader
#define MAX_DEFERRED_LIGHT_COUNT 32

namespace vkb
{
namespace sg
{
class Camera;
class Light;
class Scene;
}        // namespace sg

/**
 * @brief Light uniform structure for lighting shader
 * Inverse view projection matrix and inverse resolution vector are used
 * in lighting pass to reconstruct position from depth and frag coord
 */
struct alignas(16) LightUniform
{
	glm::mat4 inv_view_proj;
	glm::vec2 inv_resolution;
};

struct alignas(16) DeferredLights
{
	Light directional_lights[MAX_DEFERRED_LIGHT_COUNT];
	Light point_lights[MAX_DEFERRED_LIGHT_COUNT];
	Light spot_lights[MAX_DEFERRED_LIGHT_COUNT];
};

/**
 * @brief Lighting pass of Deferred Rendering
 */
class LightingSubpass : public Subpass
{
  public:
	LightingSubpass(RenderContext &render_context, ShaderSource &&vertex_shader, ShaderSource &&fragment_shader, sg::Camera &camera, sg::Scene &scene);

	void prepare() override;

	void draw(CommandBuffer &command_buffer) override;

  private:
	sg::Camera &camera;

	sg::Scene &scene;

	ShaderVariant lighting_variant;
};

}        // namespace vkb
