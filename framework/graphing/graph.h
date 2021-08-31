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

#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

#include <json.hpp>

#include "common/error.h"
#include "graph_node.h"

namespace vkb
{
namespace graphing
{
struct Edge
{
	size_t         id;
	size_t         from;
	size_t         to;
	nlohmann::json options;
	Edge(size_t id_, size_t s, size_t t) :
	    id(id_),
	    from(s),
	    to(t){};
};

/**
 * @brief Graph is an implementation of an adjacency list graph. The nodes are created from a variadic function and their implementation is defined by the given NodeType
 * 
 * @tparam NodeType either FrameworkNode or SceneNode
 */
class Graph
{
  public:
	explicit Graph(const char *name);

	static const size_t node_not_found = 0;

	/**
	 * @brief Create a new style
	 * 
	 * @param style_name the group name
	 * @param color the hex color of the group
	 */
	void new_style(const std::string& style_name, const std::string& color);

	/**
	 * @brief Create a node object
	 * 
	 * @param title of node
	 * @param style corresponds to the key used when using new_type(style, color)
	 * @param data json data to be displayed with node
	 * @return size_t id of node
	 */
	size_t create_node(const char *title = "Node", const char *style = nullptr, const nlohmann::json &data = {})
	{
		size_t id = new_id();
		nodes[id] = std::make_unique<Node>(id, title, style, data);
		return id;
	}

	/**
	 * @brief Find a node from a reference
	 * 		  If the node does not exist then the reference will be node_not_found
	 * 
	 * @param _name of node
	 * @return size_t if of node
	 */
	size_t find_ref(std::string &_name);

	/**
	 * @brief Add a readable reference to a node
	 * 
	 * @param name of reference
	 * @param id of node
	 */
	void add_ref(std::string &name, size_t id);

	/**
	 * @brief Remove a readable reference to a node
	 * 
	 * @param name of ref
	 */
	void remove_ref(std::string &name);

	/**
	 * @brief Add an edge to the graph
	 * @param from source node
	 * @param to target node
	 */
	void add_edge(size_t from, size_t to);

	/**
	 * @brief Remove Edge from the graph
	 * @param from source node
	 * @param to target node
	 */
	void remove_edge(size_t from, size_t to);

	/**
	 * @brief Dump the graphs state to json in the given file name
	 * @param file_name to dump to
	 */
	bool dump_to_file(const std::string& file_name);

	size_t new_id();

  private:
	size_t                                            next_id = 1;
	std::vector<Edge>                                 adj;
	std::unordered_map<size_t, std::unique_ptr<Node>> nodes;
	std::unordered_map<std::string, size_t>           refs;
	std::string                                       name;
	std::unordered_map<std::string, std::string>      style_colors;
};
}        // namespace graphing
}        // namespace vkb