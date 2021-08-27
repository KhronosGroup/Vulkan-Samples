/* Copyright (c) 2018-2019, Arm Limited and Contributors
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

#include "node.h"

#include <utility>

#include "component.h"
#include "components/transform.h"

namespace vkb
{
namespace sg
{
Node::Node(const size_t id, std::string name) :
    id{id},
    name{std::move(name)},
    transform{*this}
{
	set_component(transform);
}

size_t Node::get_id() const
{
	return id;
}

const std::string &Node::get_name() const
{
	return name;
}

void Node::set_parent(Node &p)
{
	parent = &p;

	transform.invalidate_world_matrix();
}

Node *Node::get_parent() const
{
	return parent;
}

void Node::add_child(Node &child)
{
	children.push_back(&child);
}

const std::vector<Node *> &Node::get_children() const
{
	return children;
}

void Node::set_component(Component &component)
{
	auto it = components.find(component.get_type());

	if (it != components.end())
	{
		it->second = &component;
	}
	else
	{
		components.insert(std::make_pair(component.get_type(), &component));
	}
}

Component &Node::get_component(const std::type_index index)
{
	return *components.at(index);
}

bool Node::has_component(const std::type_index index)
{
	return components.count(index) > 0;
}

}        // namespace sg
}        // namespace vkb
