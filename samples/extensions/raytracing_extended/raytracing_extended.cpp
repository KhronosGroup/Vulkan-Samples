/* Copyright (c) 2019-2021, Sascha Willems
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
 * Basic example for hardware accelerated ray tracing using VK_KHR_ray_tracing_pipeline and VK_KHR_acceleration_structure
  */

#include "raytracing_extended.h"
#include "gltf_loader.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include <map>

namespace
{
	struct QuickTimer
	{
		using clock = std::chrono::high_resolution_clock;
	    const char *name = "";
		const clock::time_point start;
	    QuickTimer(const char *name) :
	        name(name), start(clock::now())
	    {}

		void stop_and_log()
		{
			using namespace std::chrono;
		    const double dur = duration_cast<microseconds>(clock::now() - start).count();
		    LOGI(fmt::format("{:s} duration: {:f} ms", name, dur / 1000.));
		}

	};
}

#define ASSERT_LOG(cond, msg) \
	{if (!(cond)) { LOGE(msg); throw std::runtime_error(msg); }}

// contains information about the vertex

struct RaytracingExtended::NewVertex
{
	alignas(16) glm::vec4 A; // {x, y, z, nx}
	alignas(16) glm::vec4 B; // {ny, nz, uv0, uv1}
};

struct RaytracingExtended::Model
{
	std::vector<NewVertex>               vertices;
	std::vector<std::array<uint32_t, 3>> triangles;
	VkTransformMatrixKHR                 default_transform;
	uint32_t                             texture_index;
	uint32_t                             object_type;
	Model()
	    : default_transform()
	    , texture_index(0)
	    , object_type(0) {}
};

RaytracingExtended::RaytracingExtended()
    : index_count(0)
    , pipeline(nullptr)
    , pipeline_layout(nullptr)
    , descriptor_set(nullptr)
    , descriptor_set_layout(nullptr)
{
	title = "Ray tracing with extended features";

	// SPIRV 1.4 requires Vulkan 1.1
	set_api_version(VK_API_VERSION_1_1);

	// Ray tracing related extensions required by this sample
	add_device_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	add_device_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

	// Required by VK_KHR_acceleration_structure
	add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	add_device_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	add_device_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

	// Required for VK_KHR_ray_tracing_pipeline
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

	// Required by VK_KHR_spirv_1_4
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
}

RaytracingExtended::~RaytracingExtended()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		vkDestroyImageView(get_device().get_handle(), storage_image.view, nullptr);
		vkDestroyImage(get_device().get_handle(), storage_image.image, nullptr);
		vkFreeMemory(get_device().get_handle(), storage_image.memory, nullptr);
		delete_acceleration_structure(top_level_acceleration_structure);
		raytracing_scene.reset();
		vertex_buffer.reset();
		index_buffer.reset();
		ubo.reset();
	}
}

void RaytracingExtended::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Enable extension features required by this sample
	// These are passed to device creation via a pNext structure chain
	auto &requested_buffer_device_address_features                  = gpu.request_extension_features<VkPhysicalDeviceBufferDeviceAddressFeatures>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES);
	requested_buffer_device_address_features.bufferDeviceAddress    = VK_TRUE;
	auto &requested_ray_tracing_features                            = gpu.request_extension_features<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR);
	requested_ray_tracing_features.rayTracingPipeline               = VK_TRUE;
	auto &requested_acceleration_structure_features                 = gpu.request_extension_features<VkPhysicalDeviceAccelerationStructureFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR);
	requested_acceleration_structure_features.accelerationStructure = VK_TRUE;
}

/*
	Set up a storage image that the ray generation shader will be writing to
*/
void RaytracingExtended::create_storage_image()
{
	storage_image.width  = width;
	storage_image.height = height;

	VkImageCreateInfo image = vkb::initializers::image_create_info();
	image.imageType         = VK_IMAGE_TYPE_2D;
	image.format            = VK_FORMAT_B8G8R8A8_UNORM;
	image.extent.width      = storage_image.width;
	image.extent.height     = storage_image.height;
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
	Gets the device address from a buffer that's needed in many places during the ray tracing setup
*/
uint64_t RaytracingExtended::get_buffer_device_address(VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
	buffer_device_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	buffer_device_address_info.buffer = buffer;
	return vkGetBufferDeviceAddressKHR(device->get_handle(), &buffer_device_address_info);
}



void RaytracingExtended::create_static_object_buffers()
{
	QuickTimer timer{"Static object creation"};
	assert(!!raytracing_scene);
	auto &models       = raytracing_scene->models;
	auto &modelBuffers = raytracing_scene->modelBuffers;
	modelBuffers.resize(0);

	std::vector<uint32_t>          vertex_buffer_offsets(models.size()), index_buffer_offsets(models.size());
	uint32_t                       nTotalVertices = 0, nTotalTriangles = 0;
	std::vector<SceneInstanceData> model_indices(models.size());
	for (size_t i = 0; i < models.size(); ++i)
	{
		model_indices[i]         = {uint32_t(nTotalVertices), uint32_t(nTotalTriangles), uint32_t(models[i].texture_index), uint32_t(models[i].object_type)};
		vertex_buffer_offsets[i] = nTotalVertices * sizeof(NewVertex);
		nTotalVertices += models[i].vertices.size();

		index_buffer_offsets[i] = nTotalTriangles * sizeof(models[i].triangles[0]);
		nTotalTriangles += models[i].triangles.size();
	}

	data_to_model_buffer = std::make_unique<vkb::core::Buffer>(get_device(), model_indices.size() * sizeof(model_indices[0]), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	data_to_model_buffer->update(model_indices.data(), model_indices.size() * sizeof(model_indices[0]), 0);

	//uint32_t firstVertex = 0, primitiveOffset = 0;
	auto vertex_buffer_size = nTotalVertices * sizeof(NewVertex);
	auto index_buffer_size  = nTotalTriangles * models[0].triangles.size();

	// Create a staging buffer. (If staging buffer use is disabled, then this will be the final buffer)
	std::unique_ptr<vkb::core::Buffer> staging_vertex_buffer = nullptr, staging_index_buffer = nullptr;
	const VkBufferUsageFlags           buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	const VkBufferUsageFlags           staging_flags      = scene_options.use_vertex_staging_buffer ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : buffer_usage_flags;
	staging_vertex_buffer                                 = std::make_unique<vkb::core::Buffer>(get_device(), vertex_buffer_size, staging_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	staging_index_buffer                                  = std::make_unique<vkb::core::Buffer>(get_device(), index_buffer_size, staging_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Copy over the data for each of the models
	for (size_t i = 0; i < models.size(); ++i)
	{
		auto &model = models[i];
		staging_vertex_buffer->update(model.vertices.data(), model.vertices.size() * sizeof(model.vertices[0]), vertex_buffer_offsets[i]);
		staging_index_buffer->update(model.triangles.data(), model.triangles.size() * sizeof(model.triangles[0]), index_buffer_offsets[i]);
	}

	// now transfer over to the end buffer
	if (scene_options.use_vertex_staging_buffer)
	{
		auto &cmd = device->request_command_buffer();
		cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_NULL_HANDLE);
		auto copy = [this, &cmd](vkb::core::Buffer &staging_buffer) {
			auto output_buffer = std::make_unique<vkb::core::Buffer>(get_device(), staging_buffer.get_size(), buffer_usage_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
			cmd.copy_buffer(staging_buffer, *output_buffer, staging_buffer.get_size());

			vkb::BufferMemoryBarrier barrier;
			barrier.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
			barrier.dst_stage_mask  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			cmd.buffer_memory_barrier(*output_buffer, 0, VK_WHOLE_SIZE, barrier);
			return output_buffer;
		};
		vertex_buffer = copy(*staging_vertex_buffer);
		index_buffer  = copy(*staging_index_buffer);

		cmd.end();
		auto &queue = device->get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
		queue.submit(cmd, device->request_fence());
        device->get_fence_pool().wait();

	}
	else
	{
		vertex_buffer = std::move(staging_vertex_buffer);
		index_buffer  = std::move(staging_index_buffer);
	}


	for (size_t i = 0; i < models.size(); ++i)
	{
		ModelBuffer buffer;
		buffer.vertex_offset  = vertex_buffer_offsets[i];
		buffer.index_offset   = index_buffer_offsets[i];
		buffer.is_static      = true;
		buffer.default_transform = models[i].default_transform;
		buffer.num_vertices      = models[i].vertices.size();
		buffer.num_triangles      = models[i].triangles.size();
		buffer.object_type        = 0;
		modelBuffers.emplace_back(std::move(buffer));
	}
	timer.stop_and_log();
}

/*
	Create the bottom level acceleration structure that contains the scene's geometry (triangles)
*/
void RaytracingExtended::create_bottom_level_acceleration_structure()
{
	QuickTimer timer{"BLAS Build"};
	assert(!!raytracing_scene);
	/**
	Though we use similar code to handle static and dynamic objects, several parts differ:
	1. Static / dynamic objects have different buffers (device-only vs host-visible)
	2. Dynamic objects use different flags (i.e. for fast rebuilds)
	*/

	assert(!!vertex_buffer && !!index_buffer);
	const uint64_t static_vertex_handle = get_buffer_device_address(vertex_buffer->get_handle()),
	               static_index_handle  = get_buffer_device_address(index_buffer->get_handle()),
	               dynamic_vertex_handle = dynamic_vertex_buffer ? get_buffer_device_address(dynamic_vertex_buffer->get_handle()) : 0,
	               dynamic_index_handle  = dynamic_index_buffer ? get_buffer_device_address(dynamic_index_buffer->get_handle()) : 0;
	for (auto &&modelBuffer : raytracing_scene->modelBuffers)
	{
		const VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

		// Setup a single transformation matrix that can be used to transform the whole geometry for a single bottom level acceleration structure
		VkTransformMatrixKHR transform_matrix = modelBuffer.default_transform;
		modelBuffer.transform_matrix_buffer = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(transform_matrix), buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
		modelBuffer.transform_matrix_buffer->update(&transform_matrix, sizeof(transform_matrix));

		VkDeviceOrHostAddressConstKHR vertex_data_device_address{};
		VkDeviceOrHostAddressConstKHR index_data_device_address{};
		VkDeviceOrHostAddressConstKHR transform_matrix_device_address{};

		vertex_data_device_address.deviceAddress      =  modelBuffer.vertex_offset + (modelBuffer.is_static ? static_vertex_handle : dynamic_vertex_handle);
		index_data_device_address.deviceAddress = modelBuffer.index_offset + (modelBuffer.is_static ? static_index_handle : dynamic_index_handle);
		transform_matrix_device_address.deviceAddress = get_buffer_device_address(modelBuffer.transform_matrix_buffer->get_handle());

		// The bottom level acceleration structure contains one set of triangles as the input geometry
		auto &acceleration_structure_geometry                            = modelBuffer.acceleration_structure_geometry;
		acceleration_structure_geometry.sType                            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		acceleration_structure_geometry.pNext                            = nullptr;
		acceleration_structure_geometry.geometryType                     = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		acceleration_structure_geometry.flags                            = VK_GEOMETRY_OPAQUE_BIT_KHR;
		acceleration_structure_geometry.geometry.triangles               = {};
		acceleration_structure_geometry.geometry.triangles.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		acceleration_structure_geometry.geometry.triangles.pNext         = nullptr;
		acceleration_structure_geometry.geometry.triangles.vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT;
		acceleration_structure_geometry.geometry.triangles.vertexData    = vertex_data_device_address;
		acceleration_structure_geometry.geometry.triangles.maxVertex     = modelBuffer.num_vertices;
		acceleration_structure_geometry.geometry.triangles.vertexStride  = sizeof(NewVertex);
		acceleration_structure_geometry.geometry.triangles.indexType     = VK_INDEX_TYPE_UINT32;
		acceleration_structure_geometry.geometry.triangles.indexData     = index_data_device_address;
		acceleration_structure_geometry.geometry.triangles.transformData = transform_matrix_device_address;

		modelBuffer.buildRangeInfo                              = {};
		auto &acceleration_structure_build_range_info           = modelBuffer.buildRangeInfo;
		acceleration_structure_build_range_info.primitiveCount  = modelBuffer.num_triangles;
		acceleration_structure_build_range_info.primitiveOffset = 0;        //primitiveOffset;
		acceleration_structure_build_range_info.firstVertex     = 0;        //firstVertex;
		acceleration_structure_build_range_info.transformOffset = 0;


		// now create BLAS
		VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
		acceleration_structure_build_geometry_info.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		acceleration_structure_build_geometry_info.pNext         = nullptr;
		acceleration_structure_build_geometry_info.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		acceleration_structure_build_geometry_info.flags         = modelBuffer.is_static ? VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR : VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
		acceleration_structure_build_geometry_info.geometryCount = 1;
		acceleration_structure_build_geometry_info.pGeometries   = &acceleration_structure_geometry;

		uint32_t primitive_count = modelBuffer.num_triangles;

		auto &acceleration_structure_build_sizes_info = modelBuffer.buildSize;
		acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR(
		    device->get_handle(),
		    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		    &acceleration_structure_build_geometry_info,
		    &primitive_count,
		    &acceleration_structure_build_sizes_info);
	}

	auto &                                                  modelBuffers = raytracing_scene->modelBuffers;
	std::vector<VkAccelerationStructureGeometryKHR>         geometries;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR *> rangeInfos;
	for (auto & modelBuffer : modelBuffers)
	{
		geometries.push_back(modelBuffer.acceleration_structure_geometry);
		rangeInfos.push_back(&modelBuffer.buildRangeInfo);
	}

	for (size_t i = 0; i < modelBuffers.size(); ++i)
	{
		// Create a buffer to hold the acceleration structure
		auto &bottom_level_acceleration_structure  = modelBuffers[i].bottom_level_acceleration_structure;
		bottom_level_acceleration_structure.buffer = std::make_unique<vkb::core::Buffer>(
		    get_device(),
		    modelBuffers[i].buildSize.accelerationStructureSize,
		    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		    VMA_MEMORY_USAGE_GPU_ONLY);

		// Create the acceleration structure
		VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
		acceleration_structure_create_info.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		acceleration_structure_create_info.buffer = bottom_level_acceleration_structure.buffer->get_handle();
		acceleration_structure_create_info.size   = modelBuffers[i].buildSize.accelerationStructureSize;
		acceleration_structure_create_info.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		vkCreateAccelerationStructureKHR(device->get_handle(), &acceleration_structure_create_info, nullptr, &bottom_level_acceleration_structure.handle);

		// The actual build process starts here

		// Create a scratch buffer as a temporary storage for the acceleration structure build
		auto          scratch_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
                                                                  modelBuffers[i].buildSize.buildScratchSize,
                                                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                                  VMA_MEMORY_USAGE_CPU_TO_GPU);
		size_t                currentOffset  = 0;
		{
			VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
			acceleration_build_geometry_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			acceleration_build_geometry_info.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			acceleration_build_geometry_info.flags                     = modelBuffers[i].is_static ? VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR : VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
			acceleration_build_geometry_info.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
			acceleration_build_geometry_info.dstAccelerationStructure  = bottom_level_acceleration_structure.handle;
			acceleration_build_geometry_info.geometryCount             = 1;
			acceleration_build_geometry_info.pGeometries               = &geometries[i];
			acceleration_build_geometry_info.scratchData.deviceAddress = scratch_buffer->get_device_address() + currentOffset;

			// Build the acceleration structure on the device via a one-time command buffer submission
			// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
			VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			// = {&modelBuffer.buildRangeInfo};
			vkCmdBuildAccelerationStructuresKHR(
			    command_buffer,
			    1,
			    &acceleration_build_geometry_info,
			    &rangeInfos[i]);
			get_device().flush_command_buffer(command_buffer, queue);

			// currentOffset += modelBuffers[i].buildSize.buildScratchSize; //NB: Due to line 410 not being outside for loop, this would never get used.
		}

		scratch_buffer.reset();

		// Get the bottom acceleration structure's handle, which will be used during the top level acceleration build
		VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
		acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		acceleration_device_address_info.accelerationStructure = bottom_level_acceleration_structure.handle;
		bottom_level_acceleration_structure.device_address     = vkGetAccelerationStructureDeviceAddressKHR(device->get_handle(), &acceleration_device_address_info);
	}
	timer.stop_and_log();
}


/*
	Create the top level acceleration structure containing geometry instances of the bottom level acceleration structure(s)
*/
void RaytracingExtended::create_top_level_acceleration_structure()
{
	/*
	Often, good performance can be obtained when the TLAS uses PREFER_FAST_TRACE with full rebuilds.
	*/
	QuickTimer timer{"TLAS Build"};
	assert(!!raytracing_scene);
	VkTransformMatrixKHR transform_matrix = {
	    1.0f, 0.0f, 0.0f, 0.0f,
	    0.0f, 1.0f, 0.0f, 0.0f,
	    0.0f, 0.0f, 1.0f, 0.0f};

	// create a vector
	std::vector<VkAccelerationStructureInstanceKHR> instances;
	for (size_t i = 0; i < raytracing_scene->modelBuffers.size(); ++i)
	{
		auto &modelBuffer                                                      = raytracing_scene->modelBuffers[i];
		VkAccelerationStructureInstanceKHR acceleration_structure_instance{};
		acceleration_structure_instance.transform                              = transform_matrix;
		acceleration_structure_instance.instanceCustomIndex                    = i;
		acceleration_structure_instance.mask                                   = 0xFF;
		acceleration_structure_instance.instanceShaderBindingTableRecordOffset = 0;
		acceleration_structure_instance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		acceleration_structure_instance.accelerationStructureReference         = modelBuffer.bottom_level_acceleration_structure.device_address;
		instances.emplace_back(acceleration_structure_instance);
	}

	const size_t                       instancesDataSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();
	std::unique_ptr<vkb::core::Buffer> instances_buffer  = std::make_unique<vkb::core::Buffer>(get_device(),
                                                                                              instancesDataSize,
                                                                                              VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                                                              VMA_MEMORY_USAGE_CPU_TO_GPU);
	instances_buffer->update(instances.data(), instancesDataSize);

	VkDeviceOrHostAddressConstKHR instance_data_device_address{};
	instance_data_device_address.deviceAddress = get_buffer_device_address(instances_buffer->get_handle());

	// The top level acceleration structure contains (bottom level) instance as the input geometry
	VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
	acceleration_structure_geometry.sType                              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	acceleration_structure_geometry.geometryType                       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	acceleration_structure_geometry.flags                              = VK_GEOMETRY_OPAQUE_BIT_KHR;
	acceleration_structure_geometry.geometry.instances                 = {};
	acceleration_structure_geometry.geometry.instances.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	acceleration_structure_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
	acceleration_structure_geometry.geometry.instances.data            = instance_data_device_address;

	// Get the size requirements for buffers involved in the acceleration structure build process
	VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
	acceleration_structure_build_geometry_info.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	acceleration_structure_build_geometry_info.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	acceleration_structure_build_geometry_info.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	acceleration_structure_build_geometry_info.geometryCount = 1;
	acceleration_structure_build_geometry_info.pGeometries   = &acceleration_structure_geometry;

	const uint32_t primitive_count = instances.size();

	VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{};
	acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
	    device->get_handle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
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
	VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
	acceleration_structure_create_info.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	acceleration_structure_create_info.buffer = top_level_acceleration_structure.buffer->get_handle();
	acceleration_structure_create_info.size   = acceleration_structure_build_sizes_info.accelerationStructureSize;
	acceleration_structure_create_info.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	vkCreateAccelerationStructureKHR(device->get_handle(), &acceleration_structure_create_info, nullptr, &top_level_acceleration_structure.handle);

	// The actual build process starts here

	// Create a scratch buffer as a temporary storage for the acceleration structure build
	auto          scratch_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
                                                              acceleration_structure_build_sizes_info.buildScratchSize,
                                                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                              VMA_MEMORY_USAGE_CPU_TO_GPU);

	VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
	acceleration_build_geometry_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	acceleration_build_geometry_info.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	acceleration_build_geometry_info.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	acceleration_build_geometry_info.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	acceleration_build_geometry_info.dstAccelerationStructure  = top_level_acceleration_structure.handle;
	acceleration_build_geometry_info.geometryCount             = 1;
	acceleration_build_geometry_info.pGeometries               = &acceleration_structure_geometry;
	acceleration_build_geometry_info.scratchData.deviceAddress = scratch_buffer->get_device_address();

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

	scratch_buffer.reset();

	// Get the top acceleration structure's handle, which will be used to setup it's descriptor
	VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
	acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	acceleration_device_address_info.accelerationStructure = top_level_acceleration_structure.handle;
	top_level_acceleration_structure.device_address        = vkGetAccelerationStructureDeviceAddressKHR(device->get_handle(), &acceleration_device_address_info);
	timer.stop_and_log();
}

inline uint32_t aligned_size(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}


namespace
{
template <typename T>
struct CopyBuffer
{
	std::vector<T> operator()(std::unordered_map<std::string, vkb::core::Buffer> &buffers, const char *bufferName)
	{
		auto iter = buffers.find(bufferName);
		if (iter == buffers.cend())
		{
			return {};
		}
		auto &         buffer = iter->second;
		std::vector<T> out;

		const size_t sz = buffer.get_size();
		out.resize(sz / sizeof(T));
		const bool alreadyMapped = !!buffer.get_data();
		if (!alreadyMapped)
		{
			buffer.map();
		}
		memcpy(&out[0], buffer.get_data(), sz);
		if (!alreadyMapped)
		{
			buffer.unmap();
		}
		return out;
	}
};
}        // namespace

/*
	Create scene geometry and ray tracing acceleration structures
*/
void RaytracingExtended::create_scene()
{
	std::vector<SceneLoadInfo> scenesToLoad;
	const float sponza_scale = 0.01f;
	const glm::mat4x4          sponza_transform{0.f, 0.f, sponza_scale, 0.f,
                                       sponza_scale, 0.f, 0.f, 0.f,
                                       0.f, sponza_scale, 0.f, 0.f,
										0.f, 0.f, 0.f, 1.f};
	scenesToLoad.emplace_back("scenes/sponza/Sponza01.gltf", sponza_transform, ObjectType::OBJECT_NORMAL);
	raytracing_scene = std::make_unique<RaytracingScene>(*device, std::move(scenesToLoad));
	create_static_object_buffers();
	create_dynamic_object_buffers();
	create_bottom_level_acceleration_structure();
	create_top_level_acceleration_structure();
}

/*
	Create the Shader Binding Tables that connects the ray tracing pipelines' programs and the  top-level acceleration structure

	SBT Layout used in this sample:

		/-----------\
		| raygen    |
		|-----------|
		| miss      |
		|-----------|
		| hit       |
		\-----------/
*/

void RaytracingExtended::create_shader_binding_tables()
{
	const uint32_t           handle_size             = ray_tracing_pipeline_properties.shaderGroupHandleSize;
	const uint32_t           handle_size_aligned     = aligned_size(ray_tracing_pipeline_properties.shaderGroupHandleSize, ray_tracing_pipeline_properties.shaderGroupHandleAlignment);
	auto           group_count             = static_cast<uint32_t>(shader_groups.size());
	const uint32_t           sbt_size                = group_count * handle_size_aligned;
	const VkBufferUsageFlags sbt_buffer_usage_flags  = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	const VmaMemoryUsage     sbt_memory_usage        = VMA_MEMORY_USAGE_CPU_TO_GPU;

	// Raygen
	// Create binding table buffers for each shader type
	raygen_shader_binding_table = std::make_unique<vkb::core::Buffer>(get_device(), handle_size, sbt_buffer_usage_flags, sbt_memory_usage, 0);
	miss_shader_binding_table   = std::make_unique<vkb::core::Buffer>(get_device(), handle_size, sbt_buffer_usage_flags, sbt_memory_usage, 0);
	hit_shader_binding_table    = std::make_unique<vkb::core::Buffer>(get_device(), handle_size, sbt_buffer_usage_flags, sbt_memory_usage, 0);

	// Copy the pipeline's shader handles into a host buffer
	std::vector<uint8_t> shader_handle_storage(sbt_size);
	VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(get_device().get_handle(), pipeline, 0, group_count, sbt_size, shader_handle_storage.data()));

	// Copy the shader handles from the host buffer to the binding tables
	auto *data = static_cast<uint8_t *>(raygen_shader_binding_table->map());
	memcpy(data, shader_handle_storage.data(), handle_size);
	data = static_cast<uint8_t *>(miss_shader_binding_table->map());
	memcpy(data, shader_handle_storage.data() + handle_size_aligned, handle_size);
	data = static_cast<uint8_t *>(hit_shader_binding_table->map());
	memcpy(data, shader_handle_storage.data() + handle_size_aligned * 2, handle_size);
	raygen_shader_binding_table->unmap();
	miss_shader_binding_table->unmap();
	hit_shader_binding_table->unmap();
}

/*
	Create the descriptor sets used for the ray tracing dispatch
*/
void RaytracingExtended::create_descriptor_sets()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
	    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
	    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};
	VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(pool_sizes, 1);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info, &descriptor_set));

	// Setup the descriptor for binding our top level acceleration structure to the ray tracing shaders
	VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{};
	descriptor_acceleration_structure_info.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptor_acceleration_structure_info.accelerationStructureCount = 1;
	descriptor_acceleration_structure_info.pAccelerationStructures    = &top_level_acceleration_structure.handle;

	VkWriteDescriptorSet acceleration_structure_write{};
	acceleration_structure_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	acceleration_structure_write.dstSet          = descriptor_set;
	acceleration_structure_write.dstBinding      = 0;
	acceleration_structure_write.descriptorCount = 1;
	acceleration_structure_write.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	// The acceleration structure descriptor has to be chained via pNext
	acceleration_structure_write.pNext = &descriptor_acceleration_structure_info;

	VkDescriptorImageInfo image_descriptor{};
	image_descriptor.imageView   = storage_image.view;
	image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*ubo);
	VkDescriptorBufferInfo vertex_descriptor          = create_descriptor(*vertex_buffer);
	VkDescriptorBufferInfo index_descriptor           = create_descriptor(*index_buffer);
	VkDescriptorBufferInfo data_map_descriptor        = create_descriptor(*data_to_model_buffer);

	VkWriteDescriptorSet result_image_write   = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &image_descriptor);
	VkWriteDescriptorSet uniform_buffer_write = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &buffer_descriptor);
	VkWriteDescriptorSet vertex_buffer_write  = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, &vertex_descriptor);
	VkWriteDescriptorSet index_buffer_write   = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5, &index_descriptor);
	VkWriteDescriptorSet data_map_write   = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6, &data_map_descriptor);
	VkWriteDescriptorSet texture_array_write  = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7, raytracing_scene->imageInfos.data(), raytracing_scene->imageInfos.size());

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    acceleration_structure_write,
	    result_image_write,
	    uniform_buffer_write,
		vertex_buffer_write,
		index_buffer_write,
	    data_map_write,
	    texture_array_write 
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
}

void RaytracingExtended::create_dynamic_object_buffers()
{
	std::vector<glm::vec3> pts = { {0.f, 0.f, 0.f},
		                           {1.f, 0.f, 0.f},
		                           {1.f, 1.f, 0.f},
		                           {0.f, 1.f, 0.f} };
	std::vector<std::array<uint32_t, 3>> indices = { {0, 1, 2},
		                                {0, 2, 3} };
	glm::vec3               translation = {0, 1, 0};
	for (auto&& pt : pts)
	{
		pt += translation;
	}

	std::vector<NewVertex> vertices_out(pts.size());
	for (size_t i = 0; i < pts.size(); ++i)
	{
		NewVertex vertex;
		vertex.A = {pts[i].x, pts[i].y, pts[i].z, 0.f};
		vertex.B = { 0.f, 1.f, 0.f, 0.f };
		vertices_out[i] = vertex;
	}

	size_t vertex_buffer_size = vertices_out.size() * sizeof(NewVertex);
	size_t index_buffer_size  = indices.size() * sizeof(indices[0]);

	if (!dynamic_vertex_buffer || !dynamic_index_buffer)
	{
		// note this flags are different because they will be read/write, in contrast to static
		dynamic_vertex_buffer = std::make_unique<vkb::core::Buffer>(get_device(), vertex_buffer_size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		dynamic_index_buffer  = std::make_unique<vkb::core::Buffer>(get_device(), index_buffer_size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

	dynamic_vertex_buffer->update(vertices_out.data(), vertex_buffer_size);
	dynamic_index_buffer->update(indices.data(), index_buffer_size);

	ModelBuffer buffer;
	buffer.vertex_offset     = 0;
	buffer.index_offset      = 0;
	buffer.is_static         = false;
	buffer.default_transform = VkTransformMatrixKHR{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
	buffer.num_vertices      = vertices_out.size();
	buffer.num_triangles     = indices.size();
	buffer.object_type       = ObjectType::OBJECT_FLAME;
	raytracing_scene->modelBuffers.emplace_back(std::move(buffer));
}

/*
	Create our ray tracing pipeline
*/
void RaytracingExtended::create_ray_tracing_pipeline()
{
	// Slot for binding top level acceleration structures to the ray generation shader
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

	// Pass render mode constant
	struct SpecialConsts_s {
		uint32_t renderMode = RenderMode::RENDER_DEFAULT;
		uint32_t maxRays = 12;
	}specialConsts;
	std::vector<VkSpecializationMapEntry> specializationMapEntries;
	specializationMapEntries.push_back(vkb::initializers::specialization_map_entry(0, offsetof(SpecialConsts_s, renderMode), sizeof(uint32_t)));
	specializationMapEntries.push_back(vkb::initializers::specialization_map_entry(1, offsetof(SpecialConsts_s, maxRays), sizeof(uint32_t)));
	VkSpecializationInfo specializationInfo = vkb::initializers::specialization_info(
		specializationMapEntries.size(), &specializationMapEntries.front(), sizeof(SpecialConsts_s), &specialConsts);

	VkDescriptorSetLayoutBinding vertex_binding{};
	vertex_binding.binding                         = 4;
	vertex_binding.descriptorType                  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	vertex_binding.descriptorCount                 = 1;
	vertex_binding.stageFlags                      = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	VkDescriptorSetLayoutBinding index_binding{};
	index_binding.binding                          = 5;
	index_binding.descriptorType                   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	index_binding.descriptorCount                  = 1;
	index_binding.stageFlags                       = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	VkDescriptorSetLayoutBinding data_map_binding{};
	data_map_binding.binding      = 6;
	data_map_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	data_map_binding.descriptorCount = 1;
	data_map_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	VkDescriptorSetLayoutBinding texture_array_binding{};
	texture_array_binding.binding    = 7;
	texture_array_binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texture_array_binding.descriptorCount = raytracing_scene->imageInfos.size();
	texture_array_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	std::vector<VkDescriptorSetLayoutBinding> bindings = {
	    acceleration_structure_layout_binding,
	    result_image_layout_binding,
	    uniform_buffer_binding,
		vertex_binding,
		index_binding,
	    data_map_binding,
		texture_array_binding
	};

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

	// Ray tracing shaders require SPIR-V 1.4, so we need to set the appropriate target environment for the GLSLang compiler
	vkb::GLSLCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);

	/*
		Setup ray tracing shader groups
		Each shader group points at the corresponding shader in the pipeline
	*/
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

	// Ray generation group
	{
		shader_stages.push_back(load_shader("khr_ray_tracing_extended/raygen.rgen", VK_SHADER_STAGE_RAYGEN_BIT_KHR));
		shader_stages.back().pSpecializationInfo = &specializationInfo;
		VkRayTracingShaderGroupCreateInfoKHR raygen_group_ci{};
		raygen_group_ci.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		raygen_group_ci.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		raygen_group_ci.generalShader      = static_cast<uint32_t>(shader_stages.size()) - 1;
		raygen_group_ci.closestHitShader   = VK_SHADER_UNUSED_KHR;
		raygen_group_ci.anyHitShader       = VK_SHADER_UNUSED_KHR;
		raygen_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
		shader_groups.push_back(raygen_group_ci);
	}

	// Ray miss group
	{
		shader_stages.push_back(load_shader("khr_ray_tracing_extended/miss.rmiss", VK_SHADER_STAGE_MISS_BIT_KHR));
		VkRayTracingShaderGroupCreateInfoKHR miss_group_ci{};
		miss_group_ci.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		miss_group_ci.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		miss_group_ci.generalShader      = static_cast<uint32_t>(shader_stages.size()) - 1;
		miss_group_ci.closestHitShader   = VK_SHADER_UNUSED_KHR;
		miss_group_ci.anyHitShader       = VK_SHADER_UNUSED_KHR;
		miss_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
		shader_groups.push_back(miss_group_ci);
	}

	// Ray closest hit group
	{
		shader_stages.push_back(load_shader("khr_ray_tracing_extended/closesthit.rchit", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));
		shader_stages.back().pSpecializationInfo = &specializationInfo;
		VkRayTracingShaderGroupCreateInfoKHR closes_hit_group_ci{};
		closes_hit_group_ci.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
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
	VkRayTracingPipelineCreateInfoKHR raytracing_pipeline_create_info{};
	raytracing_pipeline_create_info.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	raytracing_pipeline_create_info.stageCount                   = static_cast<uint32_t>(shader_stages.size());
	raytracing_pipeline_create_info.pStages                      = shader_stages.data();
	raytracing_pipeline_create_info.groupCount                   = static_cast<uint32_t>(shader_groups.size());
	raytracing_pipeline_create_info.pGroups                      = shader_groups.data();
	raytracing_pipeline_create_info.maxPipelineRayRecursionDepth = 1;
	raytracing_pipeline_create_info.layout                       = pipeline_layout;
	VK_CHECK(vkCreateRayTracingPipelinesKHR(get_device().get_handle(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &raytracing_pipeline_create_info, nullptr, &pipeline));
}

void RaytracingExtended::create_display_pipeline()
{
}

/*
	Deletes all resources acquired by an acceleration structure
*/
void RaytracingExtended::delete_acceleration_structure(AccelerationStructureExtended &acceleration_structure)
{
	if (acceleration_structure.buffer)
	{
		acceleration_structure.buffer.reset();
	}
	if (acceleration_structure.handle)
	{
		vkDestroyAccelerationStructureKHR(device->get_handle(), acceleration_structure.handle, nullptr);
	}
}

/*
	Create the uniform buffer used to pass matrices to the ray tracing ray generation shader
*/
void RaytracingExtended::create_uniform_buffer()
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
void RaytracingExtended::build_command_buffers()
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
		build_command_buffers();
	}

	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();


	auto device_ptr = device->get_handle();
	auto command_pool = device->get_command_pool().get_handle();
	if (!raytracing_command_buffers.empty())
	{
		vkFreeCommandBuffers(device_ptr, command_pool, raytracing_command_buffers.size(), &raytracing_command_buffers[0]);
		raytracing_command_buffers.resize(0);
	}

	raytracing_command_buffers.resize(draw_cmd_buffers.size());
	for (auto&& command_buffer : raytracing_command_buffers)
	{
		command_buffer = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
	}

	for (auto & raytracing_command_buffer : raytracing_command_buffers)
	{
		VK_CHECK(vkBeginCommandBuffer(raytracing_command_buffer, &command_buffer_begin_info));

		/*
			Setup the strided device address regions pointing at the shader identifiers in the shader binding table
		*/

		const uint32_t handle_size_aligned = aligned_size(ray_tracing_pipeline_properties.shaderGroupHandleSize, ray_tracing_pipeline_properties.shaderGroupHandleAlignment);

		VkStridedDeviceAddressRegionKHR raygen_shader_sbt_entry{};
		raygen_shader_sbt_entry.deviceAddress = get_buffer_device_address(raygen_shader_binding_table->get_handle());
		raygen_shader_sbt_entry.stride        = handle_size_aligned;
		raygen_shader_sbt_entry.size          = handle_size_aligned;

		VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry{};
		miss_shader_sbt_entry.deviceAddress = get_buffer_device_address(miss_shader_binding_table->get_handle());
		miss_shader_sbt_entry.stride        = handle_size_aligned;
		miss_shader_sbt_entry.size          = handle_size_aligned;

		VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry{};
		hit_shader_sbt_entry.deviceAddress = get_buffer_device_address(hit_shader_binding_table->get_handle());
		hit_shader_sbt_entry.stride        = handle_size_aligned;
		hit_shader_sbt_entry.size          = handle_size_aligned;

		VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry{};

		/*
			Dispatch the ray tracing commands
		*/
		vkCmdBindPipeline(raytracing_command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
		vkCmdBindDescriptorSets(raytracing_command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

		vkCmdTraceRaysKHR(
		    raytracing_command_buffer,
		    &raygen_shader_sbt_entry,
		    &miss_shader_sbt_entry,
		    &hit_shader_sbt_entry,
		    &callable_shader_sbt_entry,
		    width,
		    height,
		    1);

		VK_CHECK(vkEndCommandBuffer(raytracing_command_buffer));
	}
}

void RaytracingExtended::update_uniform_buffers()
{
	uniform_data.proj_inverse = glm::inverse(camera.matrices.perspective);
	uniform_data.view_inverse = glm::inverse(camera.matrices.view);
	ubo->convert_and_update(uniform_data);
}

bool RaytracingExtended::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	// This sample copies the ray traced output to the swap chain image, so we need to enable the required image usage flags
	std::set<VkImageUsageFlagBits> image_usage_flags = {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT};
	get_render_context().update_swapchain(image_usage_flags);

	// Get the ray tracing pipeline properties, which we'll need later on in the sample
	ray_tracing_pipeline_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 device_properties{};
	device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	device_properties.pNext = &ray_tracing_pipeline_properties;
	vkGetPhysicalDeviceProperties2(get_device().get_gpu().get_handle(), &device_properties);

	// Get the acceleration structure features, which we'll need later on in the sample
	acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	VkPhysicalDeviceFeatures2 device_features{};
	device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	device_features.pNext = &acceleration_structure_features;
	vkGetPhysicalDeviceFeatures2(get_device().get_gpu().get_handle(), &device_features);

	camera.type = vkb::CameraType::FirstPerson;
	camera.set_perspective(60.0f, (float) width / (float) height, 0.1f, 512.0f);
	camera.set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));
	camera.set_translation(glm::vec3(0.0f, 1.5f, 0.f));

	create_storage_image();
	create_scene();
	create_uniform_buffer();
	create_ray_tracing_pipeline();
	create_display_pipeline();
	create_shader_binding_tables();
	create_descriptor_sets();
	build_command_buffers();

	prepared = true;
	return true;
}

void RaytracingExtended::draw()
{
	ASSERT_LOG(raytracing_command_buffers.size() == draw_cmd_buffers.size(), "The number of raytracing command buffers must match the render queue size")
	ApiVulkanSample::prepare_frame();
	size_t i = current_buffer;
	
	VkSubmitInfo submit = vkb::initializers::submit_info();
	submit.commandBufferCount  = 1;
	submit.pCommandBuffers      = &raytracing_command_buffers[i];

	VK_CHECK(vkQueueSubmit(queue, 1, &submit, device->request_fence()));
    device->get_fence_pool().wait();

	VkCommandBufferBeginInfo begin = vkb::initializers::command_buffer_begin_info();
	VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &begin));

	VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	/*
		Copy ray tracing output to swap chain image
	*/
	// Prepare current swap chain image as transfer destination
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

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void RaytracingExtended::draw_gui()
{
	gui->show_options_window(
		[]() {
			int current_mode = 0;
		    ImGui::Combo("Draw mode", &current_mode, "Mode\0\0");
		}
	);
}

void RaytracingExtended::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
	if (camera.updated)
		update_uniform_buffers();
}

std::unique_ptr<vkb::VulkanSample> create_raytracing_extended()
{
	return std::make_unique<RaytracingExtended>();
}

RaytracingExtended::RaytracingScene::RaytracingScene(vkb::Device& device, const std::vector<SceneLoadInfo> &scenesToLoad) 
{

	vkb::GLTFLoader loader{device};
	scenes.resize(scenesToLoad.size());
	for (size_t sceneIndex = 0; sceneIndex < scenesToLoad.size(); ++sceneIndex)
	{
		scenes[sceneIndex] = loader.read_scene_from_file(scenesToLoad[sceneIndex].filename);
		ASSERT_LOG(scenes[sceneIndex], "Cannot load file")
		auto &scene = scenes[sceneIndex];
		assert(!!scene);
		for (auto &&mesh : scene->get_components<vkb::sg::Mesh>())
		{
			for (auto &&sub_mesh : mesh->get_submeshes())
			{
				auto   material        = sub_mesh->get_material();
				auto & textures        = material->textures;
				size_t textureIndex    = std::numeric_limits<size_t>::max();
				auto   baseTextureIter = textures.find("base_color_texture");
				bool is_vase           = false;
				if (baseTextureIter != textures.cend())
				{
					auto texture = baseTextureIter->second;
					if (!texture)
						continue;

					const auto name = texture->get_image()->get_name();
					is_vase = (name.find("vase_dif.ktx") != std::basic_string<char>::npos);
					// determine the index of the texture to assign
					auto textureIter = std::find(images.cbegin(), images.cend(), texture->get_image());
					if (textureIter == images.cend())
					{
						textureIndex = images.size();
						auto image   = texture->get_image();
						assert(!!image);
						images.push_back(image);
						VkDescriptorImageInfo imageInfo;
						imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						imageInfo.imageView   = image->get_vk_image_view().get_handle();
						imageInfo.sampler     = baseTextureIter->second->get_sampler()->vk_sampler.get_handle();
						imageInfos.push_back(imageInfo);
					}
					else
					{
						textureIndex = std::distance(images.cbegin(), textureIter);
					}
				}

				auto pts      = CopyBuffer<glm::vec3>{}(sub_mesh->vertex_buffers, "position");
				const auto UV_coords = CopyBuffer<glm::vec2>{}(sub_mesh->vertex_buffers, "texcoord_0");
				const auto normals  = CopyBuffer<glm::vec3>{}(sub_mesh->vertex_buffers, "normal");

				

				auto transform = scenesToLoad[sceneIndex].transform;
				if (is_vase)
				{
					const float sponza_scale = 0.01f;
					transform                = glm::mat3x4{0.f, 0.f, sponza_scale, 4.3f,
                                            sponza_scale, 0.f, 0.f, 0.f,
                                            0.f, sponza_scale, 0.f, 9.5f};
				}
				for (auto&& pt : pts)
				{
					const auto translation = glm::vec3(transform[0][3], transform[1][3], transform[2][3]);
					pt       = glm::vec3(glm::mat4(transform) * glm::vec4(pt, 1.f)) + translation;
				}

				assert(textureIndex < std::numeric_limits<uint32_t>::max());
				const auto textureIndex32 = static_cast<uint32_t>(textureIndex);
				Model          model;
				model.vertices.resize(pts.size());
				for (size_t i = 0; i < pts.size(); ++i)
				{
					auto tex_coords     = i < UV_coords.size() ? UV_coords[i] : glm::vec2{};
					auto normal         = i < normals.size() ? normals[i] : glm::vec3{};
					model.vertices[i].A = {pts[i].x, pts[i].y, pts[i].z, normal.x};
					model.vertices[i].B = {normal.y, normal.z, tex_coords.x, tex_coords.y};
				}

				assert(sub_mesh->index_type == VK_INDEX_TYPE_UINT16);
				auto buffer = sub_mesh->index_buffer.get();
				if (buffer)
				{
					const size_t sz         = buffer->get_size();
					const size_t nTriangles = sz / sizeof(uint16_t) / 3;
					model.triangles.resize(nTriangles);
					auto ptr = buffer->get_data();
					assert(!!ptr);
					std::vector<uint16_t> tempBuffer(nTriangles * 3);
					memcpy(&tempBuffer[0], ptr, sz);
					for (size_t i = 0; i < nTriangles; ++i)
					{
						model.triangles[i] = {uint32_t(tempBuffer[3 * i]),
						                      uint32_t(tempBuffer[3 * i + 1]),
						                      uint32_t(tempBuffer[3 * i + 2])};
					}
				}


				model.default_transform = VkTransformMatrixKHR{1.f, 0.f, 0.f, 0.f,
				                                               0.f, 1.f, 0.f, 0.f,
				                                               0.f, 0.f, 1.f, 0.f};
				model.texture_index = textureIndex32;
				model.object_type   = scenesToLoad[sceneIndex].object_type;
				models.emplace_back(std::move(model));
			}
		}
	}
}
