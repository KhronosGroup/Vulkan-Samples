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

#include "graph.h"

#include "platform/filesystem.h"

namespace vkb
{
namespace utils
{
Graph::Graph(const char *new_name) :
    name(new_name)
{
}

size_t Graph::new_id()
{
	return next_id++;
}

void Graph::add_edge(size_t from, size_t to)
{
	auto it = std::find_if(adj.begin(), adj.end(), [from, to](auto &e) -> bool { return e.from == from && e.to == to; });
	if (it == adj.end())
	{
		adj.push_back({new_id(), from, to});
	}
}

void Graph::remove_edge(size_t from, size_t to)
{
	auto it = std::find_if(adj.begin(), adj.end(), [from, to](auto &e) -> bool { return e.from == from && e.to == to; });
	if (it != adj.end())
	{
		adj.erase(it);
	}
}

size_t Graph::create_vk_image(const VkImage &image)
{
	const void *addr = reinterpret_cast<const void *>(image);
	const void *uid  = get_uid(addr);
	if (!uid)
	{
		auto id    = create_vk_node("VkImage", image);
		uids[addr] = id;
		return id;
	}
	return reinterpret_cast<size_t>(uid);
}

size_t Graph::create_vk_image_view(const VkImageView &image)
{
	const void *addr = reinterpret_cast<const void *>(image);
	const void *uid  = get_uid(addr);
	if (!uid)
	{
		auto id    = create_vk_node("VkImageView", image);
		uids[addr] = id;
		return id;
	}
	return reinterpret_cast<size_t>(uid);
}

const void *Graph::get_uid(const void *addr)
{
	auto it = uids.find(addr);
	if (it != uids.end())
	{
		return reinterpret_cast<const void *>(it->second);
	}
	return nullptr;
}

bool Graph::dump_to_file(std::string file)
{
	std::vector<nlohmann::json> edges;
	for (auto &e : adj)
	{
		auto it = nodes.find(e.from);
		if (it != nodes.end())
		{
			e.options["group"] = it->second->attributes["group"];
		}
		e.options["id"]     = e.id;
		e.options["source"] = e.from;
		e.options["target"] = e.to;
		edges.push_back({{"data", e.options}});
	}

	std::vector<nlohmann::json> node_json;
	auto                        it = nodes.begin();
	while (it != nodes.end())
	{
		node_json.push_back(it->second->attributes);
		it++;
	}

	nlohmann::json j = {
	    {"name", name},
	    {"nodes", node_json},
	    {"edges", edges}};

	return fs::write_json(j, file);
}

}        // namespace utils
}        // namespace vkb