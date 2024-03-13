/*
 * Copyright (c) 2021-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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
 *
 * SPDX-FileCopyrightText: Copyright (c) 2014-2024 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * More complex example for hardware accelerated ray tracing using VK_KHR_ray_tracing_pipeline and VK_KHR_acceleration_structure
 */

#define TINYOBJLOADER_IMPLEMENTATION

#include "ray_tracing_reflection.h"
#include <glm/gtc/type_ptr.hpp>

struct ObjPlane : ObjModelCpu
{
	ObjPlane()
	{
		vertices = {
		    {{+1, 0, +1}, {0, 1, 0}},
		    {{-1, 0, +1}, {0, 1, 0}},
		    {{+1, 0, -1}, {0, 1, 0}},
		    {{-1, 0, -1}, {0, 1, 0}},
		};
		indices   = {0, 1, 2, 1, 2, 3};
		mat_index = {0, 0};
	}
};

struct ObjCube : ObjModelCpu
{
	ObjCube()
	{
		vertices = {
		    {{+0.5f, +0.5f, +0.5f}, {+0.f, +1.f, +0.f}},        // Top
		    {{-0.5f, +0.5f, +0.5f}, {+0.f, +1.f, +0.f}},
		    {{+0.5f, +0.5f, -0.5f}, {+0.f, +1.f, +0.f}},
		    {{-0.5f, +0.5f, -0.5f}, {+0.f, +1.f, +0.f}},
		    {{+0.5f, -0.5f, +0.5f}, {+0.f, -1.f, +0.f}},        // Bottom
		    {{-0.5f, -0.5f, +0.5f}, {+0.f, -1.f, +0.f}},
		    {{+0.5f, -0.5f, -0.5f}, {+0.f, -1.f, +0.f}},
		    {{-0.5f, -0.5f, -0.5f}, {+0.f, -1.f, +0.f}},
		    {{+0.5f, +0.5f, +0.5f}, {+1.f, +0.f, +0.f}},        // Right
		    {{+0.5f, +0.5f, -0.5f}, {+1.f, +0.f, +0.f}},
		    {{+0.5f, -0.5f, -0.5f}, {+1.f, +0.f, +0.f}},
		    {{+0.5f, -0.5f, +0.5f}, {+1.f, +0.f, +0.f}},
		    {{-0.5f, +0.5f, +0.5f}, {-1.f, +0.f, +0.f}},        // left
		    {{-0.5f, +0.5f, -0.5f}, {-1.f, +0.f, +0.f}},
		    {{-0.5f, -0.5f, -0.5f}, {-1.f, +0.f, +0.f}},
		    {{-0.5f, -0.5f, +0.5f}, {-1.f, +0.f, +0.f}},
		    {{-0.5f, +0.5f, +0.5f}, {+0.f, +0.f, +1.f}},        // front
		    {{+0.5f, +0.5f, +0.5f}, {+0.f, +0.f, +1.f}},
		    {{+0.5f, -0.5f, +0.5f}, {+0.f, +0.f, +1.f}},
		    {{-0.5f, -0.5f, +0.5f}, {+0.f, +0.f, +1.f}},
		    {{-0.5f, +0.5f, -0.5f}, {+0.f, +0.f, -1.f}},        // back
		    {{+0.5f, +0.5f, -0.5f}, {+0.f, +0.f, -1.f}},
		    {{+0.5f, -0.5f, -0.5f}, {+0.f, +0.f, -1.f}},
		    {{-0.5f, -0.5f, -0.5f}, {+0.f, +0.f, -1.f}},

		};
		indices = {
		    0, 1, 2, 1, 2, 3,       /*top*/
		    4, 5, 6, 5, 6, 7,       /*bottom*/
		    8, 9, 10, 8, 10, 11,    /*right*/
		    12, 13, 14, 12, 14, 15, /*left*/
		    16, 17, 18, 16, 18, 19, /*front*/
		    20, 21, 22, 20, 22, 23, /*back*/
		};
		mat_index = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5};
	}
};

RaytracingReflection::RaytracingReflection()
{
	title = "Hardware accelerated ray tracing";

	set_api_version(VK_API_VERSION_1_2);

	// Ray tracing related extensions required by this sample
	add_device_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	add_device_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

	// Required by VK_KHR_acceleration_structure
	add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	add_device_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
}

RaytracingReflection::~RaytracingReflection()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		vkDestroyImageView(get_device().get_handle(), storage_image.view, nullptr);
		vkDestroyImage(get_device().get_handle(), storage_image.image, nullptr);
		vkFreeMemory(get_device().get_handle(), storage_image.memory, nullptr);
		delete_acceleration_structure(top_level_acceleration_structure);
		for (auto &b : bottom_level_acceleration_structure)
		{
			delete_acceleration_structure(b);
		}

		for (auto &obj : obj_models)
		{
			obj.vertex_buffer.reset();
			obj.index_buffer.reset();
			obj.mat_color_buffer.reset();
			obj.mat_index_buffer.reset();
		}

		ubo.reset();
	}
}

/*
    Enable extension features required by this sample
    These are passed to device creation via a pNext structure chain
*/
void RaytracingReflection::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// The request is filling with the capabilities (all on by default)
	auto &vulkan12_features = gpu.request_extension_features<VkPhysicalDeviceVulkan12Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
	auto &vulkan11_features = gpu.request_extension_features<VkPhysicalDeviceVulkan11Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES);

	auto &ray_tracing_features            = gpu.request_extension_features<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR);
	auto &acceleration_structure_features = gpu.request_extension_features<VkPhysicalDeviceAccelerationStructureFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR);

	// Enabling all Vulkan features (Int64)
	gpu.get_mutable_requested_features() = gpu.get_features();
}

/*
    Set up a storage image that the ray generation shader will be writing to
*/
void RaytracingReflection::create_storage_image()
{
	storage_image.width  = width;
	storage_image.height = height;

	VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	image.imageType     = VK_IMAGE_TYPE_2D;
	image.format        = VK_FORMAT_B8G8R8A8_UNORM;
	image.extent.width  = storage_image.width;
	image.extent.height = storage_image.height;
	image.extent.depth  = 1;
	image.mipLevels     = 1;
	image.arrayLayers   = 1;
	image.samples       = VK_SAMPLE_COUNT_1_BIT;
	image.tiling        = VK_IMAGE_TILING_OPTIMAL;
	image.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK(vkCreateImage(get_device().get_handle(), &image, nullptr, &storage_image.image));

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(get_device().get_handle(), storage_image.image, &memory_requirements);
	VkMemoryAllocateInfo memory_allocate_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &storage_image.memory));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), storage_image.image, storage_image.memory, 0));

	VkImageViewCreateInfo color_image_view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
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
	vkb::image_layout_transition(command_buffer,
	                             storage_image.image,
	                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	                             {},
	                             {},
	                             VK_IMAGE_LAYOUT_UNDEFINED,
	                             VK_IMAGE_LAYOUT_GENERAL,
	                             {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
	get_device().flush_command_buffer(command_buffer, queue);
}

/*
    Create the bottom level acceleration structure that contains the scene's geometry (triangles)
*/
void RaytracingReflection::create_bottom_level_acceleration_structure(ObjModelGpu &obj_model)
{
	// Note that the buffer usage flags for buffers consumed by the bottom level acceleration structure require special flags
	const VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	// Setup a single transformation matrix that can be used to transform the whole geometry for a single bottom level acceleration structure
	VkTransformMatrixKHR transform_matrix = {
	    1.0f, 0.0f, 0.0f, 0.0f,
	    0.0f, 1.0f, 0.0f, 0.0f,
	    0.0f, 0.0f, 1.0f, 0.0f};
	std::unique_ptr<vkb::core::Buffer> transform_matrix_buffer = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(transform_matrix), buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	transform_matrix_buffer->update(&transform_matrix, sizeof(transform_matrix));

	VkDeviceOrHostAddressConstKHR vertex_data_device_address{};
	VkDeviceOrHostAddressConstKHR index_data_device_address{};
	VkDeviceOrHostAddressConstKHR transform_matrix_device_address{};

	vertex_data_device_address.deviceAddress      = obj_model.vertex_buffer->get_device_address();
	index_data_device_address.deviceAddress       = obj_model.index_buffer->get_device_address();
	transform_matrix_device_address.deviceAddress = transform_matrix_buffer->get_device_address();

	VkAccelerationStructureGeometryTrianglesDataKHR triangles{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
	triangles.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	triangles.vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT;
	triangles.vertexData    = vertex_data_device_address;
	triangles.maxVertex     = obj_model.nb_vertices;
	triangles.vertexStride  = sizeof(ObjVertex);
	triangles.indexType     = VK_INDEX_TYPE_UINT32;
	triangles.indexData     = index_data_device_address;
	triangles.transformData = transform_matrix_device_address;

	// The bottom level acceleration structure contains one set of triangles as the input geometry
	VkAccelerationStructureGeometryKHR acceleration_structure_geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
	acceleration_structure_geometry.geometryType       = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	acceleration_structure_geometry.flags              = VK_GEOMETRY_OPAQUE_BIT_KHR;
	acceleration_structure_geometry.geometry.triangles = triangles;

	// Get the size requirements for buffers involved in the acceleration structure build process
	VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
	acceleration_structure_build_geometry_info.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	acceleration_structure_build_geometry_info.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	acceleration_structure_build_geometry_info.geometryCount = 1;
	acceleration_structure_build_geometry_info.pGeometries   = &acceleration_structure_geometry;

	const uint32_t triangle_count = obj_model.nb_indices / 3;

	VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	vkGetAccelerationStructureBuildSizesKHR(get_device().get_handle(),
	                                        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	                                        &acceleration_structure_build_geometry_info,
	                                        &triangle_count,
	                                        &acceleration_structure_build_sizes_info);

	// Create a buffer to hold the acceleration structure
	AccelerationStructure blas;
	blas.buffer = std::make_unique<vkb::core::Buffer>(get_device(), acceleration_structure_build_sizes_info.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VMA_MEMORY_USAGE_GPU_ONLY);

	// Create the acceleration structure
	VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
	acceleration_structure_create_info.buffer = blas.buffer->get_handle();
	acceleration_structure_create_info.size   = acceleration_structure_build_sizes_info.accelerationStructureSize;
	acceleration_structure_create_info.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	vkCreateAccelerationStructureKHR(get_device().get_handle(), &acceleration_structure_create_info, nullptr, &blas.handle);

	// The actual build process starts here

	// Create a scratch buffer as a temporary storage for the acceleration structure build
	std::unique_ptr<vkb::core::Buffer> sc_buffer;
	sc_buffer = std::make_unique<vkb::core::Buffer>(get_device(), acceleration_structure_build_sizes_info.buildScratchSize,
	                                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                                                VMA_MEMORY_USAGE_CPU_TO_GPU);

	VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
	acceleration_build_geometry_info.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	acceleration_build_geometry_info.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	acceleration_build_geometry_info.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	acceleration_build_geometry_info.dstAccelerationStructure  = blas.handle;
	acceleration_build_geometry_info.geometryCount             = 1;
	acceleration_build_geometry_info.pGeometries               = &acceleration_structure_geometry;
	acceleration_build_geometry_info.scratchData.deviceAddress = sc_buffer->get_device_address();

	VkAccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info;
	acceleration_structure_build_range_info.primitiveCount                                           = triangle_count;
	acceleration_structure_build_range_info.primitiveOffset                                          = 0;
	acceleration_structure_build_range_info.firstVertex                                              = 0;
	acceleration_structure_build_range_info.transformOffset                                          = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR *> acceleration_build_structure_range_infos = {&acceleration_structure_build_range_info};

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vkCmdBuildAccelerationStructuresKHR(command_buffer,
	                                    1,
	                                    &acceleration_build_geometry_info,
	                                    acceleration_build_structure_range_infos.data());
	get_device().flush_command_buffer(command_buffer, queue);

	// delete_scratch_buffer(scratch_buffer);
	sc_buffer.reset();

	// Store the blas to be re-used as instance
	bottom_level_acceleration_structure.push_back(std::move(blas));
}

/*
    Create the top level acceleration structure containing geometry instances of the bottom level acceleration structure(s)
*/
void RaytracingReflection::create_top_level_acceleration_structure(std::vector<VkAccelerationStructureInstanceKHR> &blas_instances)
{
	std::unique_ptr<vkb::core::Buffer> instances_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                                                          sizeof(VkAccelerationStructureInstanceKHR) * blas_instances.size(),
	                                                                                          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	                                                                                          VMA_MEMORY_USAGE_CPU_TO_GPU);
	instances_buffer->update(blas_instances.data(), sizeof(VkAccelerationStructureInstanceKHR) * blas_instances.size());

	VkDeviceOrHostAddressConstKHR instance_data_device_address{};
	instance_data_device_address.deviceAddress = instances_buffer->get_device_address();

	// The top level acceleration structure contains (bottom level) instance as the input geometry
	VkAccelerationStructureGeometryKHR acceleration_structure_geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
	acceleration_structure_geometry.geometryType                       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	acceleration_structure_geometry.flags                              = VK_GEOMETRY_OPAQUE_BIT_KHR;
	acceleration_structure_geometry.geometry.instances.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	acceleration_structure_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
	acceleration_structure_geometry.geometry.instances.data            = instance_data_device_address;

	// Get the size requirements for buffers involved in the acceleration structure build process
	VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
	acceleration_structure_build_geometry_info.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	acceleration_structure_build_geometry_info.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	acceleration_structure_build_geometry_info.geometryCount = 1;
	acceleration_structure_build_geometry_info.pGeometries   = &acceleration_structure_geometry;

	const auto primitive_count = static_cast<uint32_t>(blas_instances.size());

	VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	vkGetAccelerationStructureBuildSizesKHR(
	    get_device().get_handle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	    &acceleration_structure_build_geometry_info,
	    &primitive_count,
	    &acceleration_structure_build_sizes_info);

	// Create a buffer to hold the acceleration structure
	top_level_acceleration_structure.buffer = std::make_unique<vkb::core::Buffer>(
	    get_device(),
	    acceleration_structure_build_sizes_info.accelerationStructureSize,
	    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
	    VMA_MEMORY_USAGE_GPU_ONLY);

	// Create the acceleration structure
	VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
	acceleration_structure_create_info.buffer = top_level_acceleration_structure.buffer->get_handle();
	acceleration_structure_create_info.size   = acceleration_structure_build_sizes_info.accelerationStructureSize;
	acceleration_structure_create_info.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	vkCreateAccelerationStructureKHR(get_device().get_handle(), &acceleration_structure_create_info, nullptr, &top_level_acceleration_structure.handle);

	// The actual build process starts here

	// Create a scratch buffer as a temporary storage for the acceleration structure build
	std::unique_ptr<vkb::core::Buffer> sc_buffer;
	sc_buffer = std::make_unique<vkb::core::Buffer>(get_device(), acceleration_structure_build_sizes_info.buildScratchSize,
	                                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                                                VMA_MEMORY_USAGE_CPU_TO_GPU);

	VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
	acceleration_build_geometry_info.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	acceleration_build_geometry_info.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	acceleration_build_geometry_info.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	acceleration_build_geometry_info.dstAccelerationStructure  = top_level_acceleration_structure.handle;
	acceleration_build_geometry_info.geometryCount             = 1;
	acceleration_build_geometry_info.pGeometries               = &acceleration_structure_geometry;
	acceleration_build_geometry_info.scratchData.deviceAddress = sc_buffer->get_device_address();

	VkAccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info;
	acceleration_structure_build_range_info.primitiveCount                                           = primitive_count;
	acceleration_structure_build_range_info.primitiveOffset                                          = 0;
	acceleration_structure_build_range_info.firstVertex                                              = 0;
	acceleration_structure_build_range_info.transformOffset                                          = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR *> acceleration_build_structure_range_infos = {&acceleration_structure_build_range_info};

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vkCmdBuildAccelerationStructuresKHR(
	    command_buffer,
	    1,
	    &acceleration_build_geometry_info,
	    acceleration_build_structure_range_infos.data());
	get_device().flush_command_buffer(command_buffer, queue);

	// delete_scratch_buffer(scratch_buffer);
	sc_buffer.reset();
}

inline uint32_t aligned_size(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

/*
Create the GPU representation of the model
*/
void RaytracingReflection::create_model(ObjModelCpu &obj, const std::vector<ObjMaterial> &materials)
{
	ObjModelGpu model;
	model.nb_indices  = static_cast<uint32_t>(obj.indices.size());
	model.nb_vertices = static_cast<uint32_t>(obj.vertices.size());

	auto vertex_buffer_size    = obj.vertices.size() * sizeof(ObjVertex);
	auto index_buffer_size     = obj.indices.size() * sizeof(uint32_t);
	auto mat_index_buffer_size = obj.mat_index.size() * sizeof(int32_t);
	auto mat_buffer_size       = materials.size() * sizeof(ObjMaterial);

	// Making sure the material triangle index don't exceed the number of materials
	auto                 max_index = static_cast<int32_t>(materials.size() - 1);
	std::vector<int32_t> mat_index(obj.mat_index.size());
	for (auto i = 0; i < obj.mat_index.size(); i++)
	{
		mat_index[i] = std::min(max_index, obj.mat_index[i]);
	}

	// Note that the buffer usage flags for buffers consumed by the bottom level acceleration structure require special flags
	VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	model.vertex_buffer = std::make_unique<vkb::core::Buffer>(get_device(), vertex_buffer_size, buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	model.vertex_buffer->update(obj.vertices.data(), vertex_buffer_size);

	model.index_buffer = std::make_unique<vkb::core::Buffer>(get_device(), index_buffer_size, buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	model.index_buffer->update(obj.indices.data(), index_buffer_size);

	// Acceleration structure flag is not needed for the rest
	buffer_usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	model.mat_index_buffer = std::make_unique<vkb::core::Buffer>(get_device(), mat_index_buffer_size, buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	model.mat_index_buffer->update(mat_index.data(), mat_index_buffer_size);

	model.mat_color_buffer = std::make_unique<vkb::core::Buffer>(get_device(), mat_buffer_size, buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	model.mat_color_buffer->update(reinterpret_cast<const uint8_t *>(materials.data()), mat_buffer_size);

	obj_models.push_back(std::move(model));
}

auto RaytracingReflection::create_blas_instance(uint32_t blas_id, glm::mat4 &mat)
{
	VkTransformMatrixKHR transform_matrix;
	glm::mat3x4          rtxT = glm::transpose(mat);
	memcpy(&transform_matrix, glm::value_ptr(rtxT), sizeof(VkTransformMatrixKHR));

	AccelerationStructure &blas = bottom_level_acceleration_structure[blas_id];

	// Get the bottom acceleration structure's handle, which will be used during the top level acceleration build
	VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
	acceleration_device_address_info.accelerationStructure = blas.handle;
	auto device_address                                    = vkGetAccelerationStructureDeviceAddressKHR(get_device().get_handle(), &acceleration_device_address_info);

	VkAccelerationStructureInstanceKHR blas_instance{};
	blas_instance.transform                              = transform_matrix;
	blas_instance.instanceCustomIndex                    = blas_id;
	blas_instance.mask                                   = 0xFF;
	blas_instance.instanceShaderBindingTableRecordOffset = 0;
	blas_instance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	blas_instance.accelerationStructureReference         = device_address;

	return blas_instance;
}

/*
    Create a buffer holding the address of model buffers (buffer reference)
*/
void RaytracingReflection::create_buffer_references()
{
	// For each model that was created, we retrieved the address of buffers
	// used by them. So in the shader, we have direct access to the data
	std::vector<ObjBuffers> obj_data;
	auto                    nbObj = static_cast<uint32_t>(obj_models.size());
	for (uint32_t i = 0; i < nbObj; ++i)
	{
		ObjBuffers data;
		data.vertices        = obj_models[i].vertex_buffer->get_device_address();
		data.indices         = obj_models[i].index_buffer->get_device_address();
		data.materials       = obj_models[i].mat_color_buffer->get_device_address();
		data.materialIndices = obj_models[i].mat_index_buffer->get_device_address();
		obj_data.emplace_back(data);
	}
	VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	scene_desc                            = std::make_unique<vkb::core::Buffer>(get_device(), nbObj * sizeof(ObjBuffers), buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	scene_desc->update(obj_data.data(), nbObj * sizeof(ObjBuffers));
}

/*
    Create scene geometry and ray tracing acceleration structures
*/
void RaytracingReflection::create_scene()
{
	// Materials
	ObjMaterial mat_red     = {{1, 0, 0}, {1, 1, 1}, 0.0f};
	ObjMaterial mat_green   = {{0, 1, 0}, {1, 1, 1}, 0.0f};
	ObjMaterial mat_blue    = {{0, 0, 1}, {1, 1, 1}, 0.0f};
	ObjMaterial mat_yellow  = {{1, 1, 0}, {1, 1, 1}, 0.0f};
	ObjMaterial mat_cyan    = {{0, 1, 1}, {1, 1, 1}, 0.0f};
	ObjMaterial mat_magenta = {{1, 0, 1}, {1, 1, 1}, 0.0f};
	ObjMaterial mat_grey    = {{0.7f, 0.7f, 0.7f}, {0.9f, 0.9f, 0.9f}, 0.1f};        // Slightly reflective
	ObjMaterial mat_mirror  = {{0.3f, 0.9f, 1.0f}, {0.9f, 0.9f, 0.9f}, 0.9f};        // Mirror Slightly blue

	// Geometries
	auto cube  = ObjCube();
	auto plane = ObjPlane();

	// Upload geometries to GPU
	create_model(cube, {mat_red, mat_green, mat_blue, mat_yellow, mat_cyan, mat_magenta});        // 6 color faces
	create_model(plane, {mat_grey});
	create_model(cube, {mat_mirror});

	// Create a buffer holding the address of model buffers (buffer reference)
	create_buffer_references();

	// Create as many bottom acceleration structures (blas) as there are geometries/models
	create_bottom_level_acceleration_structure(obj_models[0]);
	create_bottom_level_acceleration_structure(obj_models[1]);
	create_bottom_level_acceleration_structure(obj_models[2]);

	// Matrices to position the instances
	glm::mat4 m_mirror_back  = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, -7.0f)), glm::vec3(5.0f, 5.0f, 0.1f));
	glm::mat4 m_mirror_front = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, 7.0f)), glm::vec3(5.0f, 5.0f, 0.1f));
	glm::mat4 m_plane        = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(0.0f, -1.0f, 0.0f)), glm::vec3(15.0f, 15.0f, 15.0f));
	glm::mat4 m_cube_left    = glm::translate(glm::mat4(1.f), glm::vec3(-1.0f, 0.0f, 0.0f));
	glm::mat4 m_cube_right   = glm::translate(glm::mat4(1.f), glm::vec3(1.0f, 0.0f, 0.0f));

	// Creating instances of the blas to the top level acceleration structure
	std::vector<VkAccelerationStructureInstanceKHR> blas_instances;
	blas_instances.push_back(create_blas_instance(0, m_cube_left));
	blas_instances.push_back(create_blas_instance(0, m_cube_right));
	blas_instances.push_back(create_blas_instance(1, m_plane));
	blas_instances.push_back(create_blas_instance(2, m_mirror_back));
	blas_instances.push_back(create_blas_instance(2, m_mirror_front));

	// Building the TLAS
	create_top_level_acceleration_structure(blas_instances);
}

/*
    Create the Shader Binding Tables that connects the ray tracing pipelines' programs and the  top-level acceleration structure

    SBT Layout used in this sample:

        /-------------\
        | raygen      |
        |-------------|
        | miss        |
        |-------------|
        | miss shadow |
        |-------------|
        | hit         |
        \-------------/
*/

void RaytracingReflection::create_shader_binding_tables()
{
	// Index position of the groups in the generated ray tracing pipeline
	// To be generic, this should be pass in parameters
	std::vector<uint32_t> rgen_index{0};
	std::vector<uint32_t> miss_index{1, 2};
	std::vector<uint32_t> hit_index{3};

	const uint32_t handle_size         = ray_tracing_pipeline_properties.shaderGroupHandleSize;
	const uint32_t handle_alignment    = ray_tracing_pipeline_properties.shaderGroupHandleAlignment;
	const uint32_t handle_size_aligned = aligned_size(handle_size, handle_alignment);

	const VkBufferUsageFlags sbt_buffer_usage_flags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	const VmaMemoryUsage     sbt_memory_usage       = VMA_MEMORY_USAGE_CPU_TO_GPU;

	// Create binding table buffers for each shader type
	raygen_shader_binding_table = std::make_unique<vkb::core::Buffer>(get_device(), handle_size_aligned * rgen_index.size(), sbt_buffer_usage_flags, sbt_memory_usage, 0);
	miss_shader_binding_table   = std::make_unique<vkb::core::Buffer>(get_device(), handle_size_aligned * miss_index.size(), sbt_buffer_usage_flags, sbt_memory_usage, 0);
	hit_shader_binding_table    = std::make_unique<vkb::core::Buffer>(get_device(), handle_size_aligned * hit_index.size(), sbt_buffer_usage_flags, sbt_memory_usage, 0);

	// Copy the pipeline's shader handles into a host buffer
	const auto           group_count = static_cast<uint32_t>(rgen_index.size() + miss_index.size() + hit_index.size());
	const auto           sbt_size    = group_count * handle_size_aligned;
	std::vector<uint8_t> shader_handle_storage(sbt_size);
	VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(get_device().get_handle(), pipeline, 0, group_count, sbt_size, shader_handle_storage.data()));

	// Write the handles in the SBT buffer
	auto copyHandles = [&](auto &buffer, std::vector<uint32_t> &indices, uint32_t stride) {
		auto *pBuffer = static_cast<uint8_t *>(buffer->map());
		for (uint32_t index = 0; index < static_cast<uint32_t>(indices.size()); index++)
		{
			auto *pStart = pBuffer;
			// Copy the handle
			memcpy(pBuffer, shader_handle_storage.data() + (indices[index] * handle_size), handle_size);
			pBuffer = pStart + stride;        // Jumping to next group
		}
		buffer->unmap();
	};

	copyHandles(raygen_shader_binding_table, rgen_index, handle_size_aligned);
	copyHandles(miss_shader_binding_table, miss_index, handle_size_aligned);
	copyHandles(hit_shader_binding_table, hit_index, handle_size_aligned);
}

/*
    Create the descriptor sets used for the ray tracing dispatch
*/
void RaytracingReflection::create_descriptor_sets()
{
	uint32_t nbObj = static_cast<uint32_t>(obj_models.size());

	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
	    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
	    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
	    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
	};
	VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(pool_sizes, 1);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info, &descriptor_set));

	// Setup the descriptor for binding our top level acceleration structure to the ray tracing shaders
	VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
	descriptor_acceleration_structure_info.accelerationStructureCount = 1;
	descriptor_acceleration_structure_info.pAccelerationStructures    = &top_level_acceleration_structure.handle;

	VkWriteDescriptorSet acceleration_structure_write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
	acceleration_structure_write.dstSet          = descriptor_set;
	acceleration_structure_write.dstBinding      = 0;
	acceleration_structure_write.descriptorCount = 1;
	acceleration_structure_write.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	// The acceleration structure descriptor has to be chained via pNext
	acceleration_structure_write.pNext = &descriptor_acceleration_structure_info;

	VkDescriptorImageInfo image_descriptor{};
	image_descriptor.imageView   = storage_image.view;
	image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkDescriptorBufferInfo uniform_descriptor = create_descriptor(*ubo);
	VkDescriptorBufferInfo scene_descriptor   = create_descriptor(*scene_desc);

	VkWriteDescriptorSet result_image_write   = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &image_descriptor);
	VkWriteDescriptorSet uniform_buffer_write = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &uniform_descriptor);
	VkWriteDescriptorSet scene_buffer_write   = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &scene_descriptor);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    acceleration_structure_write,
	    result_image_write,
	    uniform_buffer_write,
	    scene_buffer_write,
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
}

/*
    Create our ray tracing pipeline
*/
void RaytracingReflection::create_ray_tracing_pipeline()
{
	// Slot for binding top level acceleration structures to the ray generation shader
	VkDescriptorSetLayoutBinding acceleration_structure_layout_binding{};
	acceleration_structure_layout_binding.binding         = 0;
	acceleration_structure_layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	acceleration_structure_layout_binding.descriptorCount = 1;
	acceleration_structure_layout_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

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

	// Scene description
	VkDescriptorSetLayoutBinding scene_buffer_binding{};
	scene_buffer_binding.binding         = 3;
	scene_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	scene_buffer_binding.descriptorCount = 1;
	scene_buffer_binding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	std::vector<VkDescriptorSetLayoutBinding> bindings = {
	    acceleration_structure_layout_binding,
	    result_image_layout_binding,
	    uniform_buffer_binding,
	    scene_buffer_binding,
	};

	VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
	layout_info.pBindings    = bindings.data();
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &layout_info, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	pipeline_layout_create_info.setLayoutCount = 1;
	pipeline_layout_create_info.pSetLayouts    = &descriptor_set_layout;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));

	// Ray tracing shaders + buffer reference require SPIR-V 1.5, so we need to set the appropriate target environment for the glslang compiler
	vkb::GLSLCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

	/*
	    Setup ray tracing shader groups
	    Each shader group points at the corresponding shader in the pipeline
	*/
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

	// Ray generation group
	{
		shader_stages.push_back(load_shader("ray_tracing_reflection/raygen.rgen", VK_SHADER_STAGE_RAYGEN_BIT_KHR));
		VkRayTracingShaderGroupCreateInfoKHR raygen_group_ci{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
		raygen_group_ci.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		raygen_group_ci.generalShader      = static_cast<uint32_t>(shader_stages.size()) - 1;
		raygen_group_ci.closestHitShader   = VK_SHADER_UNUSED_KHR;
		raygen_group_ci.anyHitShader       = VK_SHADER_UNUSED_KHR;
		raygen_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
		shader_groups.push_back(raygen_group_ci);
	}

	// Ray miss group
	{
		shader_stages.push_back(load_shader("ray_tracing_reflection/miss.rmiss", VK_SHADER_STAGE_MISS_BIT_KHR));
		VkRayTracingShaderGroupCreateInfoKHR miss_group_ci{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
		miss_group_ci.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		miss_group_ci.generalShader      = static_cast<uint32_t>(shader_stages.size()) - 1;
		miss_group_ci.closestHitShader   = VK_SHADER_UNUSED_KHR;
		miss_group_ci.anyHitShader       = VK_SHADER_UNUSED_KHR;
		miss_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
		shader_groups.push_back(miss_group_ci);
	}

	// Ray miss (shadow) group
	{
		shader_stages.push_back(load_shader("ray_tracing_reflection/missShadow.rmiss", VK_SHADER_STAGE_MISS_BIT_KHR));
		VkRayTracingShaderGroupCreateInfoKHR miss_group_ci{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
		miss_group_ci.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		miss_group_ci.generalShader      = static_cast<uint32_t>(shader_stages.size()) - 1;
		miss_group_ci.closestHitShader   = VK_SHADER_UNUSED_KHR;
		miss_group_ci.anyHitShader       = VK_SHADER_UNUSED_KHR;
		miss_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
		shader_groups.push_back(miss_group_ci);
	}

	// Ray closest hit group
	{
		shader_stages.push_back(load_shader("ray_tracing_reflection/closesthit.rchit", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));
		VkRayTracingShaderGroupCreateInfoKHR closes_hit_group_ci{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
		closes_hit_group_ci.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		closes_hit_group_ci.generalShader      = VK_SHADER_UNUSED_KHR;
		closes_hit_group_ci.closestHitShader   = static_cast<uint32_t>(shader_stages.size()) - 1;
		closes_hit_group_ci.anyHitShader       = VK_SHADER_UNUSED_KHR;
		closes_hit_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
		shader_groups.push_back(closes_hit_group_ci);
	}

	/*
	    Create the ray tracing pipeline
	*/
	VkRayTracingPipelineCreateInfoKHR raytracing_pipeline_create_info{VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
	raytracing_pipeline_create_info.stageCount                   = static_cast<uint32_t>(shader_stages.size());
	raytracing_pipeline_create_info.pStages                      = shader_stages.data();
	raytracing_pipeline_create_info.groupCount                   = static_cast<uint32_t>(shader_groups.size());
	raytracing_pipeline_create_info.pGroups                      = shader_groups.data();
	raytracing_pipeline_create_info.maxPipelineRayRecursionDepth = 2;
	raytracing_pipeline_create_info.layout                       = pipeline_layout;
	VK_CHECK(vkCreateRayTracingPipelinesKHR(get_device().get_handle(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &raytracing_pipeline_create_info, nullptr, &pipeline));
}

/*
    Deletes all resources acquired by an acceleration structure
*/
void RaytracingReflection::delete_acceleration_structure(AccelerationStructure &acceleration_structure)
{
	if (acceleration_structure.buffer)
	{
		acceleration_structure.buffer.reset();
	}

	if (acceleration_structure.handle)
	{
		vkDestroyAccelerationStructureKHR(get_device().get_handle(), acceleration_structure.handle, nullptr);
	}
}

/*
    Create the uniform buffer used to pass matrices to the ray tracing ray generation shader
*/
void RaytracingReflection::create_uniform_buffer()
{
	ubo = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(uniform_data), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	ubo->convert_and_update(uniform_data);
	update_uniform_buffers();
}

/*
    Command buffer generation
*/
void RaytracingReflection::build_command_buffers()
{
	if (width != storage_image.width || height != storage_image.height)
	{
		// If the view port size has changed, we need to recreate the storage image
		vkDestroyImageView(get_device().get_handle(), storage_image.view, nullptr);
		vkDestroyImage(get_device().get_handle(), storage_image.image, nullptr);
		vkFreeMemory(get_device().get_handle(), storage_image.memory, nullptr);
		create_storage_image();

		// The descriptor also needs to be updated to reference the new image
		VkDescriptorImageInfo image_descriptor{};
		image_descriptor.imageView              = storage_image.view;
		image_descriptor.imageLayout            = VK_IMAGE_LAYOUT_GENERAL;
		VkWriteDescriptorSet result_image_write = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &image_descriptor);
		vkUpdateDescriptorSets(get_device().get_handle(), 1, &result_image_write, 0, VK_NULL_HANDLE);
	}

	VkCommandBufferBeginInfo command_buffer_begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

	VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		/*
		    Setup the strided device address regions pointing at the shader identifiers in the shader binding table
		*/

		const uint32_t handle_size_aligned = aligned_size(ray_tracing_pipeline_properties.shaderGroupHandleSize, ray_tracing_pipeline_properties.shaderGroupHandleAlignment);

		VkStridedDeviceAddressRegionKHR raygen_shader_sbt_entry{};
		raygen_shader_sbt_entry.deviceAddress = raygen_shader_binding_table->get_device_address();
		raygen_shader_sbt_entry.stride        = handle_size_aligned;
		raygen_shader_sbt_entry.size          = handle_size_aligned;

		VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry{};
		miss_shader_sbt_entry.deviceAddress = miss_shader_binding_table->get_device_address();
		miss_shader_sbt_entry.stride        = handle_size_aligned;
		miss_shader_sbt_entry.size          = handle_size_aligned * 2;

		VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry{};
		hit_shader_sbt_entry.deviceAddress = hit_shader_binding_table->get_device_address();
		hit_shader_sbt_entry.stride        = handle_size_aligned;
		hit_shader_sbt_entry.size          = handle_size_aligned;

		VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry{};

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
		    Copy ray tracing output to swap chain image
		*/

		// Prepare current swap chain image as transfer destination
		vkb::image_layout_transition(draw_cmd_buffers[i],
		                             get_render_context().get_swapchain().get_images()[i],
		                             VK_IMAGE_LAYOUT_UNDEFINED,
		                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// Prepare ray tracing output image as transfer source
		vkb::image_layout_transition(draw_cmd_buffers[i],
		                             storage_image.image,
		                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		                             VK_PIPELINE_STAGE_TRANSFER_BIT,
		                             {},
		                             VK_ACCESS_TRANSFER_READ_BIT,
		                             VK_IMAGE_LAYOUT_GENERAL,
		                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);

		VkImageCopy copy_region{};
		copy_region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		copy_region.srcOffset      = {0, 0, 0};
		copy_region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		copy_region.dstOffset      = {0, 0, 0};
		copy_region.extent         = {width, height, 1};
		vkCmdCopyImage(draw_cmd_buffers[i], storage_image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		               get_render_context().get_swapchain().get_images()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

		// Transition swap chain image back for presentation
		vkb::image_layout_transition(draw_cmd_buffers[i],
		                             get_render_context().get_swapchain().get_images()[i],
		                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		// Transition ray tracing output image back to general layout
		vkb::image_layout_transition(draw_cmd_buffers[i],
		                             storage_image.image,
		                             VK_PIPELINE_STAGE_TRANSFER_BIT,
		                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		                             VK_ACCESS_TRANSFER_READ_BIT,
		                             {},
		                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		                             VK_IMAGE_LAYOUT_GENERAL,
		                             subresource_range);

		/*
		    Start a new render pass to draw the UI overlay on top of the ray traced image
		*/
		VkClearValue clear_values[2];
		clear_values[0].color        = {{0.0f, 0.0f, 0.033f, 0.0f}};
		clear_values[1].depthStencil = {0.0f, 0};

		VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
		render_pass_begin_info.renderPass               = render_pass;
		render_pass_begin_info.framebuffer              = framebuffers[i];
		render_pass_begin_info.renderArea.extent.width  = width;
		render_pass_begin_info.renderArea.extent.height = height;
		render_pass_begin_info.clearValueCount          = 2;
		render_pass_begin_info.pClearValues             = clear_values;

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		draw_ui(draw_cmd_buffers[i]);
		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void RaytracingReflection::update_uniform_buffers()
{
	auto mat = camera.matrices.perspective;
	mat[1][1] *= -1;        // Flipping Y axis

	uniform_data.proj_inverse = glm::inverse(mat);
	uniform_data.view_inverse = glm::inverse(camera.matrices.view);
	ubo->convert_and_update(uniform_data);
}

bool RaytracingReflection::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	// This sample copies the ray traced output to the swap chain image, so we need to enable the required image usage flags
	const std::set<VkImageUsageFlagBits> image_usage_flags = {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT};
	update_swapchain_image_usage_flags(image_usage_flags);

	// This sample renders the UI overlay on top of the ray tracing output, so we need to disable color attachment clears
	update_render_pass_flags(RenderPassCreateFlags::ColorAttachmentLoad);

	// Get the ray tracing pipeline properties, which we'll need later on in the sample
	VkPhysicalDeviceProperties2 device_properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
	device_properties.pNext = &ray_tracing_pipeline_properties;
	vkGetPhysicalDeviceProperties2(get_device().get_gpu().get_handle(), &device_properties);

	// Get the acceleration structure features, which we'll need later on in the sample
	VkPhysicalDeviceFeatures2 device_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	device_features.pNext = &acceleration_structure_features;
	vkGetPhysicalDeviceFeatures2(get_device().get_gpu().get_handle(), &device_features);

	camera.type = vkb::CameraType::LookAt;
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 512.0f);
	camera.set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));
	camera.set_translation(glm::vec3(0.0f, 0.0f, -2.5f));

	create_storage_image();
	create_scene();
	create_uniform_buffer();
	create_ray_tracing_pipeline();
	create_shader_binding_tables();
	create_descriptor_sets();
	build_command_buffers();
	prepared = true;
	return true;
}

void RaytracingReflection::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void RaytracingReflection::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (camera.updated)
	{
		update_uniform_buffers();
	}
}

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_ray_tracing_reflection()
{
	return std::make_unique<RaytracingReflection>();
}
