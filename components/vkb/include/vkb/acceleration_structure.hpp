/* Copyright (c) 2021, Sascha Willems
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

#include <map>

#include "context.hpp"

namespace vkb
{
namespace core
{
/**
 * @brief Wraps setup and access for a ray tracing top- or bottom-level acceleration structure
 */
class AccelerationStructure
{
  public:
	/**
	 * @brief Creates a acceleration structure and the required buffer to store it's geometries
	 * @param device A valid Vulkan device
	 * @param type The type of the acceleration structure (top- or bottom-level)
	 */
	AccelerationStructure(ContextPtr context, vk::AccelerationStructureTypeKHR type);
	~AccelerationStructure();

	/**
	 * @brief Adds triangle geometry to the acceleration structure (only valid for bottom level)
	 * @returns UUID for the geometry instance for the case of multiple geometries to look up in the map
	 * @param vertex_buffer Buffer containing vertices
	 * @param index_buffer Buffer containing indices
	 * @param transform_buffer Buffer containing transform data
	 * @param triangle_count Number of triangles for this geometry
	 * @param max_vertex Index of the last vertex in the geometry
	 * @param vertex_stride Stride of the vertex structure
	 * @param transform_offset Offset of this geometry in the transform data buffer
	 * @param vertex_format Format of the vertex structure
	 * @param flags Ray tracing geometry flags
	 * @param vertex_buffer_data_address set this if don't want the vertex_buffer data_address
	 * @param index_buffer_data_address set this if don't want the index_buffer data_address
	 * @param transform_buffer_data_address set this if don't want the transform_buffer data_address
	 */
	uint64_t add_triangle_geometry(BufferHandle        &vertex_buffer,
	                               BufferHandle        &index_buffer,
	                               BufferHandle        &transform_buffer,
	                               uint32_t             triangle_count,
	                               uint32_t             max_vertex,
	                               vk::DeviceSize       vertex_stride,
	                               uint32_t             transform_offset              = 0,
	                               vk::Format           vertex_format                 = vk::Format::eR32G32B32Sfloat,
	                               vk::GeometryFlagsKHR flags                         = vk::GeometryFlagBitsKHR::eOpaque,
	                               uint64_t             vertex_buffer_data_address    = 0,
	                               uint64_t             index_buffer_data_address     = 0,
	                               uint64_t             transform_buffer_data_address = 0);

	void update_triangle_geometry(uint64_t             triangleUUID,
	                              BufferHandle        &vertex_buffer,
	                              BufferHandle        &index_buffer,
	                              BufferHandle        &transform_buffer,
	                              uint32_t             triangle_count,
	                              uint32_t             max_vertex,
	                              vk::DeviceSize       vertex_stride,
	                              uint32_t             transform_offset              = 0,
	                              vk::Format           vertex_format                 = vk::Format::eR32G32B32Sfloat,
	                              vk::GeometryFlagsKHR flags                         = vk::GeometryFlagBitsKHR::eOpaque,
	                              uint64_t             vertex_buffer_data_address    = 0,
	                              uint64_t             index_buffer_data_address     = 0,
	                              uint64_t             transform_buffer_data_address = 0);

	/**
	 * @brief Adds instance geometry to the acceleration structure (only valid for top level)
	 * @returns index of the instance geometry into the structure.
	 * @param instance_buffer Buffer containing instances
	 * @param instance_count Number of instances for this geometry
	 * @param transform_offset Offset of this geometry in the transform data buffer
	 * @param flags Ray tracing geometry flags
	 */
	uint64_t add_instance_geometry(BufferHandle        &instance_buffer,
	                               uint32_t             instance_count,
	                               uint32_t             transform_offset = 0,
	                               vk::GeometryFlagsKHR flags            = vk::GeometryFlagBitsKHR::eOpaque);

	void update_instance_geometry(uint64_t instance_UID, BufferHandle &instance_buffer,
	                              uint32_t             instance_count,
	                              uint32_t             transform_offset = 0,
	                              vk::GeometryFlagsKHR flags            = vk::GeometryFlagBitsKHR::eOpaque);

	/**
	 * @brief Builds the acceleration structure on the device (requires at least one geometry to be added)
	 * @param queue Queue to use for the build process
	 * @param flags Build flags
	 * @param mode Build mode (build or update)
	 */
	void build(vk::Queue                              queue,
	           vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
	           vk::BuildAccelerationStructureModeKHR  mode  = vk::BuildAccelerationStructureModeKHR::eBuild);

	vk::AccelerationStructureKHR get_handle() const;

	const vk::AccelerationStructureKHR *get() const;

	uint64_t get_device_address() const;

	void reset_geometries()
	{
		geometries.clear();
	}

  private:
	ContextPtr context;

	vk::AccelerationStructureKHR acceleration_structure{VK_NULL_HANDLE};

	uint64_t device_address{0};

	vk::AccelerationStructureTypeKHR type{};

	vk::AccelerationStructureBuildSizesInfoKHR build_sizes_info{};

	struct Geometry
	{
		vk::AccelerationStructureGeometryKHR geometry{};
		uint32_t                             primitive_count{};
		uint32_t                             transform_offset{};
		bool                                 updated = false;
	};

	BufferHandle scratch_buffer;

	std::map<uint64_t, Geometry> geometries{};

	BufferHandle buffer{nullptr};
};
}        // namespace core
}        // namespace vkb