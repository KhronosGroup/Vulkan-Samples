/* Copyright (c) 2018-2025, Arm Limited and Contributors
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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

#include "scene_graph/component.h"
#include "scene_graph/components/transform.h"

namespace vkb
{
namespace scene_graph
{
/// @brief A leaf of the tree structure which can have children and a single parent.
template <vkb::BindingType bindingType>
class Node
{
  public:
	Node(size_t id, std::string const &name);

	virtual ~Node() = default;

	void                                    add_child(Node<bindingType> &child);
	std::vector<Node<bindingType> *> const &get_children() const;
	template <class T>
	T                  &get_component();
	vkb::sg::Component &get_component(std::type_index index);
	size_t              get_id() const;
	std::string const  &get_name() const;
	Node<bindingType>  *get_parent();
	vkb::sg::Transform &get_transform();
	template <class T>
	bool has_component() const;
	bool has_component(std::type_index index) const;
	void set_component(vkb::sg::Component &component);
	void set_parent(Node<bindingType> &parent);

  private:
	size_t                                                    id;
	std::vector<Node<vkb::BindingType::Cpp> *>                children;
	std::unordered_map<std::type_index, vkb::sg::Component *> components;
	std::string                                               name;
	Node<vkb::BindingType::Cpp>                              *parent = nullptr;
	vkb::sg::Transform                                        transform;
};

using NodeC   = Node<vkb::BindingType::C>;
using NodeCpp = Node<vkb::BindingType::Cpp>;

// Member function definitions

template <vkb::BindingType bindingType>
inline Node<bindingType>::Node(size_t id_, std::string const &name_) :
    id{id_}, name{name_}, transform{*this}
{
	set_component(transform);
}

template <vkb::BindingType bindingType>
inline void Node<bindingType>::add_child(Node<bindingType> &child)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		children.push_back(&child);
	}
	else
	{
		children.push_back(reinterpret_cast<vkb::scene_graph::NodeCpp *>(&child));
	}
}

template <vkb::BindingType bindingType>
std::vector<Node<bindingType> *> const &Node<bindingType>::get_children() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return children;
	}
	else
	{
		return reinterpret_cast<std::vector<NodeC *> const &>(children);
	}
}

template <vkb::BindingType bindingType>
template <class T>
inline T &Node<bindingType>::get_component()
{
	return dynamic_cast<T &>(get_component(typeid(T)));
}

template <vkb::BindingType bindingType>
inline vkb::sg::Component &Node<bindingType>::get_component(std::type_index index)
{
	return *components.at(index);
}

template <vkb::BindingType bindingType>
inline size_t Node<bindingType>::get_id() const
{
	return id;
}

template <vkb::BindingType bindingType>
inline std::string const &Node<bindingType>::get_name() const
{
	return name;
}

template <vkb::BindingType bindingType>
inline Node<bindingType> *Node<bindingType>::get_parent()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return parent;
	}
	else
	{
		return reinterpret_cast<NodeC *>(parent);
	}
}

template <vkb::BindingType bindingType>
inline vkb::sg::Transform &Node<bindingType>::get_transform()
{
	return transform;
}

template <vkb::BindingType bindingType>
template <class T>
inline bool Node<bindingType>::has_component() const
{
	return has_component(typeid(T));
}

template <vkb::BindingType bindingType>
inline bool Node<bindingType>::has_component(std::type_index index) const
{
	return components.contains(index);
}

template <vkb::BindingType bindingType>
inline void Node<bindingType>::set_component(vkb::sg::Component &component)
{
	components[component.get_type()] = &component;
}

template <vkb::BindingType bindingType>
inline void Node<bindingType>::set_parent(Node<bindingType> &p)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		parent = &p;
	}
	else
	{
		parent = reinterpret_cast<NodeCpp *>(&p);
	}

	transform.invalidate_world_matrix();
}
}        // namespace scene_graph
}        // namespace vkb
