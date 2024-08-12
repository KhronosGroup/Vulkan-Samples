/* Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_sub_mesh.h"
#include "mesh.h"

namespace vkb
{
namespace scene_graph
{
namespace components
{
/**
 * @brief facade class around vkb::sg::Mesh, providing a vulkan.hpp-based interface
 *
 * See vkb::sb::Mesh for documentation
 */
class HPPMesh : private vkb::sg::Mesh
{
  public:
	std::vector<vkb::scene_graph::components::HPPSubMesh *> const &get_submeshes() const
	{
		return reinterpret_cast<std::vector<vkb::scene_graph::components::HPPSubMesh *> const &>(vkb::sg::Mesh::get_submeshes());
	}
};
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
