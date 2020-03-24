/* Copyright (c) 2019-2020, Sascha Willems
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

/*
 * Basic example for ray tracing using VK_KHR_ray_tracing
 * @note: Work-in-progress
 * @note: The extension is still beta, so you need to either enable VKB_VK_ENABLE_BETA_EXTENSIONS in cmake to include beta header and functions
 */

#include "raytracing_basic.h"

/*
	Create a scratch buffer used as a temporary storage in various ray tracing structures
*/
RayTracingScratchBuffer::RayTracingScratchBuffer(vkb::Device &device, VkAccelerationStructureKHR acceleration_structure)
{
	VkMemoryRequirements2 memory_requirements_2{};
	memory_requirements_2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

	VkAccelerationStructureMemoryRequirementsInfoKHR acceleration_memory_requirements{};
	acceleration_memory_requirements.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
	acceleration_memory_requirements.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
	acceleration_memory_requirements.buildType             = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
	acceleration_memory_requirements.accelerationStructure = acceleration_structure;
	vkGetAccelerationStructureMemoryRequirementsKHR(device.get_handle(), &acceleration_memory_requirements, &memory_requirements_2);

	VkBufferCreateInfo buffer_create_info{};
	buffer_create_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size        = memory_requirements_2.memoryRequirements.size;
	buffer_create_info.usage       = VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(device.get_handle(), &buffer_create_info, nullptr, &buffer));

	VkMemoryRequirements memory_requirements{};
	vkGetBufferMemoryRequirements(device.get_handle(), buffer, &memory_requirements);

	VkMemoryAllocateFlagsInfo memory_allocate_flags_info{};
	memory_allocate_flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	memory_allocate_flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	VkMemoryAllocateInfo memory_allocate_info{};
	memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.pNext           = &memory_allocate_flags_info;
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = device.get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(device.get_handle(), &memory_allocate_info, nullptr, &memory));
	VK_CHECK(vkBindBufferMemory(device.get_handle(), buffer, memory, 0));

	VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
	buffer_device_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	buffer_device_address_info.buffer = buffer;
	device_address                    = vkGetBufferDeviceAddressKHR(device.get_handle(), &buffer_device_address_info);
}

/*
	Create memory that will be attached to an acceleration structure
*/
RayTracingObjectMemory::RayTracingObjectMemory(vkb::Device &device, VkAccelerationStructureKHR acceleration_structure)
{
	VkMemoryRequirements2 memory_requirements_2{};
	memory_requirements_2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

	VkAccelerationStructureMemoryRequirementsInfoKHR acceleration_memory_requirements{};
	acceleration_memory_requirements.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
	acceleration_memory_requirements.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
	acceleration_memory_requirements.buildType             = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
	acceleration_memory_requirements.accelerationStructure = acceleration_structure;
	vkGetAccelerationStructureMemoryRequirementsKHR(device.get_handle(), &acceleration_memory_requirements, &memory_requirements_2);

	VkMemoryRequirements memory_requirements = memory_requirements_2.memoryRequirements;

	VkMemoryAllocateInfo memory_allocate_info{};
	memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = device.get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(device.get_handle(), &memory_allocate_info, nullptr, &memory));
}

RaytracingBasic::RaytracingBasic()
{
	title = "VK_KHR_ray_tracing";
	// Enable instance and device extensions required to use VK_KHR_ray_tracing
	instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	device_extensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
	device_extensions.push_back(VK_KHR_RAY_TRACING_EXTENSION_NAME);
	device_extensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	device_extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	device_extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	device_extensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
#if defined(VKB_DEBUG)
	// @todo: NV-Extension currently required to get the validation layers working
	device_extensions.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);
#endif
}

RaytracingBasic::~RaytracingBasic()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		vkDestroyImageView(get_device().get_handle(), storage_image.view, nullptr);
		vkDestroyImage(get_device().get_handle(), storage_image.image, nullptr);
		vkFreeMemory(get_device().get_handle(), storage_image.memory, nullptr);
		vkDestroyAccelerationStructureKHR(device->get_handle(), bottom_level_acceleration_structure, nullptr);
		vkDestroyAccelerationStructureKHR(device->get_handle(), top_level_acceleration_structure, nullptr);
		vertex_buffer.reset();
		index_buffer.reset();
		shader_binding_table.reset();
		ubo.reset();
	}
}

/*
	Set up a storage image that the ray generation shader will be writing to
*/
void RaytracingBasic::create_storage_image()
{
	VkImageCreateInfo image = vkb::initializers::image_create_info();
	image.imageType         = VK_IMAGE_TYPE_2D;
	image.format            = VK_FORMAT_B8G8R8A8_UNORM;
	image.extent.width      = width;
	image.extent.height     = height;
	image.extent.depth      = 1;
	image.mipLevels         = 1;
	image.arrayLayers       = 1;
	image.samples           = VK_SAMPLE_COUNT_1_BIT;
	image.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image.usage             = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	image.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK(vkCreateImage(get_device().get_handle(), &image, nullptr, &storage_image.image));

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(get_device().get_handle(), storage_image.image, &memory_requirements);
	VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
	memory_allocate_info.allocationSize       = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex      = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &storage_image.memory));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), storage_image.image, storage_image.memory, 0));

	VkImageViewCreateInfo color_image_view           = vkb::initializers::image_view_create_info();
	color_image_view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	color_image_view.format                          = VK_FORMAT_B8G8R8A8_UNORM;
	color_image_view.subresourceRange                = {};
	color_image_view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	color_image_view.subresourceRange.baseMipLevel   = 0;
	color_image_view.subresourceRange.levelCount     = 1;
	color_image_view.subresourceRange.baseArrayLayer = 0;
	color_image_view.subresourceRange.layerCount     = 1;
	color_image_view.image                           = storage_image.image;
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &color_image_view, nullptr, &storage_image.view));

	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vkb::set_image_layout(command_buffer, storage_image.image,
	                      VK_IMAGE_LAYOUT_UNDEFINED,
	                      VK_IMAGE_LAYOUT_GENERAL,
	                      {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
	get_device().flush_command_buffer(command_buffer, queue);
}

/*
	Get the device address from a buffer
*/
uint64_t RaytracingBasic::get_buffer_device_address(VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
	buffer_device_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	buffer_device_address_info.buffer = buffer;
	return vkGetBufferDeviceAddressKHR(device->get_handle(), &buffer_device_address_info);
}

/*
	Create scene geometry and ray tracing acceleration structures
*/
void RaytracingBasic::create_scene()
{
	// Setup vertices for a single triangle
	struct Vertex
	{
		float pos[3];
	};
	std::vector<Vertex> vertices = {
	    {{1.0f, 1.0f, 0.0f}},
	    {{-1.0f, 1.0f, 0.0f}},
	    {{0.0f, -1.0f, 0.0f}}};

	// Setup indices
	std::vector<uint32_t> indices = {0, 1, 2};
	index_count                   = static_cast<uint32_t>(indices.size());

	auto vertex_buffer_size = vertices.size() * sizeof(Vertex);
	auto index_buffer_size  = indices.size() * sizeof(uint32_t);

	// Create buffers
	// For the sake of simplicity we won't stage the vertex data to the gpu memory
	// Vertex buffer
	vertex_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                    vertex_buffer_size,
	                                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	// Index buffer
	index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                   index_buffer_size,
	                                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	                                                   VMA_MEMORY_USAGE_CPU_TO_GPU);
	index_buffer->update(indices.data(), index_buffer_size);

	VkDeviceOrHostAddressConstKHR vertex_data_device_address{};
	VkDeviceOrHostAddressConstKHR index_data_device_address{};

	VkBufferDeviceAddressInfo buffer_device_address_info{};
	buffer_device_address_info.sType         = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	buffer_device_address_info.buffer        = vertex_buffer->get_handle();
	vertex_data_device_address.deviceAddress = vkGetBufferDeviceAddressKHR(device->get_handle(), &buffer_device_address_info);

	buffer_device_address_info.buffer       = index_buffer->get_handle();
	index_data_device_address.deviceAddress = vkGetBufferDeviceAddressKHR(device->get_handle(), &buffer_device_address_info);

	/*
		Create the bottom level acceleration structure containing the actual scene geometry as triangles
	*/
	{
		VkAccelerationStructureCreateGeometryTypeInfoKHR acceleration_create_geometry_info{};
		acceleration_create_geometry_info.sType             = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
		acceleration_create_geometry_info.geometryType      = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		acceleration_create_geometry_info.maxPrimitiveCount = 1;
		acceleration_create_geometry_info.indexType         = VK_INDEX_TYPE_UINT32;
		acceleration_create_geometry_info.maxVertexCount    = vertices.size();
		acceleration_create_geometry_info.vertexFormat      = VK_FORMAT_R32G32B32_SFLOAT;
		acceleration_create_geometry_info.allowsTransforms  = VK_FALSE;

		VkAccelerationStructureCreateInfoKHR acceleration_create_info{};
		acceleration_create_info.sType            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		acceleration_create_info.type             = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		acceleration_create_info.flags            = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		acceleration_create_info.maxGeometryCount = 1;
		acceleration_create_info.pGeometryInfos   = &acceleration_create_geometry_info;
		VK_CHECK(vkCreateAccelerationStructureKHR(device->get_handle(), &acceleration_create_info, nullptr, &bottom_level_acceleration_structure));

		// Bind object memory to the top level acceleration structure
		RayTracingObjectMemory object_memory(get_device(), bottom_level_acceleration_structure);

		VkBindAccelerationStructureMemoryInfoKHR bind_acceleration_memory_info{};
		bind_acceleration_memory_info.sType                 = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
		bind_acceleration_memory_info.accelerationStructure = bottom_level_acceleration_structure;
		bind_acceleration_memory_info.memory                = object_memory.memory;
		VK_CHECK(vkBindAccelerationStructureMemoryKHR(device->get_handle(), 1, &bind_acceleration_memory_info));

		VkAccelerationStructureGeometryKHR acceleration_geometry{};
		acceleration_geometry.sType                                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		acceleration_geometry.flags                                       = VK_GEOMETRY_OPAQUE_BIT_KHR;
		acceleration_geometry.geometryType                                = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		acceleration_geometry.geometry.triangles.sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		acceleration_geometry.geometry.triangles.vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT;
		acceleration_geometry.geometry.triangles.vertexData.deviceAddress = vertex_data_device_address.deviceAddress;
		acceleration_geometry.geometry.triangles.vertexStride             = sizeof(Vertex);
		acceleration_geometry.geometry.triangles.indexType                = VK_INDEX_TYPE_UINT32;
		acceleration_geometry.geometry.triangles.indexData.deviceAddress  = index_data_device_address.deviceAddress;

		std::vector<VkAccelerationStructureGeometryKHR> acceleration_geometries           = {acceleration_geometry};
		VkAccelerationStructureGeometryKHR *            acceleration_structure_geometries = acceleration_geometries.data();

		// Create a small scratch buffer used during build of the bottom level acceleration structure
		RayTracingScratchBuffer scratch_buffer(get_device(), bottom_level_acceleration_structure);

		VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
		acceleration_build_geometry_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		acceleration_build_geometry_info.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		acceleration_build_geometry_info.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		acceleration_build_geometry_info.update                    = VK_FALSE;
		acceleration_build_geometry_info.dstAccelerationStructure  = bottom_level_acceleration_structure;
		acceleration_build_geometry_info.geometryArrayOfPointers   = VK_FALSE;
		acceleration_build_geometry_info.geometryCount             = 1;
		acceleration_build_geometry_info.ppGeometries              = &acceleration_structure_geometries;
		acceleration_build_geometry_info.scratchData.deviceAddress = scratch_buffer.device_address;

		VkAccelerationStructureBuildOffsetInfoKHR acceleration_build_offset_info{};
		acceleration_build_offset_info.primitiveCount  = 1;
		acceleration_build_offset_info.primitiveOffset = 0x0;
		acceleration_build_offset_info.firstVertex     = 0;
		acceleration_build_offset_info.transformOffset = 0x0;

		std::vector<VkAccelerationStructureBuildOffsetInfoKHR *> acceleration_build_offsets = {&acceleration_build_offset_info};

		if (ray_tracing_features.rayTracingHostAccelerationStructureCommands)
		{
			// Implementation supports building acceleration structure building on host
			VK_CHECK(vkBuildAccelerationStructureKHR(device->get_handle(), 1, &acceleration_build_geometry_info, acceleration_build_offsets.data()));
		}
		else
		{
			// Acceleration structure needs to be build on the device
			VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			vkCmdBuildAccelerationStructureKHR(command_buffer, 1, &acceleration_build_geometry_info, acceleration_build_offsets.data());
			get_device().flush_command_buffer(command_buffer, queue);
		}

		VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
		acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		acceleration_device_address_info.accelerationStructure = bottom_level_acceleration_structure;

		bottom_level_acceleration_structure_handle = vkGetAccelerationStructureDeviceAddressKHR(device->get_handle(), &acceleration_device_address_info);
	}

	/*
		Create the top level acceleration structure containing geometry instances
	*/
	{
		VkAccelerationStructureCreateGeometryTypeInfoKHR acceleration_create_geometry_info{};
		acceleration_create_geometry_info.sType             = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
		acceleration_create_geometry_info.geometryType      = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		acceleration_create_geometry_info.maxPrimitiveCount = 1;
		acceleration_create_geometry_info.allowsTransforms  = VK_FALSE;

		VkAccelerationStructureCreateInfoKHR acceleration_create_info{};
		acceleration_create_info.sType            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		acceleration_create_info.type             = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		acceleration_create_info.flags            = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		acceleration_create_info.maxGeometryCount = 1;
		acceleration_create_info.pGeometryInfos   = &acceleration_create_geometry_info;
		VK_CHECK(vkCreateAccelerationStructureKHR(device->get_handle(), &acceleration_create_info, nullptr, &top_level_acceleration_structure));

		// Bind object memory to the top level acceleration structure
		RayTracingObjectMemory object_memory(get_device(), top_level_acceleration_structure);

		VkBindAccelerationStructureMemoryInfoKHR bind_acceleration_memory_info{};
		bind_acceleration_memory_info.sType                 = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
		bind_acceleration_memory_info.accelerationStructure = top_level_acceleration_structure;
		bind_acceleration_memory_info.memory                = object_memory.memory;
		VK_CHECK(vkBindAccelerationStructureMemoryKHR(device->get_handle(), 1, &bind_acceleration_memory_info));

		VkTransformMatrixKHR transform_matrix = {
		    1.0f, 0.0f, 0.0f, 0.0f,
		    0.0f, 1.0f, 0.0f, 0.0f,
		    0.0f, 0.0f, 1.0f, 0.0f};

		VkAccelerationStructureInstanceKHR instance{};
		instance.transform                              = transform_matrix;
		instance.instanceCustomIndex                    = 0;
		instance.mask                                   = 0xFF;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instance.accelerationStructureReference         = bottom_level_acceleration_structure_handle;

		std::unique_ptr<vkb::core::Buffer> instances_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
		                                                                                          sizeof(instance),
		                                                                                          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		                                                                                          VMA_MEMORY_USAGE_CPU_TO_GPU);
		instances_buffer->update(&instance, sizeof(VkAccelerationStructureInstanceKHR));

		buffer_device_address_info.buffer = instances_buffer->get_handle();
		VkDeviceOrHostAddressConstKHR instance_data_device_address{};
		instance_data_device_address.deviceAddress = vkGetBufferDeviceAddressKHR(device->get_handle(), &buffer_device_address_info);

		VkAccelerationStructureGeometryKHR acceleration_geometry{};
		acceleration_geometry.sType                                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		acceleration_geometry.flags                                 = VK_GEOMETRY_OPAQUE_BIT_KHR;
		acceleration_geometry.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		acceleration_geometry.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		acceleration_geometry.geometry.instances.arrayOfPointers    = VK_FALSE;
		acceleration_geometry.geometry.instances.data.deviceAddress = instance_data_device_address.deviceAddress;

		std::vector<VkAccelerationStructureGeometryKHR> acceleration_geometries           = {acceleration_geometry};
		VkAccelerationStructureGeometryKHR *            acceleration_structure_geometries = acceleration_geometries.data();

		// Create a small scratch buffer used during build of the top level acceleration structure
		RayTracingScratchBuffer scratch_buffer(get_device(), top_level_acceleration_structure);

		VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
		acceleration_build_geometry_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		acceleration_build_geometry_info.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		acceleration_build_geometry_info.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		acceleration_build_geometry_info.update                    = VK_FALSE;
		acceleration_build_geometry_info.srcAccelerationStructure  = VK_NULL_HANDLE;
		acceleration_build_geometry_info.dstAccelerationStructure  = top_level_acceleration_structure;
		acceleration_build_geometry_info.geometryArrayOfPointers   = VK_FALSE;
		acceleration_build_geometry_info.geometryCount             = 1;
		acceleration_build_geometry_info.ppGeometries              = &acceleration_structure_geometries;
		acceleration_build_geometry_info.scratchData.deviceAddress = scratch_buffer.device_address;

		VkAccelerationStructureBuildOffsetInfoKHR acceleration_build_offset_info{};
		acceleration_build_offset_info.primitiveCount                                       = 1;
		acceleration_build_offset_info.primitiveOffset                                      = 0x0;
		acceleration_build_offset_info.firstVertex                                          = 0;
		acceleration_build_offset_info.transformOffset                                      = 0x0;
		std::vector<VkAccelerationStructureBuildOffsetInfoKHR *> acceleration_build_offsets = {&acceleration_build_offset_info};

		if (ray_tracing_features.rayTracingHostAccelerationStructureCommands)
		{
			// Implementation supports building acceleration structure building on host
			VK_CHECK(vkBuildAccelerationStructureKHR(device->get_handle(), 1, &acceleration_build_geometry_info, acceleration_build_offsets.data()));
		}
		else
		{
			// Acceleration structure needs to be build on the device
			VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			vkCmdBuildAccelerationStructureKHR(command_buffer, 1, &acceleration_build_geometry_info, acceleration_build_offsets.data());
			get_device().flush_command_buffer(command_buffer, queue);
		}

		VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
		acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		acceleration_device_address_info.accelerationStructure = top_level_acceleration_structure;

		top_level_acceleration_structure_handle = vkGetAccelerationStructureDeviceAddressKHR(device->get_handle(), &acceleration_device_address_info);
	}
}

VkDeviceSize RaytracingBasic::copy_shader_identifier(uint8_t *data, const uint8_t *shaderHandleStorage, uint32_t groupIndex)
{
	const uint32_t shader_group_handle_size = ray_tracing_properties.shaderGroupHandleSize;
	memcpy(data, shaderHandleStorage + groupIndex * shader_group_handle_size, shader_group_handle_size);
	data += shader_group_handle_size;
	return shader_group_handle_size;
}

/*
	Create the Shader Binding Table that binds the programs and top-level acceleration structure
*/
void RaytracingBasic::create_shader_binding_table()
{
	uint32_t shader_binding_table_size = ray_tracing_properties.shaderGroupHandleSize * 3;
	// Create buffer to hold the shader binding table
	shader_binding_table = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                           shader_binding_table_size,
	                                                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR,
	                                                           VMA_MEMORY_USAGE_CPU_TO_GPU, 0);

	auto shader_handle_storage = new uint8_t[shader_binding_table_size];
	// Get shader group handles
	auto *data = static_cast<uint8_t *>(shader_binding_table->map());
	VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(get_device().get_handle(), pipeline, 0, 3, shader_binding_table_size, data));
	shader_binding_table->unmap();
}

/*
	Create the descriptor sets used for the ray tracing dispatch
*/
void RaytracingBasic::create_descriptor_sets()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
	    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
	    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};
	VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(pool_sizes, 1);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info, &descriptor_set));

	VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{};
	descriptor_acceleration_structure_info.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptor_acceleration_structure_info.accelerationStructureCount = 1;
	descriptor_acceleration_structure_info.pAccelerationStructures    = &top_level_acceleration_structure;

	VkWriteDescriptorSet acceleration_structure_write{};
	acceleration_structure_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	// The specialized acceleration structure descriptor has to be chained
	acceleration_structure_write.pNext           = &descriptor_acceleration_structure_info;
	acceleration_structure_write.dstSet          = descriptor_set;
	acceleration_structure_write.dstBinding      = 0;
	acceleration_structure_write.descriptorCount = 1;
	acceleration_structure_write.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

	VkDescriptorImageInfo image_descriptor{};
	image_descriptor.imageView   = storage_image.view;
	image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*ubo);

	VkWriteDescriptorSet result_image_write   = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &image_descriptor);
	VkWriteDescriptorSet uniform_buffer_write = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &buffer_descriptor);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    acceleration_structure_write,
	    result_image_write,
	    uniform_buffer_write};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
}

/*
	Create our ray tracing pipeline
*/
void RaytracingBasic::create_ray_tracing_pipeline()
{
	VkDescriptorSetLayoutBinding acceleration_structure_layout_binding{};
	acceleration_structure_layout_binding.binding         = 0;
	acceleration_structure_layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	acceleration_structure_layout_binding.descriptorCount = 1;
	acceleration_structure_layout_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	VkDescriptorSetLayoutBinding result_image_layout_binding{};
	result_image_layout_binding.binding         = 1;
	result_image_layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	result_image_layout_binding.descriptorCount = 1;
	result_image_layout_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	VkDescriptorSetLayoutBinding uniform_buffer_binding{};
	uniform_buffer_binding.binding         = 2;
	uniform_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniform_buffer_binding.descriptorCount = 1;
	uniform_buffer_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	std::vector<VkDescriptorSetLayoutBinding> bindings({acceleration_structure_layout_binding,
	                                                    result_image_layout_binding,
	                                                    uniform_buffer_binding});

	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
	layout_info.pBindings    = bindings.data();
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &layout_info, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
	pipeline_layout_create_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = 1;
	pipeline_layout_create_info.pSetLayouts    = &descriptor_set_layout;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));

	std::array<VkPipelineShaderStageCreateInfo, 3> shader_stages;
	shader_stages[INDEX_RAYGEN]      = load_shader("khr_ray_tracing_basic/raygen.rgen", VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	shader_stages[INDEX_MISS]        = load_shader("khr_ray_tracing_basic/miss.rmiss", VK_SHADER_STAGE_MISS_BIT_KHR);
	shader_stages[INDEX_CLOSEST_HIT] = load_shader("khr_ray_tracing_basic/closesthit.rchit", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

	/*
		Setup ray tracing shader groups
	*/
	std::array<VkRayTracingShaderGroupCreateInfoKHR, 3> groups{};
	for (auto &group : groups)
	{
		// Init all groups with some default values
		group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		group.generalShader      = VK_SHADER_UNUSED_KHR;
		group.closestHitShader   = VK_SHADER_UNUSED_KHR;
		group.anyHitShader       = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;
	}

	// Links shaders and types to ray tracing shader groups
	groups[INDEX_RAYGEN].type                  = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	groups[INDEX_RAYGEN].generalShader         = INDEX_RAYGEN;
	groups[INDEX_MISS].type                    = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	groups[INDEX_MISS].generalShader           = INDEX_MISS;
	groups[INDEX_CLOSEST_HIT].type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	groups[INDEX_CLOSEST_HIT].closestHitShader = INDEX_CLOSEST_HIT;

	VkRayTracingPipelineCreateInfoKHR raytracing_pipeline_create_info{};
	raytracing_pipeline_create_info.sType             = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	raytracing_pipeline_create_info.stageCount        = static_cast<uint32_t>(shader_stages.size());
	raytracing_pipeline_create_info.pStages           = shader_stages.data();
	raytracing_pipeline_create_info.groupCount        = static_cast<uint32_t>(groups.size());
	raytracing_pipeline_create_info.pGroups           = groups.data();
	raytracing_pipeline_create_info.maxRecursionDepth = 1;
	raytracing_pipeline_create_info.layout            = pipeline_layout;
	raytracing_pipeline_create_info.libraries.sType   = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
	VK_CHECK(vkCreateRayTracingPipelinesKHR(get_device().get_handle(), VK_NULL_HANDLE, 1, &raytracing_pipeline_create_info, nullptr, &pipeline));
}

/*
	Create the uniform buffer used to pass matrices to the ray tracing ray generation shader
*/
void RaytracingBasic::create_uniform_buffer()
{
	ubo = std::make_unique<vkb::core::Buffer>(get_device(),
	                                          sizeof(uniform_data),
	                                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                          VMA_MEMORY_USAGE_CPU_TO_GPU);
	ubo->convert_and_update(uniform_data);

	update_uniform_buffers();
}

/*
	Command buffer generation
*/
void RaytracingBasic::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		/*
			Setup the strided buffer regions pointing to the shaders in our shader binding table
		*/

		VkStridedBufferRegionKHR raygen_shader_sbt_entry{};
		raygen_shader_sbt_entry.buffer = shader_binding_table->get_handle();
		raygen_shader_sbt_entry.offset = static_cast<VkDeviceSize>(ray_tracing_properties.shaderGroupHandleSize * INDEX_RAYGEN);
		raygen_shader_sbt_entry.size   = ray_tracing_properties.shaderGroupHandleSize;

		VkStridedBufferRegionKHR miss_shader_sbt_entry{};
		miss_shader_sbt_entry.buffer = shader_binding_table->get_handle();
		miss_shader_sbt_entry.offset = static_cast<VkDeviceSize>(ray_tracing_properties.shaderGroupHandleSize * INDEX_MISS);
		miss_shader_sbt_entry.size   = ray_tracing_properties.shaderGroupHandleSize;

		VkStridedBufferRegionKHR hit_shader_sbt_entry{};
		hit_shader_sbt_entry.buffer = shader_binding_table->get_handle();
		hit_shader_sbt_entry.offset = static_cast<VkDeviceSize>(ray_tracing_properties.shaderGroupHandleSize * INDEX_CLOSEST_HIT);
		hit_shader_sbt_entry.size   = ray_tracing_properties.shaderGroupHandleSize;

		VkStridedBufferRegionKHR callable_shader_sbt_entry{};

		/*
			Dispatch the ray tracing commands
		*/
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_layout, 0, 1, &descriptor_set, 0, 0);

		vkCmdTraceRaysKHR(
		    draw_cmd_buffers[i],
		    &raygen_shader_sbt_entry,
		    &miss_shader_sbt_entry,
		    &hit_shader_sbt_entry,
		    &callable_shader_sbt_entry,
		    width,
		    height,
		    1);

		/*
			Copy raytracing output to swap chain image
		*/
		// Prepare current swapchain image as transfer destination
		vkb::set_image_layout(
		    draw_cmd_buffers[i],
		    get_render_context().get_swapchain().get_images()[i],
		    VK_IMAGE_LAYOUT_UNDEFINED,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    subresource_range);

		// Prepare ray tracing output image as transfer source
		vkb::set_image_layout(
		    draw_cmd_buffers[i],
		    storage_image.image,
		    VK_IMAGE_LAYOUT_GENERAL,
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    subresource_range);

		VkImageCopy copy_region{};
		copy_region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		copy_region.srcOffset      = {0, 0, 0};
		copy_region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		copy_region.dstOffset      = {0, 0, 0};
		copy_region.extent         = {width, height, 1};
		vkCmdCopyImage(draw_cmd_buffers[i], storage_image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		               get_render_context().get_swapchain().get_images()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

		// Transition swap chain image back for presentation
		vkb::set_image_layout(
		    draw_cmd_buffers[i],
		    get_render_context().get_swapchain().get_images()[i],
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		    subresource_range);

		// Transition ray tracing output image back to general layout
		vkb::set_image_layout(
		    draw_cmd_buffers[i],
		    storage_image.image,
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    VK_IMAGE_LAYOUT_GENERAL,
		    subresource_range);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void RaytracingBasic::update_uniform_buffers()
{
	uniform_data.proj_inverse = glm::inverse(camera.matrices.perspective);
	uniform_data.view_inverse = glm::inverse(camera.matrices.view);
	ubo->convert_and_update(uniform_data);
}

bool RaytracingBasic::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	// This sample copies ray traced output to the swapchain image, so we need to enable the required image usage flags
	std::set<VkImageUsageFlagBits> image_usage_flags = {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT};
	get_render_context().update_swapchain(image_usage_flags);

	// Query the ray tracing properties and features of the current implementation, we will need them later on
	ray_tracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 device_properties{};
	device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	device_properties.pNext = &ray_tracing_properties;
	vkGetPhysicalDeviceProperties2(get_device().get_physical_device(), &device_properties);

	ray_tracing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR;
	VkPhysicalDeviceFeatures2 device_features{};
	device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	device_features.pNext = &ray_tracing_features;
	vkGetPhysicalDeviceFeatures2(get_device().get_physical_device(), &device_features);

	// Note: Using Revsered depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.type = vkb::CameraType::LookAt;
	camera.set_perspective(60.0f, (float) width / (float) height, 512.0f, 0.1f);
	camera.set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));
	camera.set_translation(glm::vec3(0.0f, 0.0f, -2.5f));

	create_scene();
	create_storage_image();
	create_uniform_buffer();
	create_ray_tracing_pipeline();
	create_shader_binding_table();
	create_descriptor_sets();
	build_command_buffers();
	prepared = true;
	return true;
}

void RaytracingBasic::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void RaytracingBasic::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
	if (camera.updated)
		update_uniform_buffers();
}

std::unique_ptr<vkb::VulkanSample> create_raytracing_basic()
{
	return std::make_unique<RaytracingBasic>();
}