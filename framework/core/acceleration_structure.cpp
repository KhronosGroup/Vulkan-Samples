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

#include "acceleration_structure.h"

#include "device.h"

namespace vkb
{
namespace core
{
AccelerationStructure::AccelerationStructure(Device                        &device,
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

uint64_t AccelerationStructure::add_triangle_geometry(std::unique_ptr<vkb::core::Buffer> &vertex_buffer,
                                                      std::unique_ptr<vkb::core::Buffer> &index_buffer,
                                                      std::unique_ptr<vkb::core::Buffer> &transform_buffer,
                                                      uint32_t triangle_count, uint32_t max_vertex,
                                                      VkDeviceSize vertex_stride, uint32_t transform_offset,
                                                      VkFormat vertex_format, VkGeometryFlagsKHR flags,
                                                      uint64_t vertex_buffer_data_address,
                                                      uint64_t index_buffer_data_address,
                                                      uint64_t transform_buffer_data_address)
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
	geometry.geometry.triangles.vertexData.deviceAddress    = vertex_buffer_data_address == 0 ? vertex_buffer->get_device_address() : vertex_buffer_data_address;
	geometry.geometry.triangles.indexData.deviceAddress     = index_buffer_data_address == 0 ? index_buffer->get_device_address() : index_buffer_data_address;
	geometry.geometry.triangles.transformData.deviceAddress = transform_buffer_data_address == 0 ? transform_buffer->get_device_address() : transform_buffer_data_address;

	uint64_t index = geometries.size();
	geometries.insert({index, {geometry, triangle_count, transform_offset}});
	return index;
}

uint64_t AccelerationStructure::add_triangle_geometry(vkb::core::Buffer                  &vertex_buffer,
                                                      std::unique_ptr<vkb::core::Buffer> &index_buffer,
                                                      std::unique_ptr<vkb::core::Buffer> &transform_buffer,
                                                      uint32_t triangle_count, uint32_t max_vertex,
                                                      VkDeviceSize vertex_stride, uint32_t transform_offset,
                                                      VkFormat vertex_format, VkGeometryFlagsKHR flags,
                                                      uint64_t vertex_buffer_data_address,
                                                      uint64_t index_buffer_data_address,
                                                      uint64_t transform_buffer_data_address)
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
	geometry.geometry.triangles.vertexData.deviceAddress    = vertex_buffer_data_address == 0 ? vertex_buffer.get_device_address() : vertex_buffer_data_address;
	geometry.geometry.triangles.indexData.deviceAddress     = index_buffer_data_address == 0 ? index_buffer->get_device_address() : index_buffer_data_address;
	geometry.geometry.triangles.transformData.deviceAddress = transform_buffer_data_address == 0 ? transform_buffer->get_device_address() : transform_buffer_data_address;

	uint64_t index = geometries.size();
	geometries.insert({index, {geometry, triangle_count, transform_offset}});
	return index;
}

void AccelerationStructure::update_triangle_geometry(uint64_t                            triangleUUID,
                                                     std::unique_ptr<vkb::core::Buffer> &vertex_buffer,
                                                     std::unique_ptr<vkb::core::Buffer> &index_buffer,
                                                     std::unique_ptr<vkb::core::Buffer> &transform_buffer,
                                                     uint32_t triangle_count, uint32_t max_vertex,
                                                     VkDeviceSize vertex_stride, uint32_t transform_offset,
                                                     VkFormat vertex_format, VkGeometryFlagsKHR flags,
                                                     uint64_t vertex_buffer_data_address,
                                                     uint64_t index_buffer_data_address,
                                                     uint64_t transform_buffer_data_address)
{
	VkAccelerationStructureGeometryKHR *geometry             = &geometries[triangleUUID].geometry;
	geometry->sType                                          = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry->geometryType                                   = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geometry->flags                                          = flags;
	geometry->geometry.triangles.sType                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	geometry->geometry.triangles.vertexFormat                = vertex_format;
	geometry->geometry.triangles.maxVertex                   = max_vertex;
	geometry->geometry.triangles.vertexStride                = vertex_stride;
	geometry->geometry.triangles.indexType                   = VK_INDEX_TYPE_UINT32;
	geometry->geometry.triangles.vertexData.deviceAddress    = vertex_buffer_data_address == 0 ? vertex_buffer->get_device_address() : vertex_buffer_data_address;
	geometry->geometry.triangles.indexData.deviceAddress     = index_buffer_data_address == 0 ? index_buffer->get_device_address() : index_buffer_data_address;
	geometry->geometry.triangles.transformData.deviceAddress = transform_buffer_data_address == 0 ? transform_buffer->get_device_address() : transform_buffer_data_address;
	geometries[triangleUUID].primitive_count                 = triangle_count;
	geometries[triangleUUID].transform_offset                = transform_offset;
	geometries[triangleUUID].updated                         = true;
}

uint64_t AccelerationStructure::add_instance_geometry(std::unique_ptr<vkb::core::Buffer> &instance_buffer, uint32_t instance_count, uint32_t transform_offset, VkGeometryFlagsKHR flags)
{
	VkAccelerationStructureGeometryKHR geometry{};
	geometry.sType                                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry.flags                                 = flags;
	geometry.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry.geometry.instances.arrayOfPointers    = VK_FALSE;
	geometry.geometry.instances.data.deviceAddress = instance_buffer->get_device_address();

	uint64_t index = geometries.size();
	geometries.insert({index, {geometry, instance_count, transform_offset}});
	return index;
}

void AccelerationStructure::update_instance_geometry(uint64_t                            instance_UID,
                                                     std::unique_ptr<vkb::core::Buffer> &instance_buffer,
                                                     uint32_t instance_count, uint32_t transform_offset,
                                                     VkGeometryFlagsKHR flags)
{
	VkAccelerationStructureGeometryKHR *geometry    = &geometries[instance_UID].geometry;
	geometry->sType                                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry->geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry->flags                                 = flags;
	geometry->geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry->geometry.instances.arrayOfPointers    = VK_FALSE;
	geometry->geometry.instances.data.deviceAddress = instance_buffer->get_device_address();
	geometries[instance_UID].primitive_count        = instance_count;
	geometries[instance_UID].transform_offset       = transform_offset;
	geometries[instance_UID].updated                = true;
}

void AccelerationStructure::build(VkQueue queue, VkBuildAccelerationStructureFlagsKHR flags, VkBuildAccelerationStructureModeKHR mode)
{
	assert(!geometries.empty());

	std::vector<VkAccelerationStructureGeometryKHR>       acceleration_structure_geometries;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR> acceleration_structure_build_range_infos;
	std::vector<uint32_t>                                 primitive_counts;
	for (auto &geometry : geometries)
	{
		if (mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR && !geometry.second.updated)
		{
			continue;
		}
		acceleration_structure_geometries.push_back(geometry.second.geometry);
		// Infer build range info from geometry
		VkAccelerationStructureBuildRangeInfoKHR build_range_info;
		build_range_info.primitiveCount  = geometry.second.primitive_count;
		build_range_info.primitiveOffset = 0;
		build_range_info.firstVertex     = 0;
		build_range_info.transformOffset = geometry.second.transform_offset;
		acceleration_structure_build_range_infos.push_back(build_range_info);
		primitive_counts.push_back(geometry.second.primitive_count);
		geometry.second.updated = false;
	}

	VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info{};
	build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_geometry_info.type  = type;
	build_geometry_info.flags = flags;
	build_geometry_info.mode  = mode;
	if (mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR && handle != VK_NULL_HANDLE)
	{
		build_geometry_info.srcAccelerationStructure = handle;
		build_geometry_info.dstAccelerationStructure = handle;
	}
	build_geometry_info.geometryCount = static_cast<uint32_t>(acceleration_structure_geometries.size());
	build_geometry_info.pGeometries   = acceleration_structure_geometries.data();

	// Get required build sizes
	build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
	    device.get_handle(),
	    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	    &build_geometry_info,
	    primitive_counts.data(),
	    &build_sizes_info);

	// Create a buffer for the acceleration structure
	if (!buffer || buffer->get_size() != build_sizes_info.accelerationStructureSize)
	{
		buffer = std::make_unique<vkb::core::Buffer>(
		    device,
		    build_sizes_info.accelerationStructureSize,
		    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
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
	}

	// Get the acceleration structure's handle
	VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
	acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	acceleration_device_address_info.accelerationStructure = handle;
	device_address                                         = vkGetAccelerationStructureDeviceAddressKHR(device.get_handle(), &acceleration_device_address_info);

	// Create a scratch buffer as a temporary storage for the acceleration structure build
	scratch_buffer = std::make_unique<vkb::core::Buffer>(
	    device,
	    build_sizes_info.buildScratchSize,
	    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY);

	build_geometry_info.scratchData.deviceAddress = scratch_buffer->get_device_address();
	build_geometry_info.dstAccelerationStructure  = handle;

	// Build the acceleration structure on the device via a one-time command buffer submission
	VkCommandBuffer command_buffer       = device.create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	auto            as_build_range_infos = &*acceleration_structure_build_range_infos.data();
	vkCmdBuildAccelerationStructuresKHR(
	    command_buffer,
	    1,
	    &build_geometry_info,
	    &as_build_range_infos);
	device.flush_command_buffer(command_buffer, queue);
	scratch_buffer.reset();
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