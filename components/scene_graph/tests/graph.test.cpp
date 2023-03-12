/* Copyright (c) 2022, Arm Limited and Contributors
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

#include <catch2/catch_test_macros.hpp>

#include <components/scene_graph/graph.hpp>

using namespace components;

TEST_CASE("sg::Node::create", "[scene_graph]")
{
	auto registry  = sg::create_registry();
	auto root_node = sg::Node::create(registry, "root");
	REQUIRE(root_node->parent() == nullptr);
	REQUIRE(root_node->children().size() == 0);

	// default components set by create
	REQUIRE(root_node->has_components<sg::Transform, sg::SceneNode>());
}

TEST_CASE("multiple children test", "[scene_graph]")
{
	auto registry  = sg::create_registry();
	auto root_node = sg::Node::create(registry, "root");

	std::vector<sg::NodePtr> children;

	for (size_t i = 0; i < 10; i++)
	{
		auto child = sg::Node::create(registry, "child");
		root_node->add_child(child);
		children.push_back(child);
	}

	REQUIRE(root_node->children().size() == 10);

	// adding the same children again should have no new effects
	for (auto &child : children)
	{
		root_node->add_child(child);
	}

	REQUIRE(root_node->children().size() == 10);

	for (auto &child : children)
	{
		REQUIRE(child->parent() == root_node.get());
		root_node->remove_child(child);
		REQUIRE(child->parent() == nullptr);
	}

	REQUIRE(root_node->children().size() == 0);
}

TEST_CASE("multiple node destructor test", "[scene_graph]")
{
	auto registry  = sg::create_registry();
	auto root_node = sg::Node::create(registry, "root");

	std::vector<sg::NodePtr> children;

	for (size_t i = 0; i < 10; i++)
	{
		auto child = sg::Node::create(registry, "child");
		root_node->add_child(child);
		children.push_back(child);
	}

	REQUIRE(root_node->children().size() == 10);
}

TEST_CASE("node add component test", "[scene_graph]")
{
	auto registry  = sg::create_registry();
	auto root_node = sg::Node::create(registry, "root");

	struct Component
	{
		uint32_t value;
	};

	REQUIRE(!root_node->has_components<Component>());

	root_node->set_component<Component>(12U);
	REQUIRE(root_node->has_components<Component>());

	auto &component = root_node->get_component<Component>();
	REQUIRE(component.value == 12U);
}

TEST_CASE("component view across multiple nodes", "[scene_graph]")
{
	auto registry = sg::create_registry();

	const size_t node_count = 20;

	std::vector<sg::NodePtr> nodes;
	nodes.reserve(node_count);

	struct Component
	{
		uint32_t value;
	};

	for (size_t i = 0; i < node_count; i++)
	{
		auto node = sg::Node::create(registry, "node");
		node->set_component<Component>(static_cast<uint32_t>(i));
	}

	auto view = registry->view<Component>();

	uint32_t value = 0;
	for (auto entity : view)
	{
		auto &component = view.get<Component>(entity);
		REQUIRE(component.value == value);
		value++;
	}
}