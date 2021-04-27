/* Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * More complex example for hardware accelerated ray tracing using VK_KHR_ray_tracing_pipeline and VK_KHR_acceleration_structure
  */

#define TINYOBJLOADER_IMPLEMENTATION

#include "raytracing_reflection.h"
#include "tiny_obj_loader.h"
#include <glm/gtc/type_ptr.hpp>

using rt_refl::AccelerationStructure;

RaytracingReflection::RaytracingReflection()
{
	title = "Hardware accelerated ray tracing";

	// SPIRV 1.5 requires Vulkan 1.2
	set_api_version(VK_API_VERSION_1_2);

	// Ray tracing related extensions required by this sample
	add_device_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	add_device_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

	// Required by VK_KHR_acceleration_structure
	add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	add_device_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	add_device_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	add_device_extension(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);

	// Required for VK_KHR_ray_tracing_pipeline
	//add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

	// Required by VK_KHR_spirv_1_4
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
}

RaytracingReflection::~RaytracingReflection()
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
		for (auto &b : bottom_level_acceleration_structure)
		{
			b.buffer.reset();
			vkDestroyAccelerationStructureKHR(device->get_handle(), b.handle, nullptr);
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

void RaytracingReflection::request_gpu_features(vkb::PhysicalDevice &gpu)
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
void RaytracingReflection::create_storage_image()
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
    Create the bottom level acceleration structure that contains the scene's geometry (triangles)
*/
void RaytracingReflection::create_bottom_level_acceleration_structure(ObjModel &obj_model)
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

	// The bottom level acceleration structure contains one set of triangles as the input geometry
	VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
	acceleration_structure_geometry.sType                            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	acceleration_structure_geometry.geometryType                     = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	acceleration_structure_geometry.flags                            = VK_GEOMETRY_OPAQUE_BIT_KHR;
	acceleration_structure_geometry.geometry.triangles.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	acceleration_structure_geometry.geometry.triangles.vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT;
	acceleration_structure_geometry.geometry.triangles.vertexData    = vertex_data_device_address;
	acceleration_structure_geometry.geometry.triangles.maxVertex     = obj_model.nb_vertices;
	acceleration_structure_geometry.geometry.triangles.vertexStride  = sizeof(ObjVertex);
	acceleration_structure_geometry.geometry.triangles.indexType     = VK_INDEX_TYPE_UINT32;
	acceleration_structure_geometry.geometry.triangles.indexData     = index_data_device_address;
	acceleration_structure_geometry.geometry.triangles.transformData = transform_matrix_device_address;

	// Get the size requirements for buffers involved in the acceleration structure build process
	VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
	acceleration_structure_build_geometry_info.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	acceleration_structure_build_geometry_info.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	acceleration_structure_build_geometry_info.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	acceleration_structure_build_geometry_info.geometryCount = 1;
	acceleration_structure_build_geometry_info.pGeometries   = &acceleration_structure_geometry;

	const uint32_t triangle_count = obj_model.nb_indices / 3;

	VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{};
	acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
	    device->get_handle(),
	    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	    &acceleration_structure_build_geometry_info,
	    &triangle_count,
	    &acceleration_structure_build_sizes_info);

	// Create a buffer to hold the acceleration structure
	BLAS blas;
	blas.buffer = std::make_unique<vkb::core::Buffer>(get_device(), acceleration_structure_build_sizes_info.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VMA_MEMORY_USAGE_GPU_ONLY);

	// Create the acceleration structure
	VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
	acceleration_structure_create_info.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	acceleration_structure_create_info.buffer = blas.buffer->get_handle();
	acceleration_structure_create_info.size   = acceleration_structure_build_sizes_info.accelerationStructureSize;
	acceleration_structure_create_info.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	vkCreateAccelerationStructureKHR(device->get_handle(), &acceleration_structure_create_info, nullptr, &blas.handle);

	// The actual build process starts here

	// Create a scratch buffer as a temporary storage for the acceleration structure build
	std::unique_ptr<vkb::core::Buffer> sc_buffer;
	sc_buffer = std::make_unique<vkb::core::Buffer>(get_device(), acceleration_structure_build_sizes_info.buildScratchSize,
	                                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                                                VMA_MEMORY_USAGE_CPU_TO_GPU);

	VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
	acceleration_build_geometry_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
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
	vkCmdBuildAccelerationStructuresKHR(
	    command_buffer,
	    1,
	    &acceleration_build_geometry_info,
	    acceleration_build_structure_range_infos.data());
	get_device().flush_command_buffer(command_buffer, queue);

	//delete_scratch_buffer(scratch_buffer);
	sc_buffer.reset();

	// Get the bottom acceleration structure's handle, which will be used during the top level acceleration build
	VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
	acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	acceleration_device_address_info.accelerationStructure = blas.handle;
	blas.device_address                                    = vkGetAccelerationStructureDeviceAddressKHR(device->get_handle(), &acceleration_device_address_info);

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
	VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
	acceleration_structure_geometry.sType                              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	acceleration_structure_geometry.geometryType                       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	acceleration_structure_geometry.flags                              = VK_GEOMETRY_OPAQUE_BIT_KHR;
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

	const uint32_t primitive_count = static_cast<uint32_t>(blas_instances.size());

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
	std::unique_ptr<vkb::core::Buffer> sc_buffer;
	sc_buffer = std::make_unique<vkb::core::Buffer>(get_device(), acceleration_structure_build_sizes_info.buildScratchSize,
	                                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                                                VMA_MEMORY_USAGE_CPU_TO_GPU);

	VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
	acceleration_build_geometry_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
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

	//delete_scratch_buffer(scratch_buffer);
	sc_buffer.reset();

	// Get the top acceleration structure's handle, which will be used to setup it's descriptor
	VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
	acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	acceleration_device_address_info.accelerationStructure = top_level_acceleration_structure.handle;
	top_level_acceleration_structure.device_address        = vkGetAccelerationStructureDeviceAddressKHR(device->get_handle(), &acceleration_device_address_info);
}

inline uint32_t aligned_size(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

/*
Loading an OBJ file
*/
void RaytracingReflection::load_model(const std::string &file_name, std::shared_ptr<ObjMaterial> mat)
{
	std::string obj_file = vkb::fs::path::get(vkb::fs::path::Type::Assets) + file_name;

	tinyobj::ObjReader reader;
	reader.ParseFromFile(obj_file);
	if (!reader.Valid())
	{
		LOGE(reader.Error().c_str());
		std::cerr << "Cannot load: " << obj_file << std::endl;
		assert(reader.Valid());
	}

	// Collecting the material in the scene
	std::vector<ObjMaterial> materials;
	if (mat != nullptr)
	{
		// Incoming material
		materials.emplace_back(*mat);
	}
	else if (reader.GetMaterials().empty())
	{
		// Default material
		materials.emplace_back(ObjMaterial());
	}
	else
	{
		// OBJ materials
		for (const auto &material : reader.GetMaterials())
		{
			ObjMaterial m;
			m.diffuse   = glm::make_vec3(material.diffuse);
			m.specular  = glm::vec4(glm::make_vec3(material.specular), 0);
			m.shininess = material.shininess;
			materials.emplace_back(m);
		}
	}

	std::vector<ObjVertex> obj_vertices;
	std::vector<uint32_t>  obj_indices;
	std::vector<int32_t>   obj_mat_index;

	const tinyobj::attrib_t &attrib = reader.GetAttrib();
	for (const auto &shape : reader.GetShapes())
	{
		obj_mat_index.insert(obj_mat_index.end(), shape.mesh.material_ids.begin(), shape.mesh.material_ids.end());
		for (const auto &index : shape.mesh.indices)
		{
			ObjVertex    vertex = {};
			const float *vp     = &attrib.vertices[3 * index.vertex_index];
			vertex.pos          = glm::vec4(glm::make_vec3(vp), 0);

			if (!attrib.normals.empty() && index.normal_index >= 0)
			{
				const float *np = &attrib.normals[3 * index.normal_index];
				vertex.nrm      = glm::vec4(glm::make_vec3(np), 0);
			}

			obj_vertices.push_back(vertex);
			obj_indices.push_back(static_cast<int>(obj_indices.size()));
		}
	}

	ObjModel model;
	model.nb_indices  = static_cast<uint32_t>(obj_indices.size());
	model.nb_vertices = static_cast<uint32_t>(obj_vertices.size());

	auto vertex_buffer_size    = obj_vertices.size() * sizeof(ObjVertex);
	auto index_buffer_size     = obj_indices.size() * sizeof(uint32_t);
	auto mat_index_buffer_size = obj_mat_index.size() * sizeof(int32_t);
	auto mat_buffer_size       = materials.size() * sizeof(ObjMaterial);

	// Note that the buffer usage flags for buffers consumed by the bottom level acceleration structure require special flags
	VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	model.vertex_buffer = std::make_unique<vkb::core::Buffer>(get_device(), vertex_buffer_size, buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	model.vertex_buffer->update(obj_vertices.data(), vertex_buffer_size);

	// Acceleration structure flag is not needed for the rest
	buffer_usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	model.index_buffer = std::make_unique<vkb::core::Buffer>(get_device(), index_buffer_size, buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	model.index_buffer->update(obj_indices.data(), index_buffer_size);

	model.mat_index_buffer = std::make_unique<vkb::core::Buffer>(get_device(), mat_index_buffer_size, buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	model.mat_index_buffer->update(obj_mat_index.data(), mat_index_buffer_size);

	model.mat_color_buffer = std::make_unique<vkb::core::Buffer>(get_device(), mat_buffer_size, buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	model.mat_color_buffer->update(materials.data(), mat_buffer_size);

	obj_models.push_back(std::move(model));
}

VkAccelerationStructureInstanceKHR RaytracingReflection::create_blas_instance(uint32_t blas_id, glm::mat4 &mat)
{
	VkTransformMatrixKHR transform_matrix;
	glm::mat3x4          rtxT = glm::transpose(mat);
	memcpy(&transform_matrix, glm::value_ptr(rtxT), sizeof(VkTransformMatrixKHR));

	VkAccelerationStructureInstanceKHR blas_instance{};
	blas_instance.transform                              = transform_matrix;
	blas_instance.instanceCustomIndex                    = blas_id;
	blas_instance.mask                                   = 0xFF;
	blas_instance.instanceShaderBindingTableRecordOffset = 0;
	blas_instance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	blas_instance.accelerationStructureReference         = bottom_level_acceleration_structure[blas_id].device_address;

	return blas_instance;
}

/*
    Create scene geometry and ray tracing acceleration structures
*/
void RaytracingReflection::create_scene()
{
	std::shared_ptr<ObjMaterial> mat_red = std::make_shared<ObjMaterial>();
	mat_red->diffuse                     = glm::vec3(1, 0, 0);
	mat_red->specular                    = glm::vec4(1.0f);
	mat_red->shininess                   = 0.0f;

	std::shared_ptr<ObjMaterial> mat_grey = std::make_shared<ObjMaterial>();
	mat_grey->diffuse                     = glm::vec3(0.7f);
	mat_grey->specular                    = glm::vec4(0.95f);
	mat_grey->shininess                   = 0.1f;

	std::shared_ptr<ObjMaterial> mat_mirror = std::make_shared<ObjMaterial>();
	mat_mirror->diffuse                     = glm::vec3(0, 0, 0.7f);
	mat_mirror->specular                    = glm::vec4(0.98f);
	mat_mirror->shininess                   = 0.98f;

	load_model("scenes/cube.obj", mat_red);
	load_model("scenes/plane.obj", mat_grey);
	load_model("scenes/cube.obj", mat_mirror);

	create_bottom_level_acceleration_structure(obj_models[0]);
	create_bottom_level_acceleration_structure(obj_models[1]);
	create_bottom_level_acceleration_structure(obj_models[2]);

	std::vector<VkAccelerationStructureInstanceKHR> blas_instances;
	blas_instances.push_back(create_blas_instance(0, glm::translate(glm::mat4(), glm::vec3(-1.0f, 0.0f, 0.0f))));
	blas_instances.push_back(create_blas_instance(0, glm::translate(glm::mat4(), glm::vec3(1.0f, 0.0f, 0.0f))));
	blas_instances.push_back(create_blas_instance(1, glm::translate(glm::mat4(), glm::vec3(0.0f, -1.0f, 0.0f))));

	glm::mat4 m_mirror = glm::scale(glm::mat4(), glm::vec3(5.0f, 5.0f, 0.1f));
	blas_instances.push_back(create_blas_instance(2, glm::translate(m_mirror, glm::vec3(0.0f, 0.0f, -35.0f))));
	blas_instances.push_back(create_blas_instance(2, glm::translate(m_mirror, glm::vec3(0.0f, 0.0f, 35.0f))));

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
	const uint32_t           handle_size             = ray_tracing_pipeline_properties.shaderGroupHandleSize;
	const uint32_t           handle_size_aligned     = aligned_size(ray_tracing_pipeline_properties.shaderGroupHandleSize, ray_tracing_pipeline_properties.shaderGroupHandleAlignment);
	const uint32_t           handle_alignment        = ray_tracing_pipeline_properties.shaderGroupHandleAlignment;
	const uint32_t           group_count             = static_cast<uint32_t>(shader_groups.size());
	const uint32_t           sbt_size                = group_count * handle_size_aligned;
	const VkBufferUsageFlags sbt_buffer_usafge_flags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	const VmaMemoryUsage     sbt_memory_usage        = VMA_MEMORY_USAGE_CPU_TO_GPU;

	// Raygen
	// Create binding table buffers for each shader type
	raygen_shader_binding_table = std::make_unique<vkb::core::Buffer>(get_device(), handle_size, sbt_buffer_usafge_flags, sbt_memory_usage, 0);
	miss_shader_binding_table   = std::make_unique<vkb::core::Buffer>(get_device(), handle_size, sbt_buffer_usafge_flags, sbt_memory_usage, 0);
	hit_shader_binding_table    = std::make_unique<vkb::core::Buffer>(get_device(), handle_size, sbt_buffer_usafge_flags, sbt_memory_usage, 0);

	// Copy the pipeline's shader handles into a host buffer
	std::vector<uint8_t> shader_handle_storage(sbt_size);
	VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(get_device().get_handle(), pipeline, 0, group_count, sbt_size, shader_handle_storage.data()));

	// Copy the shader handles from the host buffer to the binding tables
	uint8_t *data = static_cast<uint8_t *>(raygen_shader_binding_table->map());
	memcpy(data, shader_handle_storage.data(), handle_size);
	data = static_cast<uint8_t *>(miss_shader_binding_table->map());
	memcpy(data, shader_handle_storage.data() + handle_size_aligned, handle_size * 2);        // 2 miss shaders
	data = static_cast<uint8_t *>(hit_shader_binding_table->map());
	memcpy(data, shader_handle_storage.data() + handle_size_aligned * 3, handle_size);        // rgen + 2*miss == 3
	raygen_shader_binding_table->unmap();
	miss_shader_binding_table->unmap();
	hit_shader_binding_table->unmap();
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
	    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nbObj * 3},        // Material + Vertex + index
	};
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

	// All materials
	std::vector<VkDescriptorBufferInfo> mat_descriptors;
	std::vector<VkDescriptorBufferInfo> vtx_descriptors;
	std::vector<VkDescriptorBufferInfo> idx_descriptors;
	for (uint32_t i = 0; i < nbObj; ++i)
	{
		mat_descriptors.push_back(create_descriptor(*obj_models[i].mat_color_buffer));
		vtx_descriptors.push_back(create_descriptor(*obj_models[i].vertex_buffer));
		idx_descriptors.push_back(create_descriptor(*obj_models[i].index_buffer));
	}

	VkWriteDescriptorSet result_image_write   = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &image_descriptor);
	VkWriteDescriptorSet uniform_buffer_write = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &buffer_descriptor);
	VkWriteDescriptorSet mat_buffer_write     = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, mat_descriptors.data(), nbObj);
	VkWriteDescriptorSet vtx_buffer_write     = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, vtx_descriptors.data(), nbObj);
	VkWriteDescriptorSet idx_buffer_write     = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5, idx_descriptors.data(), nbObj);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    acceleration_structure_write,
	    result_image_write,
	    uniform_buffer_write,
	    mat_buffer_write,
	    vtx_buffer_write,
	    idx_buffer_write,
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

	// Scene elements
	uint32_t nbObj = static_cast<uint32_t>(obj_models.size());

	// Material
	VkDescriptorSetLayoutBinding material_buffer_binding{};
	material_buffer_binding.binding         = 3;
	material_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	material_buffer_binding.descriptorCount = nbObj;
	material_buffer_binding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	// Vertex
	VkDescriptorSetLayoutBinding vertex_buffer_binding{};
	vertex_buffer_binding.binding         = 4;
	vertex_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	vertex_buffer_binding.descriptorCount = nbObj;
	vertex_buffer_binding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	// Index
	VkDescriptorSetLayoutBinding index_buffer_binding{};
	index_buffer_binding.binding         = 5;
	index_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	index_buffer_binding.descriptorCount = nbObj;
	index_buffer_binding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	std::vector<VkDescriptorSetLayoutBinding> bindings = {
	    acceleration_structure_layout_binding,
	    result_image_layout_binding,
	    uniform_buffer_binding,
	    material_buffer_binding,
	    vertex_buffer_binding,
	    index_buffer_binding,
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

	// Ray tracing shaders require SPIR-V 1.4, so we need to set the appropriate target environment for the glslang compiler
	vkb::GLSLCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);

	/*
        Setup ray tracing shader groups
        Each shader group points at the corresponding shader in the pipeline
    */
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

	// Ray generation group
	{
		shader_stages.push_back(load_shader("khr_ray_tracing_reflection/raygen.rgen", VK_SHADER_STAGE_RAYGEN_BIT_KHR));
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
		shader_stages.push_back(load_shader("khr_ray_tracing_reflection/miss.rmiss", VK_SHADER_STAGE_MISS_BIT_KHR));
		VkRayTracingShaderGroupCreateInfoKHR miss_group_ci{};
		miss_group_ci.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		miss_group_ci.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		miss_group_ci.generalShader      = static_cast<uint32_t>(shader_stages.size()) - 1;
		miss_group_ci.closestHitShader   = VK_SHADER_UNUSED_KHR;
		miss_group_ci.anyHitShader       = VK_SHADER_UNUSED_KHR;
		miss_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
		shader_groups.push_back(miss_group_ci);
	}

	// Ray miss (shadow) group
	{
		shader_stages.push_back(load_shader("khr_ray_tracing_reflection/missShadow.rmiss", VK_SHADER_STAGE_MISS_BIT_KHR));
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
		shader_stages.push_back(load_shader("khr_ray_tracing_reflection/closesthit.rchit", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));
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
		vkDestroyAccelerationStructureKHR(device->get_handle(), acceleration_structure.handle, nullptr);
	}
}

/*
    Create the uniform buffer used to pass matrices to the ray tracing ray generation shader
*/
void RaytracingReflection::create_uniform_buffer()
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
		build_command_buffers();
	}

	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

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
		miss_shader_sbt_entry.size          = handle_size_aligned;

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

void RaytracingReflection::update_uniform_buffers()
{
	auto mat = camera.matrices.perspective;
	mat[1][1] *= -1;        // Flipping Y axis

	uniform_data.proj_inverse = glm::inverse(mat);
	uniform_data.view_inverse = glm::inverse(camera.matrices.view);
	ubo->convert_and_update(uniform_data);
}

bool RaytracingReflection::prepare(vkb::Platform &platform)
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

	camera.type = vkb::CameraType::LookAt;
	camera.set_perspective(60.0f, (float) width / (float) height, 0.1f, 512.0f);
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
		return;
	draw();
	if (camera.updated)
		update_uniform_buffers();
}

std::unique_ptr<vkb::VulkanSample> create_raytracing_reflection()
{
	return std::make_unique<RaytracingReflection>();
}
