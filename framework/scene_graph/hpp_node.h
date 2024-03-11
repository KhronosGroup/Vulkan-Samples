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

#include "components/hpp_transform.h"
#include "node.h"

namespace vkb
{
namespace scene_graph
{
/**
 * @brief facade class around vkb::sg::Node, providing a vulkan.hpp-based interface
 *
 * See vkb::sg::Node for documentation
 */
class HPPNode : private vkb::sg::Node
{
  public:
	using vkb::sg::Node::get_component;
	using vkb::sg::Node::get_name;

	HPPNode(const size_t id, const std::string &name) :
	    vkb::sg::Node(id, name)
	{}

	void add_child(vkb::scene_graph::HPPNode &child)
	{
		vkb::sg::Node::add_child(reinterpret_cast<vkb::sg::Node &>(child));
	}

	template <typename T>
	T &get_component()
	{
		return reinterpret_cast<T &>(vkb::sg::Node::get_component<typename T::ComponentType>());
	}

	vkb::scene_graph::components::HPPTransform &get_transform()
	{
		return reinterpret_cast<vkb::scene_graph::components::HPPTransform &>(vkb::sg::Node::get_transform());
	}

	template <typename T>
	void set_component(T &component)
	{
		vkb::sg::Node::set_component(reinterpret_cast<typename T::ComponentType &>(component));
	}

	void set_parent(vkb::scene_graph::HPPNode &parent)
	{
		vkb::sg::Node::set_parent(reinterpret_cast<vkb::sg::Node &>(parent));
	}
};
}        // namespace scene_graph
}        // namespace vkb
