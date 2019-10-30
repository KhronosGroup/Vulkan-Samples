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

#include "mesh.h"

namespace vkb
{
namespace sg
{
Mesh::Mesh(const std::string &name) :
    Component{name}
{}

void Mesh::update_bounds(const std::vector<glm::vec3> &vertex_data, const std::vector<uint16_t> &index_data)
{
	bounds.update(vertex_data, index_data);
}

std::type_index Mesh::get_type()
{
	return typeid(Mesh);
}

const AABB &Mesh::get_bounds() const
{
	return bounds;
}

void Mesh::add_submesh(SubMesh &submesh)
{
	submeshes.push_back(&submesh);
}

const std::vector<SubMesh *> &Mesh::get_submeshes() const
{
	return submeshes;
}

void Mesh::add_node(Node &node)
{
	nodes.push_back(&node);
}

const std::vector<Node *> &Mesh::get_nodes() const
{
	return nodes;
}
}        // namespace sg
}        // namespace vkb
