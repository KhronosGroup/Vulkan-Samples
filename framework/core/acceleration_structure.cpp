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
AccelerationStructure::AccelerationStructure(Device &                       device,
                                             VkAccelerationStructureTypeKHR type) :
    device{device},
    type{type}
{
}

AccelerationStructure::~AccelerationStructure()
{
	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyAccelerationStructureKHR(device.get_handle(), handle, nullptr);
	}
}

void AccelerationStructure::add_triangle_geometry(std::unique_ptr<vkb::core::Buffer> &vertex_buffer, std::unique_ptr<vkb::core::Buffer> &index_buffer, std::unique_ptr<vkb::core::Buffer> &transform_buffer, uint32_t triangle_count, uint32_t max_vertex, VkDeviceSize vertex_stride, uint32_t transform_offset, VkFormat vertex_format, VkGeometryFlagsKHR flags)
{
	VkAccelerationStructureGeometryKHR geometry{};
	geometry.sType                                          = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType                                   = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geometry.flags                                          = flags;
	geometry.geometry.triangles.sType                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	geometry.geometry.triangles.vertexFormat                = vertex_format;
	geometry.geometry.triangles.maxVertex                   = max_vertex;
	geometry.geometry.triangles.vertexStride                = vertex_stride;
	geometry.geometry.triangles.indexType                   = VK_INDEX_TYPE_UINT32;
	geometry.geometry.triangles.vertexData.deviceAddress    = vertex_buffer->get_device_address();
	geometry.geometry.triangles.indexData.deviceAddress     = index_buffer->get_device_address();
	geometry.geometry.triangles.transformData.deviceAddress = transform_buffer->get_device_address();

	geometries.push_back({geometry, triangle_count, transform_offset});
}

void AccelerationStructure::add_instance_geometry(std::unique_ptr<vkb::core::Buffer> &instance_buffer, uint32_t instance_count, uint32_t transform_offset, VkGeometryFlagsKHR flags)
{
	VkAccelerationStructureGeometryKHR geometry{};
	geometry.sType                                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry.flags                                 = flags;
	geometry.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry.geometry.instances.arrayOfPointers    = VK_FALSE;
	geometry.geometry.instances.data.deviceAddress = instance_buffer->get_device_address();

	geometries.push_back({geometry, instance_count, transform_offset});
}

void AccelerationStructure::build(VkQueue queue, VkBuildAccelerationStructureFlagsKHR flags, VkBuildAccelerationStructureModeKHR mode)
{
	assert(!geometries.empty());

	std::vector<VkAccelerationStructureGeometryKHR>         acceleration_structure_geometries;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR>   acceleration_structure_build_range_infos;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR *> pp_acceleration_structure_build_range_infos;
	std::vector<uint32_t>                                   primitive_counts;
	for (auto &geometry : geometries)
	{
		acceleration_structure_geometries.push_back(geometry.geometry);
		// Infer build range info from geometry
		VkAccelerationStructureBuildRangeInfoKHR build_range_info;
		build_range_info.primitiveCount  = geometry.primitive_count;
		build_range_info.primitiveOffset = 0;
		build_range_info.firstVertex     = 0;
		build_range_info.transformOffset = geometry.transform_offset;
		acceleration_structure_build_range_infos.push_back(build_range_info);
		primitive_counts.push_back(1);
	}
	for (size_t i = 0; i < acceleration_structure_build_range_infos.size(); i++)
	{
		pp_acceleration_structure_build_range_infos.push_back(&acceleration_structure_build_range_infos[i]);
	}

	VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info{};
	build_geometry_info.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_geometry_info.type          = type;
	build_geometry_info.flags         = flags;
	build_geometry_info.mode          = mode;
	build_geometry_info.geometryCount = static_cast<uint32_t>(acceleration_structure_geometries.size());
	build_geometry_info.pGeometries   = acceleration_structure_geometries.data();

	uint32_t primitive_count = build_geometry_info.geometryCount;

	// Get required build sizes
	build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
	    device.get_handle(),
	    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	    &build_geometry_info,
	    primitive_counts.data(),
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

	// Create a scratch buffer as a temporary storage for the acceleration structure build
	std::unique_ptr<vkb::core::ScratchBuffer> scratch_buffer = std::make_unique<vkb::core::ScratchBuffer>(device, build_sizes_info.buildScratchSize);

	build_geometry_info.scratchData.deviceAddress = scratch_buffer->get_device_address();
	build_geometry_info.dstAccelerationStructure  = handle;

	// Build the acceleration structure on the device via a one-time command buffer submission
	VkCommandBuffer command_buffer = device.create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vkCmdBuildAccelerationStructuresKHR(
	    command_buffer,
	    1,
	    &build_geometry_info,
	    pp_acceleration_structure_build_range_infos.data());
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