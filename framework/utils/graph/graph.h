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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

#include <json.hpp>

#include "common/error.h"
#include "node.h"

namespace vkb
{
namespace utils
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
	Graph(const char *name);

	/**
	 * @brief Create a node object
	 * @tparam T
	 * @tparam Args 
	 * @param node required
	 * @param args 
	 * @return size_t id of the node in the graph
	 */
	template <class NodeType, typename T, typename... Args>
	size_t create_node(const T &node, Args... args)
	{
		const void *addr = reinterpret_cast<const void *>(&node);
		const void *uid  = get_uid(addr);
		if (!uid)
		{
			size_t id = new_id();
			uids[uid] = id;
			nodes[id] = std::make_unique<NodeType>(id, node, args...);
			return id;
		}
		return reinterpret_cast<size_t>(addr);
	}

	size_t create_vk_image(const VkImage &image);

	size_t create_vk_image_view(const VkImageView &image);

	template <typename T>
	size_t create_vk_node(const char *name, const T &handle)
	{
		size_t id = new_id();
		nodes[id] = std::make_unique<Node>(id, name, "Vulkan", nlohmann::json{{name, Node::handle_to_uintptr_t(handle)}});
		return id;
	}

	/**
	 * @brief Get the uid of a node
	 * 
	 * @param addr 
	 * @return const void* if null node doesnt exist
	 */
	const void *get_uid(const void *addr);

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
	bool dump_to_file(std::string file_name);

	size_t new_id();

  private:
	size_t                                            next_id = 0;
	std::vector<Edge>                                 adj;
	std::unordered_map<size_t, std::unique_ptr<Node>> nodes;
	std::unordered_map<const void *, size_t>          uids;
	std::string                                       name;
};
}        // namespace utils
}        // namespace vkb