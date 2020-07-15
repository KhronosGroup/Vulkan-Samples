/* Copyright (c) 2018-2020, Arm Limited and Contributors
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

#include "common/error.h"

#include "buffer_pool.h"
#include "rendering/subpasses/geometry_subpass.h"

#define MAX_FORWARD_LIGHT_COUNT 16

namespace vkb
{
namespace sg
{
class Scene;
class Node;
class Mesh;
class SubMesh;
class Camera;
}        // namespace sg

struct alignas(16) ForwardLights
{
	Light lights[MAX_FORWARD_LIGHT_COUNT];
};

/**
 * @brief This subpass is responsible for rendering a Scene
 */
class ForwardSubpass : public GeometrySubpass
{
  public:
	/**
	 * @brief Constructs a subpass designed for forward rendering
	 * @param render_context Render context
	 * @param vertex_shader Vertex shader source
	 * @param fragment_shader Fragment shader source
	 * @param scene Scene to render on this subpass
	 * @param camera Camera used to look at the scene
	 */
	ForwardSubpass(RenderContext &render_context, ShaderSource &&vertex_shader, ShaderSource &&fragment_shader, sg::Scene &scene, sg::Camera &camera);

	virtual ~ForwardSubpass() = default;

	virtual void prepare() override;

	/**
	 * @brief Record draw commands
	 */
	virtual void draw(CommandBuffer &command_buffer) override;
};

}        // namespace vkb
