/* Copyright (c) 2019-2020, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "graphing/scene_graph.h"

#include "glm/gtx/string_cast.hpp"

namespace vkb
{
namespace graphing
{
namespace scene_graph
{
template <typename T>
std::string label(const T &node, const std::string &type)
{
	return node.get_name().empty() ? type : type + ": " + node.get_name();
}

bool generate(sg::Scene &scene)
{
	Graph scene_graph("Scene");
	scene_graph.new_style("Scene", "#00BCD4");
	scene_graph.new_style("Component", "#FFC107");
	scene_graph.new_style("Node", "#F44336");

	size_t scene_id = sg_scene_node(scene_graph, scene);

	scrape_scene_node(scene_graph, scene.get_root_node().get_children(), scene_id);

	return scene_graph.dump_to_file("scene.json");
}

void scrape_scene_node(Graph &graph, const std::vector<sg::Node *> &children, size_t owner)
{
	for (const auto &child : children)
	{
		size_t child_id = sg_node_node(graph, *child);
		graph.add_edge(owner, child_id);

		if (child->has_component<sg::Transform>())
		{
			auto & i            = child->get_component<sg::Transform>();
			size_t component_id = sg_transform_node(graph, i);
			graph.add_edge(child_id, component_id);
		}
		if (child->has_component<sg::Mesh>())
		{
			auto & mesh    = child->get_component<sg::Mesh>();
			size_t mesh_id = sg_mesh_node(graph, mesh);
			graph.add_edge(child_id, mesh_id);

			for (const auto &sub_mesh : mesh.get_submeshes())
			{
				size_t sub_mesh_id = sg_submesh_node(graph, *sub_mesh);
				graph.add_edge(mesh_id, sub_mesh_id);

				const auto &material    = sub_mesh->get_material();
				size_t      material_id = sg_material_node(graph, *material);
				graph.add_edge(sub_mesh_id, material_id);

				auto it = material->textures.begin();
				while (it != material->textures.end())
				{
					size_t texture_id = sg_texture_node(graph, *it->second);
					graph.add_edge(material_id, texture_id);
					it++;
				}
			}
		}

		scrape_scene_node(graph, child->get_children(), child_id);
	}
}

size_t sg_scene_node(Graph &graph, const sg::Scene &scene)
{
	return graph.create_node("Scene", "Scene");
}
size_t sg_node_node(Graph &graph, const sg::Node &node)
{
	return graph.create_node(label(node, "Node").c_str(), "Node");
}
size_t sg_transform_node(Graph &graph, const sg::Transform &transform)
{
	nlohmann::json data = {
	    {"translation", {{"x", transform.get_translation().x}, {"y", transform.get_translation().y}, {"z", transform.get_translation().z}}},
	    {"rotation", {{"x", transform.get_rotation().x}, {"y", transform.get_rotation().y}, {"z", transform.get_rotation().z}, {"w", transform.get_rotation().w}}},
	    {"scale", {{"x", transform.get_scale().x}, {"y", transform.get_scale().y}, {"z", transform.get_scale().z}}},
	    {"matrix", glm::to_string(transform.get_matrix())}};

	return graph.create_node(label(transform, "Transform").c_str(), "Component", data);
}
size_t sg_mesh_node(Graph &graph, const sg::Mesh &mesh)
{
	return graph.create_node(label(mesh, "Mesh").c_str(), "Component");
}
size_t sg_submesh_node(Graph &graph, const sg::SubMesh &submesh)
{
	return graph.create_node(label(submesh, "SubMesh").c_str(), "Component");
}
size_t sg_texture_node(Graph &graph, const sg::Texture &texture)
{
	return graph.create_node(label(texture, "Texture").c_str(), "Component");
}
size_t sg_material_node(Graph &graph, const sg::Material &mat)
{
	nlohmann::json data = {
	    {"AlphaMode", to_string(mat.alpha_mode)},
	    {"emissive", glm::to_string(mat.emissive)},
	    {"double_sided", to_string(mat.double_sided)},
	    {"alpha_cutoff", mat.alpha_cutoff}};

	return graph.create_node(label(mat, "Material").c_str(), "Component", data);
}
}        // namespace scene_graph
}        // namespace graphing
}        // namespace vkb
