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

#include "scene.h"

#include <queue>
#include <utility>

#include "common/error.h"
#include "component.h"
#include "components/sub_mesh.h"
#include "node.h"

namespace vkb
{
namespace sg
{
Scene::Scene(std::string name) :
    name{std::move(name)}
{}

void Scene::set_name(const std::string &new_name)
{
	name = new_name;
}

const std::string &Scene::get_name() const
{
	return name;
}

void Scene::set_nodes(std::vector<std::unique_ptr<Node>> &&n)
{
	assert(nodes.empty() && "Scene nodes were already set");
	nodes = std::move(n);
}

void Scene::add_node(std::unique_ptr<Node> &&n)
{
	nodes.emplace_back(std::move(n));
}

void Scene::add_child(Node &child)
{
	root->add_child(child);
}

std::unique_ptr<Component> Scene::get_model(uint32_t index)
{
	auto meshes = std::move(components.at(typeid(SubMesh)));

	return std::move(meshes.at(index));
}

void Scene::add_component(std::unique_ptr<Component> &&component, Node &node)
{
	node.set_component(*component);

	if (component)
	{
		components[component->get_type()].push_back(std::move(component));
	}
}

void Scene::add_component(std::unique_ptr<Component> &&component)
{
	if (component)
	{
		components[component->get_type()].push_back(std::move(component));
	}
}

void Scene::set_components(const std::type_index &type_info, std::vector<std::unique_ptr<Component>> &&new_components)
{
	components[type_info] = std::move(new_components);
}

const std::vector<std::unique_ptr<Component>> &Scene::get_components(const std::type_index &type_info) const
{
	return components.at(type_info);
}

bool Scene::has_component(const std::type_index &type_info) const
{
	auto component = components.find(type_info);
	return (component != components.end() && !component->second.empty());
}

Node *Scene::find_node(const std::string &node_name)
{
	for (auto root_node : root->get_children())
	{
		std::queue<sg::Node *> traverse_nodes{};
		traverse_nodes.push(root_node);

		while (!traverse_nodes.empty())
		{
			auto node = traverse_nodes.front();
			traverse_nodes.pop();

			if (node->get_name() == node_name)
			{
				return node;
			}

			for (auto child_node : node->get_children())
			{
				traverse_nodes.push(child_node);
			}
		}
	}

	return nullptr;
}

void Scene::set_root_node(Node &node)
{
	root = &node;
}

Node &Scene::get_root_node()
{
	return *root;
}
}        // namespace sg
}        // namespace vkb