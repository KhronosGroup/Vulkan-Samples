/* Copyright (c) 2018-2025, Arm Limited and Contributors
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

#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "scene_graph/component.h"
#include "scene_graph/components/aabb.h"
#include "scene_graph/node.h"

namespace vkb
{
namespace sg
{
class SubMesh;

class Mesh : public Component
{
  public:
	Mesh(const std::string &name);

	virtual ~Mesh() = default;

	void update_bounds(const std::vector<glm::vec3> &vertex_data, const std::vector<uint16_t> &index_data = {});

	virtual std::type_index get_type() override;

	const AABB &get_bounds() const;

	void add_submesh(SubMesh &submesh);

	const std::vector<SubMesh *> &get_submeshes() const;

	void add_node(vkb::scene_graph::NodeC &node);

	const std::vector<vkb::scene_graph::NodeC *> &get_nodes() const;

  private:
	AABB bounds;

	std::vector<SubMesh *> submeshes;

	std::vector<vkb::scene_graph::NodeC *> nodes;
};
}        // namespace sg
}        // namespace vkb
