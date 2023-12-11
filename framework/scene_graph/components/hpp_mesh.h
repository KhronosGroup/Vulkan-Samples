/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "mesh.h"

namespace vkb
{
namespace scene_graph
{
class HPPNode;

namespace components
{
class HPPSubMesh;

/**
 * @brief facade class around vkb::sg::Mesh, providing a vulkan.hpp-based interface
 *
 * See vkb::sg::Mesh for documentation
 */
class HPPMesh : private vkb::sg::Mesh
{
  public:
	using ComponentType = vkb::sg::Mesh;

	HPPMesh(const std::string &name) :
	    vkb::sg::Mesh(name)
	{}

	void add_node(vkb::scene_graph::HPPNode &node)
	{
		vkb::sg::Mesh::add_node(reinterpret_cast<vkb::sg::Node &>(node));
	}

	void add_submesh(vkb::scene_graph::components::HPPSubMesh &submesh)
	{
		vkb::sg::Mesh::add_submesh(reinterpret_cast<vkb::sg::SubMesh &>(submesh));
	}
};
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
