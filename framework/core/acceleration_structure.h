/* Copyright (c) 2021-2024, Sascha Willems
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

#include "common/helpers.h"
#include "common/vk_common.h"
#include "core/buffer.h"

namespace vkb
{
class Device;

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
	AccelerationStructure(Device                        &device,
	                      VkAccelerationStructureTypeKHR type);

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
	 * @param index_type Type of the indices
	 * @param flags Ray tracing geometry flags
	 * @param vertex_buffer_data_address set this if don't want the vertex_buffer data_address
	 * @param index_buffer_data_address set this if don't want the index_buffer data_address
	 * @param transform_buffer_data_address set this if don't want the transform_buffer data_address
	 */
	uint64_t add_triangle_geometry(vkb::core::BufferC &vertex_buffer,
	                               vkb::core::BufferC &index_buffer,
	                               vkb::core::BufferC &transform_buffer,
	                               uint32_t            triangle_count,
	                               uint32_t            max_vertex,
	                               VkDeviceSize        vertex_stride,
	                               uint32_t            transform_offset              = 0,
	                               VkFormat            vertex_format                 = VK_FORMAT_R32G32B32_SFLOAT,
	                               VkIndexType         index_type                    = VK_INDEX_TYPE_UINT32,
	                               VkGeometryFlagsKHR  flags                         = VK_GEOMETRY_OPAQUE_BIT_KHR,
	                               uint64_t            vertex_buffer_data_address    = 0,
	                               uint64_t            index_buffer_data_address     = 0,
	                               uint64_t            transform_buffer_data_address = 0);

	void update_triangle_geometry(uint64_t triangleUUID, std::unique_ptr<vkb::core::BufferC> &vertex_buffer,
	                              std::unique_ptr<vkb::core::BufferC> &index_buffer,
	                              std::unique_ptr<vkb::core::BufferC> &transform_buffer,
	                              uint32_t                             triangle_count,
	                              uint32_t                             max_vertex,
	                              VkDeviceSize                         vertex_stride,
	                              uint32_t                             transform_offset              = 0,
	                              VkFormat                             vertex_format                 = VK_FORMAT_R32G32B32_SFLOAT,
	                              VkGeometryFlagsKHR                   flags                         = VK_GEOMETRY_OPAQUE_BIT_KHR,
	                              uint64_t                             vertex_buffer_data_address    = 0,
	                              uint64_t                             index_buffer_data_address     = 0,
	                              uint64_t                             transform_buffer_data_address = 0);

	/**
	 * @brief Adds instance geometry to the acceleration structure (only valid for top level)
	 * @returns index of the instance geometry into the structure.
	 * @param instance_buffer Buffer containing instances
	 * @param instance_count Number of instances for this geometry
	 * @param transform_offset Offset of this geometry in the transform data buffer
	 * @param flags Ray tracing geometry flags
	 */
	uint64_t add_instance_geometry(std::unique_ptr<vkb::core::BufferC> &instance_buffer,
	                               uint32_t                             instance_count,
	                               uint32_t                             transform_offset = 0,
	                               VkGeometryFlagsKHR                   flags            = VK_GEOMETRY_OPAQUE_BIT_KHR);

	void update_instance_geometry(uint64_t instance_UID, std::unique_ptr<vkb::core::BufferC> &instance_buffer,
	                              uint32_t           instance_count,
	                              uint32_t           transform_offset = 0,
	                              VkGeometryFlagsKHR flags            = VK_GEOMETRY_OPAQUE_BIT_KHR);

	/**
	 * @brief Builds the acceleration structure on the device (requires at least one geometry to be added)
	 * @param queue Queue to use for the build process
	 * @param flags Build flags
	 * @param mode Build mode (build or update)
	 */
	void build(VkQueue                              queue,
	           VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	           VkBuildAccelerationStructureModeKHR  mode  = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR);

	VkAccelerationStructureKHR get_handle() const;

	const VkAccelerationStructureKHR *get() const;

	uint64_t get_device_address() const;

	vkb::core::BufferC *get_buffer() const
	{
		return buffer.get();
	}

	void resetGeometries()
	{
		geometries.clear();
	}

  private:
	Device &device;

	VkAccelerationStructureKHR handle{VK_NULL_HANDLE};

	uint64_t device_address{0};

	VkAccelerationStructureTypeKHR type{};

	VkAccelerationStructureBuildSizesInfoKHR build_sizes_info{};

	struct Geometry
	{
		VkAccelerationStructureGeometryKHR geometry{};
		uint32_t                           primitive_count{};
		uint32_t                           transform_offset{};
		bool                               updated = false;
	};

	std::unique_ptr<vkb::core::BufferC> scratch_buffer;

	std::map<uint64_t, Geometry> geometries{};

	std::unique_ptr<vkb::core::BufferC> buffer{nullptr};
};
}        // namespace core
}        // namespace vkb