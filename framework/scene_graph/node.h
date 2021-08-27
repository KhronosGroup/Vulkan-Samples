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

#pragma once

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "scene_graph/components/transform.h"

namespace vkb
{
namespace sg
{
class Component;

/// @brief A leaf of the tree structure which can have children and a single parent.
class Node
{
  public:
	Node(size_t id, std::string name);

	virtual ~Node() = default;

	size_t get_id() const;

	const std::string &get_name() const;

	Transform &get_transform()
	{
		return transform;
	}

	void set_parent(Node &parent);

	Node *get_parent() const;

	void add_child(Node &child);

	const std::vector<Node *> &get_children() const;

	void set_component(Component &component);

	template <class T>
	inline T &get_component()
	{
		return dynamic_cast<T &>(get_component(typeid(T)));
	}

	Component &get_component(std::type_index index);

	template <class T>
	bool has_component()
	{
		return has_component(typeid(T));
	}

	bool has_component(std::type_index index);

  private:
	size_t id;

	std::string name;

	Transform transform;

	Node *parent{nullptr};

	std::vector<Node *> children;

	std::unordered_map<std::type_index, Component *> components;
};
}        // namespace sg
}        // namespace vkb
