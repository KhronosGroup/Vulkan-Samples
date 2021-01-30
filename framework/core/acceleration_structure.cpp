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

#include "acceleration_structure.h"

#include "device.h"

namespace vkb
{
namespace core
{
AccelerationStructure::AccelerationStructure(Device &                                    device,
                                             VkAccelerationStructureTypeKHR              type,
                                             VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info,
                                             uint32_t                                    primitive_count) :
    device{device},
    type{type}
{
	// Get required build sizes
	build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
	    device.get_handle(),
	    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	    &build_geometry_info,
	    &primitive_count,
	    &build_sizes_info);

	// Create a buffer for the acceleration structure
	buffer = std::make_unique<vkb::core::Buffer>(
	    device,
	    build_sizes_info.accelerationStructureSize,
	    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
	    VMA_MEMORY_USAGE_GPU_ONLY);

	VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
	acceleration_structure_create_info.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	acceleration_structure_create_info.buffer = buffer->get_handle();
	acceleration_structure_create_info.size   = build_sizes_info.accelerationStructureSize;
	acceleration_structure_create_info.type   = type;
	VkResult result                           = vkCreateAccelerationStructureKHR(device.get_handle(), &acceleration_structure_create_info, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Could not create acceleration structure"};
	}

	// Get the acceleration structure's handle
	VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
	acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	acceleration_device_address_info.accelerationStructure = handle;
	device_address                                         = vkGetAccelerationStructureDeviceAddressKHR(device.get_handle(), &acceleration_device_address_info);
}
AccelerationStructure::~AccelerationStructure()
{
	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyAccelerationStructureKHR(device.get_handle(), handle, nullptr);
	}
}
void AccelerationStructure::build(const std::vector<VkAccelerationStructureGeometryKHR> & geometries,
                                  std::vector<VkAccelerationStructureBuildRangeInfoKHR *> build_range_infos,
                                  VkQueue                                                 queue,
                                  VkBuildAccelerationStructureFlagsKHR                    flags,
                                  VkBuildAccelerationStructureModeKHR                     mode)
{
	// Create a scratch buffer as a temporary storage for the acceleration structure build
	std::unique_ptr<vkb::core::ScratchBuffer> scratch_buffer = std::make_unique<vkb::core::ScratchBuffer>(device, build_sizes_info.buildScratchSize);

	VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
	acceleration_build_geometry_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	acceleration_build_geometry_info.type                      = type;
	acceleration_build_geometry_info.flags                     = flags;
	acceleration_build_geometry_info.mode                      = mode;
	acceleration_build_geometry_info.dstAccelerationStructure  = handle;
	acceleration_build_geometry_info.geometryCount             = static_cast<uint32_t>(geometries.size());
	acceleration_build_geometry_info.pGeometries               = geometries.data();
	acceleration_build_geometry_info.scratchData.deviceAddress = scratch_buffer->get_device_address();

	// Build the acceleration structure on the device via a one-time command buffer submission
	VkCommandBuffer command_buffer = device.create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vkCmdBuildAccelerationStructuresKHR(
	    command_buffer,
	    1,
	    &acceleration_build_geometry_info,
	    build_range_infos.data());
	device.flush_command_buffer(command_buffer, queue);
}
VkAccelerationStructureKHR AccelerationStructure::get_handle() const
{
	return handle;
}
const VkAccelerationStructureKHR *AccelerationStructure::get() const
{
	return &handle;
}
uint64_t AccelerationStructure::get_device_address() const
{
	return device_address;
}
}        // namespace core
}        // namespace vkb