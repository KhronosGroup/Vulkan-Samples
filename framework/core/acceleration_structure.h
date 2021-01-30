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

#include "common/helpers.h"
#include "common/vk_common.h"
#include "core/buffer.h"
#include "core/scratch_buffer.h"

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
	 * @param build_geometry_info Geometry information for the acceleration structure build
	 * @param primitive_count Number of primitives of this acceleration structure
	 */
	AccelerationStructure(Device &                                    device,
	                      VkAccelerationStructureTypeKHR              type,
	                      VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info,
	                      uint32_t                                    primitive_count);

	~AccelerationStructure();

	void build(const std::vector<VkAccelerationStructureGeometryKHR> & geometries,
	           std::vector<VkAccelerationStructureBuildRangeInfoKHR *> build_range_infos,
	           VkQueue                                                 queue,
	           VkBuildAccelerationStructureFlagsKHR                    flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	           VkBuildAccelerationStructureModeKHR                     mode  = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR);

	VkAccelerationStructureKHR get_handle() const;

	const VkAccelerationStructureKHR *get() const;

	uint64_t get_device_address() const;

  private:
	Device &device;

	VkAccelerationStructureKHR handle{VK_NULL_HANDLE};

	uint64_t device_address{0};

	VkAccelerationStructureTypeKHR type{};

	VkAccelerationStructureBuildSizesInfoKHR build_sizes_info{};

	std::unique_ptr<vkb::core::Buffer> buffer{nullptr};
};
}        // namespace core
}        // namespace vkb