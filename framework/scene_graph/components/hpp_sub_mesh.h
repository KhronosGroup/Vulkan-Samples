/* Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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
class HPPPBRMaterial;

struct HPPVertexAttribute
{
	vk::Format    format = vk::Format::eUndefined;
	std::uint32_t stride = 0;
	std::uint32_t offset = 0;
};

/**
 * @brief facade class around vkb::sg::SubMesh, providing a vulkan.hpp-based interface
 *
 * See vkb::sb::SubMeshfor for documentation
 */
class HPPSubMesh : private vkb::sg::SubMesh
{
  public:
	using ComponentType = vkb::sg::SubMesh;

	using vkb::sg::SubMesh::vertex_indices;
	using vkb::sg::SubMesh::vertices_count;

	HPPSubMesh(std::string const &name = {}) :
	    vkb::sg::SubMesh(name)
	{}

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

	void set_attribute(const std::string &name, const vkb::scene_graph::components::HPPVertexAttribute &attribute)
	{
		vkb::sg::SubMesh::set_attribute(name, reinterpret_cast<vkb::sg::VertexAttribute const &>(attribute));
	}

	void set_index_buffer(std::unique_ptr<vkb::core::HPPBuffer> &&index_buffer)
	{
		vkb::sg::SubMesh::index_buffer = reinterpret_cast<std::unique_ptr<vkb::core::Buffer> &&>(index_buffer);
	}

	void set_index_type(vk::IndexType index_type)
	{
		vkb::sg::SubMesh::index_type = static_cast<VkIndexType>(index_type);
	}

	void set_material(vkb::scene_graph::components::HPPPBRMaterial const &material)
	{
		vkb::sg::SubMesh::set_material(reinterpret_cast<vkb::sg::Material const &>(material));
	}

	void set_vertex_buffer(std::string const &name, vkb::core::HPPBuffer &&buffer)
	{
		vkb::sg::SubMesh::vertex_buffers.insert(std::make_pair(name, std::move(reinterpret_cast<vkb::core::Buffer &&>(buffer))));
	}
};
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
