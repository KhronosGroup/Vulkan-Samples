/* Copyright (c) 2018-2022, Arm Limited and Contributors
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

#include <components/vfs/filesystem.hpp>

namespace vkb
{
namespace graphing
{
Graph::Graph(const char *new_name) :
    name(new_name)
{
}

void Graph::new_style(std::string style_name, std::string color)
{
	auto it = style_colors.find(style_name);
	if (it != style_colors.end())
	{
		it->second = color;
	}
	else
	{
		style_colors.insert({style_name, color});
	}
}

size_t Graph::new_id()
{
	return next_id++;
}

size_t Graph::find_ref(std::string &name)
{
	auto it = refs.find(name);
	if (it == refs.end())
	{
		return node_not_found;
	}
	return it->second;
}

void Graph::add_ref(std::string &name, size_t id)
{
	refs.insert({name, id});
}

void Graph::remove_ref(std::string &name)
{
	auto it = refs.find(name);
	if (it != refs.end())
	{
		refs.erase(it);
	}
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

bool Graph::dump_to_file(std::string file)
{
	std::vector<nlohmann::json> edges;
	for (auto &e : adj)
	{
		auto it = nodes.find(e.from);
		if (it != nodes.end())
		{
			e.options["style"] = it->second->attributes["style"];
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
	    {"edges", edges},
	    {"styles", style_colors}};

	std::stringstream json;

	try
	{
		// Whitespace needed as last character is overwritten on android causing the json to be corrupt
		json << j << " ";
	}
	catch (std::exception &e)
	{
		// JSON dump errors
		LOGE(e.what());
		return false;
	}

	if (!nlohmann::json::accept(json.str()))
	{
		LOGE("Invalid JSON string");
		return false;
	}

	std::string str = json.str();

	std::vector<uint8_t> data{str.begin(), str.end()};

	return vfs::instance().write_file(file, data.data(), data.size()) != vfs::status::Success;
}        // namespace graphing

}        // namespace graphing
}        // namespace vkb