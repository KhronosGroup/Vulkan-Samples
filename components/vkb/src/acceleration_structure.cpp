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

#include "acceleration_structure.hpp"

namespace vkb
{
namespace core
{
AccelerationStructure::AccelerationStructure(ContextPtr context, vk::AccelerationStructureTypeKHR type) :
    context{context},
    type{type}
{
	assert(context && "Context must be valid");
}

AccelerationStructure::~AccelerationStructure()
{
	if (acceleration_structure)
	{
		context->device.destroyAccelerationStructureKHR(acceleration_structure);
	}
}

uint64_t AccelerationStructure::add_triangle_geometry(BufferHandle &vertex_buffer,
                                                      BufferHandle &index_buffer,
                                                      BufferHandle &transform_buffer,
                                                      uint32_t triangle_count, uint32_t max_vertex,
                                                      vk::DeviceSize vertex_stride, uint32_t transform_offset,
                                                      vk::Format vertex_format, vk::GeometryFlagsKHR flags,
                                                      uint64_t vertex_buffer_data_address,
                                                      uint64_t index_buffer_data_address,
                                                      uint64_t transform_buffer_data_address)
{
	vk::AccelerationStructureGeometryKHR geometry{};
	geometry.geometryType                                   = vk::GeometryTypeKHR::eTriangles;
	geometry.flags                                          = flags;
	geometry.geometry.triangles.sType                       = vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR;
	geometry.geometry.triangles.vertexFormat                = vertex_format;
	geometry.geometry.triangles.maxVertex                   = max_vertex;
	geometry.geometry.triangles.vertexStride                = vertex_stride;
	geometry.geometry.triangles.indexType                   = vk::IndexType::eUint32;
	geometry.geometry.triangles.vertexData.deviceAddress    = vertex_buffer_data_address == 0 ? vertex_buffer->device_address() : vertex_buffer_data_address;
	geometry.geometry.triangles.indexData.deviceAddress     = index_buffer_data_address == 0 ? index_buffer->device_address() : index_buffer_data_address;
	geometry.geometry.triangles.transformData.deviceAddress = transform_buffer_data_address == 0 ? transform_buffer->device_address() : transform_buffer_data_address;

	uint64_t index = geometries.size();
	geometries.insert({index, {geometry, triangle_count, transform_offset}});
	return index;
}

void AccelerationStructure::update_triangle_geometry(uint64_t      triangleUUID,
                                                     BufferHandle &vertex_buffer,
                                                     BufferHandle &index_buffer,
                                                     BufferHandle &transform_buffer,
                                                     uint32_t triangle_count, uint32_t max_vertex,
                                                     vk::DeviceSize vertex_stride, uint32_t transform_offset,
                                                     vk::Format vertex_format, vk::GeometryFlagsKHR flags,
                                                     uint64_t vertex_buffer_data_address,
                                                     uint64_t index_buffer_data_address,
                                                     uint64_t transform_buffer_data_address)
{
	vk::AccelerationStructureGeometryKHR *geometry           = &geometries[triangleUUID].geometry;
	geometry->geometryType                                   = vk::GeometryTypeKHR::eTriangles;
	geometry->flags                                          = flags;
	geometry->geometry.triangles.sType                       = vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR;
	geometry->geometry.triangles.vertexFormat                = vertex_format;
	geometry->geometry.triangles.maxVertex                   = max_vertex;
	geometry->geometry.triangles.vertexStride                = vertex_stride;
	geometry->geometry.triangles.indexType                   = vk::IndexType::eUint32;
	geometry->geometry.triangles.vertexData.deviceAddress    = vertex_buffer_data_address == 0 ? vertex_buffer->device_address() : vertex_buffer_data_address;
	geometry->geometry.triangles.indexData.deviceAddress     = index_buffer_data_address == 0 ? index_buffer->device_address() : index_buffer_data_address;
	geometry->geometry.triangles.transformData.deviceAddress = transform_buffer_data_address == 0 ? transform_buffer->device_address() : transform_buffer_data_address;
	geometries[triangleUUID].primitive_count                 = triangle_count;
	geometries[triangleUUID].transform_offset                = transform_offset;
	geometries[triangleUUID].updated                         = true;
}

uint64_t AccelerationStructure::add_instance_geometry(BufferHandle &instance_buffer, uint32_t instance_count, uint32_t transform_offset, vk::GeometryFlagsKHR flags)
{
	vk::AccelerationStructureGeometryKHR geometry{};
	geometry.geometryType                          = vk::GeometryTypeKHR::eInstances;
	geometry.flags                                 = flags;
	geometry.geometry.instances.sType              = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
	geometry.geometry.instances.arrayOfPointers    = VK_FALSE;
	geometry.geometry.instances.data.deviceAddress = instance_buffer->device_address();

	uint64_t index = geometries.size();
	geometries.insert({index, {geometry, instance_count, transform_offset}});
	return index;
}

void AccelerationStructure::update_instance_geometry(uint64_t      instance_UID,
                                                     BufferHandle &instance_buffer,
                                                     uint32_t instance_count, uint32_t transform_offset,
                                                     vk::GeometryFlagsKHR flags)
{
	vk::AccelerationStructureGeometryKHR *geometry  = &geometries[instance_UID].geometry;
	geometry->geometryType                          = vk::GeometryTypeKHR::eInstances;
	geometry->flags                                 = flags;
	geometry->geometry.instances.sType              = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
	geometry->geometry.instances.arrayOfPointers    = VK_FALSE;
	geometry->geometry.instances.data.deviceAddress = instance_buffer->device_address();
	geometries[instance_UID].primitive_count        = instance_count;
	geometries[instance_UID].transform_offset       = transform_offset;
	geometries[instance_UID].updated                = true;
}

void AccelerationStructure::build(vk::Queue queue, vk::BuildAccelerationStructureFlagsKHR flags, vk::BuildAccelerationStructureModeKHR mode)
{
	assert(!geometries.empty());

	std::vector<vk::AccelerationStructureGeometryKHR>       acceleration_structure_geometries;
	std::vector<vk::AccelerationStructureBuildRangeInfoKHR> acceleration_structure_build_range_infos;
	std::vector<uint32_t>                                   primitive_counts;
	for (auto &geometry : geometries)
	{
		if (mode == vk::BuildAccelerationStructureModeKHR::eUpdate && !geometry.second.updated)
		{
			continue;
		}
		acceleration_structure_geometries.push_back(geometry.second.geometry);
		// Infer build range info from geometry
		vk::AccelerationStructureBuildRangeInfoKHR build_range_info;
		build_range_info.primitiveCount  = geometry.second.primitive_count;
		build_range_info.primitiveOffset = 0;
		build_range_info.firstVertex     = 0;
		build_range_info.transformOffset = geometry.second.transform_offset;
		acceleration_structure_build_range_infos.push_back(build_range_info);
		primitive_counts.push_back(geometry.second.primitive_count);
		geometry.second.updated = false;
	}

	vk::AccelerationStructureBuildGeometryInfoKHR build_geometry_info{};
	build_geometry_info.type  = type;
	build_geometry_info.flags = flags;
	build_geometry_info.mode  = mode;
	if (mode == vk::BuildAccelerationStructureModeKHR::eUpdate && acceleration_structure != VK_NULL_HANDLE)
	{
		build_geometry_info.srcAccelerationStructure = acceleration_structure;
		build_geometry_info.dstAccelerationStructure = acceleration_structure;
	}
	build_geometry_info.geometryCount = static_cast<uint32_t>(acceleration_structure_geometries.size());
	build_geometry_info.pGeometries   = acceleration_structure_geometries.data();

	// Get required build sizes
	build_sizes_info = context->device.getAccelerationStructureBuildSizesKHR(
	    vk::AccelerationStructureBuildTypeKHR::eDevice,
	    build_geometry_info,
	    primitive_counts);

	// Create a buffer for the acceleration structure
	if (!buffer || buffer->properties().size != build_sizes_info.accelerationStructureSize)
	{
		buffer = context
		             ->buffer_pool()
		             ->request_buffer(build_sizes_info.accelerationStructureSize,
		                              vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
		                              VMA_MEMORY_USAGE_GPU_ONLY);

		vk::AccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
		acceleration_structure_create_info.buffer = buffer->handle();
		acceleration_structure_create_info.size   = build_sizes_info.accelerationStructureSize;
		acceleration_structure_create_info.type   = type;

		acceleration_structure = context->device.createAccelerationStructureKHR(acceleration_structure_create_info);
	}

	// Get the acceleration structure's handle

	device_address = context->device.getAccelerationStructureAddressKHR(
	    vk::AccelerationStructureDeviceAddressInfoKHR{
	        acceleration_structure,
	    });

	// Create a scratch buffer as a temporary storage for the acceleration structure build
	scratch_buffer = context->buffer_pool()->request_scratch_buffer(build_sizes_info.buildScratchSize);

	build_geometry_info.scratchData.deviceAddress = scratch_buffer->device_address();
	build_geometry_info.dstAccelerationStructure  = acceleration_structure;

	// Build the acceleration structure on the device via a one-time command buffer submission
	context->one_time_command(
	    vk::QueueFlagBits::eCompute,
	    [build_geometry_info, acceleration_structure_build_range_infos](vk::CommandBuffer command_buffer) {
		    // Record the build command
		    std::vector<const vk::AccelerationStructureBuildRangeInfoKHR *> acceleration_build_structure_range_info_ptrs{};
		    for (auto &acceleration_structure_build_range_info : acceleration_structure_build_range_infos)
		    {
			    acceleration_build_structure_range_info_ptrs.push_back(&acceleration_structure_build_range_info);
		    }
		    command_buffer.buildAccelerationStructuresKHR(build_geometry_info, acceleration_build_structure_range_info_ptrs);
	    },
	    [&]() {
		    // On completion, remove scratch buffer
		    scratch_buffer.reset();
	    });
}

vk::AccelerationStructureKHR AccelerationStructure::get_handle() const
{
	return acceleration_structure;
}

const vk::AccelerationStructureKHR *AccelerationStructure::get() const
{
	return &acceleration_structure;
}

uint64_t AccelerationStructure::get_device_address() const
{
	return device_address;
}

}        // namespace core
}        // namespace vkb