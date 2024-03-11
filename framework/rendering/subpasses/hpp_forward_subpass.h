/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "rendering/subpasses/forward_subpass.h"

#include <rendering/hpp_render_context.h>

namespace vkb
{
namespace scene_graph
{
class HPPScene;
}        // namespace scene_graph

namespace rendering
{
namespace subpasses
{
/**
 * @brief facade class around vkb::ForwardSubpass, providing a vulkan.hpp-based interface
 *
 * See vkb::ForwardSubpass for documentation
 */
class HPPForwardSubpass : public vkb::ForwardSubpass
{
  public:
	HPPForwardSubpass(vkb::rendering::HPPRenderContext &render_context,
	                  vkb::ShaderSource               &&vertex_shader,
	                  vkb::ShaderSource               &&fragment_shader,
	                  vkb::scene_graph::HPPScene       &scene,
	                  vkb::sg::Camera                  &camera) :
	    vkb::ForwardSubpass(reinterpret_cast<vkb::RenderContext &>(render_context),
	                        std::forward<ShaderSource>(vertex_shader),
	                        std::forward<ShaderSource>(fragment_shader),
	                        reinterpret_cast<vkb::sg::Scene &>(scene),
	                        camera)
	{}
};

}        // namespace subpasses
}        // namespace rendering
}        // namespace vkb
