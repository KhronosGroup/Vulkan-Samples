/* Copyright (c) 2018-2026, Arm Limited and Contributors
 * Copyright (c) 2026, NVIDIA CORPORATION. All rights reserved.
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
#include "scene_graph/components/camera.h"
#include "scene_graph/components/hpp_mesh.h"
#include "scene_graph/components/light.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/sub_mesh.h"
#include "scene_graph/node.h"
#include "scene_graph/scripts/animation.h"

namespace vkb
{
namespace scene_graph
{

/// @brief A collection of nodes organized in a tree structure.
template <vkb::BindingType bindingType>
class Scene
{
  public:
	Scene() = default;
	Scene(std::string const &name);

  public:
	void add_child(vkb::scene_graph::Node<bindingType> &child);
	void add_component(std::unique_ptr<vkb::sg::Component> &&component);
	void add_component(std::unique_ptr<vkb::sg::Component> &&component, vkb::scene_graph::Node<bindingType> &node);
	void add_node(std::unique_ptr<vkb::scene_graph::Node<bindingType>> &&node);
	template <class T>
	void                                 clear_components();
	vkb::scene_graph::Node<bindingType> *find_node(std::string const &name);
	template <class T>
	std::vector<T *>                     get_components() const;
	std::string const                   &get_name() const;
	vkb::scene_graph::Node<bindingType> &get_root_node();
	template <class T>
	bool has_component() const;
	template <class T>
	void set_components(std::vector<std::unique_ptr<T>> &&components);
	void set_name(std::string const &name);
	void set_nodes(std::vector<std::unique_ptr<vkb::scene_graph::Node<bindingType>>> &&nodes);
	void set_root_node(vkb::scene_graph::Node<bindingType> &node);

  private:
	template <typename T>
	std::type_index get_type_index() const;
	bool            has_component(std::type_index const &type_info) const;
	std::type_index map_type_index(std::type_index ti) const;

  private:
	std::unordered_map<std::type_index, std::vector<std::unique_ptr<vkb::sg::Component>>> components;
	std::string                                                                           name;
	std::vector<std::unique_ptr<vkb::scene_graph::NodeCpp>>                               nodes;        // List of all the nodes
	vkb::scene_graph::NodeCpp                                                            *root = nullptr;
};

using SceneC   = Scene<vkb::BindingType::C>;
using SceneCpp = Scene<vkb::BindingType::Cpp>;

// Member function definitions

template <vkb::BindingType bindingType>
inline Scene<bindingType>::Scene(std::string const &name) : name{name}
{}

template <vkb::BindingType bindingType>
inline void Scene<bindingType>::add_child(vkb::scene_graph::Node<bindingType> &child)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		root->add_child(child);
	}
	else
	{
		root->add_child(reinterpret_cast<vkb::scene_graph::NodeCpp &>(child));
	}
}

template <vkb::BindingType bindingType>
inline void Scene<bindingType>::add_component(std::unique_ptr<vkb::sg::Component> &&component)
{
	if (component)
	{
		components[map_type_index(component->get_type())].push_back(std::move(component));
	}
}

template <vkb::BindingType bindingType>
inline void Scene<bindingType>::add_component(std::unique_ptr<vkb::sg::Component> &&component, vkb::scene_graph::Node<bindingType> &node)
{
	node.set_component(*component);
	add_component(std::move(component));
}

template <vkb::BindingType bindingType>
inline void Scene<bindingType>::add_node(std::unique_ptr<vkb::scene_graph::Node<bindingType>> &&n)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		nodes.emplace_back(std::move(n));
	}
	else
	{
		nodes.push_back(std::unique_ptr<vkb::scene_graph::NodeCpp>(reinterpret_cast<vkb::scene_graph::NodeCpp *>(n.release())));
	}
}

template <vkb::BindingType bindingType>
template <class T>
inline void Scene<bindingType>::clear_components()
{
	components[get_type_index<T>()].clear();
}

template <vkb::BindingType bindingType>
inline vkb::scene_graph::Node<bindingType> *Scene<bindingType>::find_node(std::string const &node_name)
{
	assert(root);

	for (auto root_node : root->get_children())
	{
		std::queue<vkb::scene_graph::NodeCpp *> traverse_nodes;
		traverse_nodes.push(root_node);

		while (!traverse_nodes.empty())
		{
			auto node = traverse_nodes.front();
			traverse_nodes.pop();

			if (node->get_name() == node_name)
			{
				if constexpr (bindingType == vkb::BindingType::Cpp)
				{
					return node;
				}
				else
				{
					return reinterpret_cast<vkb::scene_graph::NodeC *>(node);
				}
			}

			for (auto child_node : node->get_children())
			{
				traverse_nodes.push(child_node);
			}
		}
	}

	return nullptr;
}

template <vkb::BindingType bindingType>
template <class T>
inline std::vector<T *> Scene<bindingType>::get_components() const
{
	std::vector<T *> result;
	if (has_component(get_type_index<T>()))
	{
		auto &scene_components = components.at(get_type_index<T>());

		result.resize(scene_components.size());
		std::transform(scene_components.begin(), scene_components.end(), result.begin(), [](std::unique_ptr<vkb::sg::Component> const &component) -> T * {
			return reinterpret_cast<T *>(component.get());
		});
	}

	return result;
}

template <vkb::BindingType bindingType>
inline std::string const &Scene<bindingType>::get_name() const
{
	return name;
}

template <vkb::BindingType bindingType>
inline vkb::scene_graph::Node<bindingType> &Scene<bindingType>::get_root_node()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return *root;
	}
	else
	{
		return reinterpret_cast<vkb::scene_graph::NodeC &>(*root);
	}
}

template <vkb::BindingType bindingType>
template <class T>
inline bool Scene<bindingType>::has_component() const
{
	return has_component(get_type_index<T>());
}

template <vkb::BindingType bindingType>
template <typename T>
inline std::type_index Scene<bindingType>::get_type_index() const
{
	// please add a type check here for types never encountered before
	static_assert(std::is_same<T, vkb::scene_graph::components::HPPMesh>::value || std::is_same<T, vkb::scene_graph::components::SamplerC>::value ||
	              std::is_same<T, vkb::sg::Animation>::value || std::is_same<T, vkb::sg::Camera>::value || std::is_same<T, vkb::sg::Image>::value ||
	              std::is_same<T, vkb::sg::Light>::value || std::is_same<T, vkb::sg::Mesh>::value || std::is_same<T, vkb::sg::PBRMaterial>::value ||
	              std::is_same<T, vkb::sg::Script>::value || std::is_same<T, vkb::sg::SubMesh>::value || std::is_same<T, vkb::sg::Texture>::value);

	return map_type_index(typeid(T));
}

template <vkb::BindingType bindingType>
inline bool Scene<bindingType>::has_component(std::type_index const &type_info) const
{
	auto component = components.find(type_info);
	return (component != components.end() && !component->second.empty());
}

// map type_index from C-type to C++-type
template <vkb::BindingType bindingType>
inline std::type_index Scene<bindingType>::map_type_index(std::type_index ti) const
{
	if (ti == typeid(vkb::scene_graph::components::SamplerC))
	{
		return typeid(vkb::scene_graph::components::SamplerCpp);
	}
	else if (ti == typeid(vkb::sg::Mesh))
	{
		return typeid(vkb::scene_graph::components::HPPMesh);
	}
	else
	{
		return ti;
	}
}

template <vkb::BindingType bindingType>
template <class T>
inline void Scene<bindingType>::set_components(std::vector<std::unique_ptr<T>> &&new_components)
{
	std::vector<std::unique_ptr<vkb::sg::Component>> result(new_components.size());
	std::ranges::transform(new_components, result.begin(), [](std::unique_ptr<T> &component) -> std::unique_ptr<vkb::sg::Component> {
		return std::unique_ptr<vkb::sg::Component>(std::move(component));
	});
	components[get_type_index<T>()] = std::move(result);
}

template <vkb::BindingType bindingType>
inline void Scene<bindingType>::set_name(std::string const &new_name)
{
	name = new_name;
}

template <vkb::BindingType bindingType>
inline void Scene<bindingType>::set_nodes(std::vector<std::unique_ptr<vkb::scene_graph::Node<bindingType>>> &&n)
{
	assert(nodes.empty() && "Scene nodes were already set");
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		nodes = std::move(n);
	}
	else
	{
		nodes.reserve(n.size());
		for (auto &node : n)
		{
			nodes.push_back(std::unique_ptr<vkb::scene_graph::NodeCpp>(reinterpret_cast<vkb::scene_graph::NodeCpp *>(node.release())));
		}
	}
}

template <vkb::BindingType bindingType>
inline void Scene<bindingType>::set_root_node(vkb::scene_graph::Node<bindingType> &node)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		root = &node;
	}
	else
	{
		root = reinterpret_cast<vkb::scene_graph::NodeCpp *>(&node);
	}
}

}        // namespace scene_graph
}        // namespace vkb