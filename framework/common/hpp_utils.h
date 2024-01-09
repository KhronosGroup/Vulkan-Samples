/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "utils.h"

#include "rendering/hpp_render_context.h"

/**
 * @brief facade helper functions around the functions in common/utils.h, providing a vulkan.hpp-based interface
 */
namespace vkb
{
namespace scene_graph
{
class HPPNode;
class HPPScene;

namespace components
{
class HPPLight;
}
}        // namespace scene_graph

namespace common
{
inline vkb::scene_graph::components::HPPLight &add_directional_light(vkb::scene_graph::HPPScene &scene, const glm::quat &rotation, const sg::LightProperties &props = {}, vkb::scene_graph::HPPNode *parent_node = nullptr)
{
	return reinterpret_cast<vkb::scene_graph::components::HPPLight &>(vkb::add_directional_light(reinterpret_cast<vkb::sg::Scene &>(scene), rotation, props, reinterpret_cast<vkb::sg::Node *>(parent_node)));
}

inline sg::Node &add_free_camera(vkb::scene_graph::HPPScene &scene, const std::string &node_name, vk::Extent2D const &extent)
{
	return vkb::add_free_camera(reinterpret_cast<vkb::sg::Scene &>(scene), node_name, static_cast<VkExtent2D>(extent));
}

inline void screenshot(vkb::rendering::HPPRenderContext &render_context, const std::string &filename)
{
	vkb::screenshot(reinterpret_cast<vkb::RenderContext &>(render_context), filename);
}
}        // namespace common
}        // namespace vkb
