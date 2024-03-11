/* Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "light.h"

namespace vkb
{
namespace scene_graph
{
class HPPNode;

namespace components
{
/**
 * @brief facade class around vkb::sg::Light, providing a vulkan.hpp-based interface
 *
 * See vkb::sg::Light for documentation
 */
class HPPLight : private vkb::sg::Light
{
  public:
	using ComponentType = vkb::sg::Light;

	using vkb::sg::Light::set_light_type;
	using vkb::sg::Light::set_properties;

	HPPLight(const std::string &name) :
	    vkb::sg::Light(name)
	{}

	void set_node(vkb::scene_graph::HPPNode &node)
	{
		vkb::sg::Light::set_node(reinterpret_cast<vkb::sg::Node &>(node));
	}
};
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
