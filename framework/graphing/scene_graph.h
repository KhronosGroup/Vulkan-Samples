/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "graphing/graph.h"
#include "graphing/graph_node.h"
#include "scene_graph/component.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/sub_mesh.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/components/transform.h"
#include "scene_graph/node.h"
#include "scene_graph/scene.h"

namespace vkb
{
namespace graphing
{
namespace scene_graph
{
void scrape_scene_node(Graph &graph, const std::vector<sg::Node *> &children, size_t owner);

bool generate(sg::Scene &scene);

size_t sg_scene_node(Graph &graph, const sg::Scene &scene);
size_t sg_node_node(Graph &graph, const sg::Node &node);
size_t sg_component_node(Graph &graph, const sg::Component &component);
size_t sg_transform_node(Graph &graph, const sg::Transform &transform);
size_t sg_mesh_node(Graph &graph, const sg::Mesh &mesh);
size_t sg_submesh_node(Graph &graph, const sg::SubMesh &submesh);
size_t sg_texture_node(Graph &graph, const sg::Texture &texture);
size_t sg_material_node(Graph &graph, const sg::Material &mat);

}        // namespace scene_graph
}        // namespace graphing
}        // namespace vkb
