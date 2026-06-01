/* Copyright (c) 2021-2026, NVIDIA CORPORATION. All rights reserved.
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

#include <core/hpp_shader_module.h>
#include <scene_graph/components/hpp_material.h>
#include <scene_graph/components/sub_mesh.h>

namespace vkb
{
namespace scene_graph
{
namespace components
{
struct HPPVertexAttribute
{
	vk::Format    format = vk::Format::eUndefined;
	std::uint32_t stride = 0;
	std::uint32_t offset = 0;
};

/**
 * @brief facade class around vkb::sg::SubMesh, providing a vulkan.hpp-based interface
 *
 * See vkb::sb::SubMesh for documentation
 */
class HPPSubMesh : private vkb::sg::SubMesh
{
  public:
	using vkb::sg::Component::get_name;
	using vkb::sg::SubMesh::get_index_offset;
	using vkb::sg::SubMesh::get_vertex_indices;
	using vkb::sg::SubMesh::get_vertices_count;

	bool get_attribute(const std::string &name, HPPVertexAttribute &attribute) const
	{
		return vkb::sg::SubMesh::get_attribute(name, reinterpret_cast<vkb::sg::VertexAttribute &>(attribute));
	}

	vkb::core::BufferCpp const &get_index_buffer() const
	{
		return reinterpret_cast<vkb::core::BufferCpp const &>(vkb::sg::SubMesh::get_index_buffer());
	}

	vk::IndexType get_index_type() const
	{
		return static_cast<vk::IndexType>(vkb::sg::SubMesh::get_index_type());
	}

	const HPPMaterial *get_material() const
	{
		return reinterpret_cast<vkb::scene_graph::components::HPPMaterial const *>(vkb::sg::SubMesh::get_material());
	}

	vkb::core::HPPShaderVariant &get_mut_shader_variant()
	{
		return reinterpret_cast<vkb::core::HPPShaderVariant &>(vkb::sg::SubMesh::get_mut_shader_variant());
	}

	const vkb::core::HPPShaderVariant &get_shader_variant() const
	{
		return reinterpret_cast<vkb::core::HPPShaderVariant const &>(vkb::sg::SubMesh::get_shader_variant());
	}

	vkb::core::BufferCpp const &get_vertex_buffer(std::string const &name) const
	{
		return reinterpret_cast<vkb::core::BufferCpp const &>(vkb::sg::SubMesh::vertex_buffers.at(name));
	}

	vkb::core::BufferCpp const *find_vertex_buffer(std::string const &name) const
	{
		auto it = vkb::sg::SubMesh::vertex_buffers.find(name);
		return it != vkb::sg::SubMesh::vertex_buffers.end() ? reinterpret_cast<vkb::core::BufferCpp const *>(&it->second) : nullptr;
	}
};
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
