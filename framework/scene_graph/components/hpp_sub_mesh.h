/* Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
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

#include <core/hpp_buffer.h>
#include <scene_graph/components/sub_mesh.h>

namespace vkb
{
namespace scene_graph
{
namespace components
{
/**
 * @brief facade class around vkb::sg::SubMesh, providing a vulkan.hpp-based interface
 *
 * See vkb::sb::SubMeshfor for documentation
 */
class HPPSubMesh : private vkb::sg::SubMesh
{
  public:
	using vkb::sg::SubMesh::vertex_indices;

	vkb::core::HPPBuffer const &get_index_buffer() const
	{
		return reinterpret_cast<vkb::core::HPPBuffer const &>(*vkb::sg::SubMesh::index_buffer);
	}

	vk::IndexType get_index_type() const
	{
		return static_cast<vk::IndexType>(vkb::sg::SubMesh::index_type);
	}

	vkb::core::HPPBuffer const &get_vertex_buffer(std::string const &name) const
	{
		return reinterpret_cast<vkb::core::HPPBuffer const &>(vkb::sg::SubMesh::vertex_buffers.at(name));
	}
};
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
