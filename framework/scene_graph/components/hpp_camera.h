/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "camera.h"

namespace vkb
{
namespace scene_graph
{
class HPPNode;

namespace components
{
/**
 * @brief facade class around vkb::sg::Camera, providing a vulkan.hpp-based interface
 *
 * See vkb::sg::Camera for documentation
 */
class HPPCamera : private vkb::sg::Camera
{
  public:
	using ComponentType = vkb::sg::Camera;

	using vkb::sg::Camera::get_name;

	HPPCamera(std::string const &name) :
	    vkb::sg::Camera(name)
	{}

	vkb::scene_graph::HPPNode *get_node()
	{
		return reinterpret_cast<vkb::scene_graph::HPPNode *>(vkb::sg::Camera::get_node());
	}

	void set_node(vkb::scene_graph::HPPNode &node)
	{
		vkb::sg::Camera::set_node(reinterpret_cast<vkb::sg::Node &>(node));
	}
};
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
