/* Copyright (c) 2019, Sascha Willems
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
 * Basic example for ray tracing using VK_NV_ray_tracing
 */

#include "raytracing_basic.h"

RaytracingBasic::RaytracingBasic()
{
	title = "VK_NV_ray_tracing";

	// Enable instance and device extensions required to use VK_NV_ray_tracing
	instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	device_extensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
	device_extensions.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);
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
		vkFreeMemory(get_device().get_handle(), bottom_level_acceleration_structure.memory, nullptr);
		vkFreeMemory(get_device().get_handle(), top_level_acceleration_structure.memory, nullptr);
		vkDestroyAccelerationStructureNV(get_device().get_handle(), bottom_level_acceleration_structure.acceleration_structure, nullptr);
		vkDestroyAccelerationStructureNV(get_device().get_handle(), top_level_acceleration_structure.acceleration_structure, nullptr);
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
	The bottom level acceleration structure contains the scene's geometry (vertices, triangles)
*/
void RaytracingBasic::create_bottom_level_acceleration_structure(const VkGeometryNV *geometries)
{
	VkAccelerationStructureInfoNV acceleration_structure_info{};
	acceleration_structure_info.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	acceleration_structure_info.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	acceleration_structure_info.instanceCount = 0;
	acceleration_structure_info.geometryCount = 1;
	acceleration_structure_info.pGeometries   = geometries;

	VkAccelerationStructureCreateInfoNV acceleration_structure_create_info{};
	acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	acceleration_structure_create_info.info  = acceleration_structure_info;
	VK_CHECK(vkCreateAccelerationStructureNV(get_device().get_handle(), &acceleration_structure_create_info, nullptr, &bottom_level_acceleration_structure.acceleration_structure));

	VkAccelerationStructureMemoryRequirementsInfoNV memory_requirements{};
	memory_requirements.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memory_requirements.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	memory_requirements.accelerationStructure = bottom_level_acceleration_structure.acceleration_structure;

	VkMemoryRequirements2 memory_requirements2{};
	vkGetAccelerationStructureMemoryRequirementsNV(get_device().get_handle(), &memory_requirements, &memory_requirements2);

	VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
	memory_allocate_info.allocationSize       = memory_requirements2.memoryRequirements.size;
	memory_allocate_info.memoryTypeIndex      = get_device().get_memory_type(memory_requirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &bottom_level_acceleration_structure.memory));

	VkBindAccelerationStructureMemoryInfoNV acceleration_structure_memory_info{};
	acceleration_structure_memory_info.sType                 = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	acceleration_structure_memory_info.accelerationStructure = bottom_level_acceleration_structure.acceleration_structure;
	acceleration_structure_memory_info.memory                = bottom_level_acceleration_structure.memory;
	VK_CHECK(vkBindAccelerationStructureMemoryNV(get_device().get_handle(), 1, &acceleration_structure_memory_info));

	VK_CHECK(vkGetAccelerationStructureHandleNV(get_device().get_handle(), bottom_level_acceleration_structure.acceleration_structure, sizeof(uint64_t), &bottom_level_acceleration_structure.handle));
}

/*
	The top level acceleration structure contains the scene's object instances
*/
void RaytracingBasic::create_top_level_acceleration_structure()
{
	VkAccelerationStructureInfoNV acceleration_structure_info{};
	acceleration_structure_info.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	acceleration_structure_info.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	acceleration_structure_info.instanceCount = 1;
	acceleration_structure_info.geometryCount = 0;

	VkAccelerationStructureCreateInfoNV acceleration_structure_create_info{};
	acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	acceleration_structure_create_info.info  = acceleration_structure_info;
	VK_CHECK(vkCreateAccelerationStructureNV(get_device().get_handle(), &acceleration_structure_create_info, nullptr, &top_level_acceleration_structure.acceleration_structure));

	VkAccelerationStructureMemoryRequirementsInfoNV memory_requirements{};
	memory_requirements.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memory_requirements.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	memory_requirements.accelerationStructure = top_level_acceleration_structure.acceleration_structure;

	VkMemoryRequirements2 memory_requirements2{};
	vkGetAccelerationStructureMemoryRequirementsNV(get_device().get_handle(), &memory_requirements, &memory_requirements2);

	VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
	memory_allocate_info.allocationSize       = memory_requirements2.memoryRequirements.size;
	memory_allocate_info.memoryTypeIndex      = get_device().get_memory_type(memory_requirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &top_level_acceleration_structure.memory));

	VkBindAccelerationStructureMemoryInfoNV acceleration_structure_memory_info{};
	acceleration_structure_memory_info.sType                 = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	acceleration_structure_memory_info.accelerationStructure = top_level_acceleration_structure.acceleration_structure;
	acceleration_structure_memory_info.memory                = top_level_acceleration_structure.memory;
	VK_CHECK(vkBindAccelerationStructureMemoryNV(get_device().get_handle(), 1, &acceleration_structure_memory_info));

	VK_CHECK(vkGetAccelerationStructureHandleNV(get_device().get_handle(), top_level_acceleration_structure.acceleration_structure, sizeof(uint64_t), &top_level_acceleration_structure.handle));
}

/*
	Create scene geometry and ray tracing acceleration structures
*/
void RaytracingBasic::create_scene()
{
	// Setup vertices for a single triangle
	struct Vertex
	{
		float pos[4];
	};
	std::vector<Vertex> vertices = {
	    {{1.0f, 1.0f, 0.0f, 1.0f}},
	    {{-1.0f, 1.0f, 0.0f, 1.0f}},
	    {{0.0f, -1.0f, 0.0f, 1.0f}}};

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
	                                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                    VMA_MEMORY_USAGE_GPU_TO_CPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	// Index buffer
	index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                   index_buffer_size,
	                                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                                   VMA_MEMORY_USAGE_GPU_TO_CPU);
	index_buffer->update(indices.data(), index_buffer_size);

	/*
		Create the bottom level acceleration structure containing the actual scene geometry
	*/
	VkGeometryNV geometry{};
	geometry.sType                              = VK_STRUCTURE_TYPE_GEOMETRY_NV;
	geometry.geometryType                       = VK_GEOMETRY_TYPE_TRIANGLES_NV;
	geometry.geometry.triangles.sType           = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
	geometry.geometry.triangles.vertexData      = vertex_buffer->get_handle();
	geometry.geometry.triangles.vertexOffset    = 0;
	geometry.geometry.triangles.vertexCount     = static_cast<uint32_t>(vertices.size());
	geometry.geometry.triangles.vertexStride    = sizeof(Vertex);
	geometry.geometry.triangles.vertexFormat    = VK_FORMAT_R32G32B32_SFLOAT;
	geometry.geometry.triangles.indexData       = index_buffer->get_handle();
	geometry.geometry.triangles.indexOffset     = 0;
	geometry.geometry.triangles.indexCount      = index_count;
	geometry.geometry.triangles.indexType       = VK_INDEX_TYPE_UINT32;
	geometry.geometry.triangles.transformData   = VK_NULL_HANDLE;
	geometry.geometry.triangles.transformOffset = 0;
	geometry.geometry.aabbs                     = {};
	geometry.geometry.aabbs.sType               = {VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV};
	geometry.flags                              = VK_GEOMETRY_OPAQUE_BIT_NV;

	create_bottom_level_acceleration_structure(&geometry);

	/*
		Create the top-level acceleration structure that contains geometry instance information
	*/

	glm::mat3x4 transform = {
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f,
	    0.0f,
	};

	GeometryInstance geometry_instance{};
	geometry_instance.transform                     = transform;
	geometry_instance.instance_id                   = 0;
	geometry_instance.mask                          = 0xff;
	geometry_instance.instance_offset               = 0;
	geometry_instance.flags                         = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
	geometry_instance.acceleration_structure_handle = bottom_level_acceleration_structure.handle;

	// Single instance with a 3x4 transform matrix for the ray traced triangle
	vkb::core::Buffer instance_buffer{get_device(), sizeof(GeometryInstance), VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VMA_MEMORY_USAGE_CPU_ONLY};

	instance_buffer.convert_and_update(geometry_instance);

	create_top_level_acceleration_structure();

	/*
		Build the acceleration structure
	*/

	// Acceleration structure build requires some scratch space to store temporary information
	VkAccelerationStructureMemoryRequirementsInfoNV memory_requirements_info{};
	memory_requirements_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memory_requirements_info.type  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;	

	VkMemoryRequirements2 memory_requirements_bottom_level;
	memory_requirements_info.accelerationStructure = bottom_level_acceleration_structure.acceleration_structure;
	vkGetAccelerationStructureMemoryRequirementsNV(get_device().get_handle(), &memory_requirements_info, &memory_requirements_bottom_level);

	VkMemoryRequirements2 memory_requirements_top_level;
	memory_requirements_info.accelerationStructure = top_level_acceleration_structure.acceleration_structure;
	vkGetAccelerationStructureMemoryRequirementsNV(get_device().get_handle(), &memory_requirements_info, &memory_requirements_top_level);

	const VkDeviceSize scratch_buffer_size = std::max(memory_requirements_bottom_level.memoryRequirements.size, memory_requirements_top_level.memoryRequirements.size);

	vkb::core::Buffer scratch_buffer{get_device(), scratch_buffer_size, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VMA_MEMORY_USAGE_GPU_ONLY};

	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	/*
		Build bottom level acceleration structure
	*/
	VkAccelerationStructureInfoNV build_info{};
	build_info.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	build_info.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	build_info.geometryCount = 1;
	build_info.pGeometries   = &geometry;

	vkCmdBuildAccelerationStructureNV(
	    command_buffer,
	    &build_info,
	    VK_NULL_HANDLE,
	    0,
	    VK_FALSE,
	    bottom_level_acceleration_structure.acceleration_structure,
	    VK_NULL_HANDLE,
	    scratch_buffer.get_handle(),
	    0);

	VkMemoryBarrier memory_barrier = vkb::initializers::memory_barrier();
	memory_barrier.srcAccessMask   = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memory_barrier.dstAccessMask   = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memory_barrier, 0, 0, 0, 0);

	/*
		Build top-level acceleration structure
	*/
	build_info.pGeometries   = 0;
	build_info.geometryCount = 0;
	build_info.instanceCount = 1;

	vkCmdBuildAccelerationStructureNV(
	    command_buffer,
	    &build_info,
	    instance_buffer.get_handle(),
	    0,
	    VK_FALSE,
	    top_level_acceleration_structure.acceleration_structure,
	    VK_NULL_HANDLE,
	    scratch_buffer.get_handle(),
	    0);

	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memory_barrier, 0, 0, 0, 0);

	get_device().flush_command_buffer(command_buffer, queue);
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
	// Create buffer for the shader binding table
	const uint32_t shader_binding_table_size = ray_tracing_properties.shaderGroupHandleSize * 3;
	shader_binding_table                     = std::make_unique<vkb::core::Buffer>(get_device(),
                                                               shader_binding_table_size,
                                                               VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                                                               VMA_MEMORY_USAGE_GPU_TO_CPU);

	auto shader_handle_storage = new uint8_t[shader_binding_table_size];
	// Get shader identifiers
	VK_CHECK(vkGetRayTracingShaderGroupHandlesNV(get_device().get_handle(), pipeline, 0, 3, shader_binding_table_size, shader_handle_storage));
	auto *data = static_cast<uint8_t *>(shader_binding_table->map());
	// Copy the shader identifiers to the shader binding table
	VkDeviceSize offset = 0;
	data += copy_shader_identifier(data, shader_handle_storage, INDEX_RAYGEN);
	data += copy_shader_identifier(data, shader_handle_storage, INDEX_MISS);
	data += copy_shader_identifier(data, shader_handle_storage, INDEX_CLOSEST_HIT);
	shader_binding_table->unmap();
}

/*
	Create the descriptor sets used for the ray tracing dispatch
*/
void RaytracingBasic::create_descriptor_sets()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1},
	    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
	    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};
	VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(pool_sizes, 1);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info, &descriptor_set));

	VkWriteDescriptorSetAccelerationStructureNV descriptor_acceleration_structure_info{};
	descriptor_acceleration_structure_info.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
	descriptor_acceleration_structure_info.accelerationStructureCount = 1;
	descriptor_acceleration_structure_info.pAccelerationStructures    = &top_level_acceleration_structure.acceleration_structure;

	VkWriteDescriptorSet acceleration_structure_write{};
	acceleration_structure_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	// The specialized acceleration structure descriptor has to be chained
	acceleration_structure_write.pNext           = &descriptor_acceleration_structure_info;
	acceleration_structure_write.dstSet          = descriptor_set;
	acceleration_structure_write.dstBinding      = 0;
	acceleration_structure_write.descriptorCount = 1;
	acceleration_structure_write.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

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
	acceleration_structure_layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
	acceleration_structure_layout_binding.descriptorCount = 1;
	acceleration_structure_layout_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_NV;

	VkDescriptorSetLayoutBinding result_image_layout_binding{};
	result_image_layout_binding.binding         = 1;
	result_image_layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	result_image_layout_binding.descriptorCount = 1;
	result_image_layout_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_NV;

	VkDescriptorSetLayoutBinding uniform_buffer_binding{};
	uniform_buffer_binding.binding         = 2;
	uniform_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniform_buffer_binding.descriptorCount = 1;
	uniform_buffer_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_NV;

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

	const uint32_t shader_index_raygen      = 0;
	const uint32_t shader_index_miss        = 1;
	const uint32_t shader_index_closest_hit = 2;

	std::array<VkPipelineShaderStageCreateInfo, 3> shader_stages;
	shader_stages[shader_index_raygen]      = load_shader("nv_ray_tracing_basic/raygen.rgen", VK_SHADER_STAGE_RAYGEN_BIT_NV);
	shader_stages[shader_index_miss]        = load_shader("nv_ray_tracing_basic/miss.rmiss", VK_SHADER_STAGE_MISS_BIT_NV);
	shader_stages[shader_index_closest_hit] = load_shader("nv_ray_tracing_basic/closesthit.rchit", VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

	/*
		Setup ray tracing shader groups
	*/
	std::array<VkRayTracingShaderGroupCreateInfoNV, 3> groups{};
	for (auto &group : groups)
	{
		// Init all groups with some default values
		group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
		group.generalShader      = VK_SHADER_UNUSED_NV;
		group.closestHitShader   = VK_SHADER_UNUSED_NV;
		group.anyHitShader       = VK_SHADER_UNUSED_NV;
		group.intersectionShader = VK_SHADER_UNUSED_NV;
	}

	// Links shaders and types to ray tracing shader groups
	groups[INDEX_RAYGEN].type                  = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	groups[INDEX_RAYGEN].generalShader         = shader_index_raygen;
	groups[INDEX_MISS].type                    = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	groups[INDEX_MISS].generalShader           = shader_index_miss;
	groups[INDEX_CLOSEST_HIT].type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
	groups[INDEX_CLOSEST_HIT].generalShader    = VK_SHADER_UNUSED_NV;
	groups[INDEX_CLOSEST_HIT].closestHitShader = shader_index_closest_hit;

	VkRayTracingPipelineCreateInfoNV raytracing_pipeline_create_info{};
	raytracing_pipeline_create_info.sType             = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
	raytracing_pipeline_create_info.stageCount        = static_cast<uint32_t>(shader_stages.size());
	raytracing_pipeline_create_info.pStages           = shader_stages.data();
	raytracing_pipeline_create_info.groupCount        = static_cast<uint32_t>(groups.size());
	raytracing_pipeline_create_info.pGroups           = groups.data();
	raytracing_pipeline_create_info.maxRecursionDepth = 1;
	raytracing_pipeline_create_info.layout            = pipeline_layout;
	VK_CHECK(vkCreateRayTracingPipelinesNV(get_device().get_handle(), VK_NULL_HANDLE, 1, &raytracing_pipeline_create_info, nullptr, &pipeline));
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
			Dispatch the ray tracing commands
		*/
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipeline);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipeline_layout, 0, 1, &descriptor_set, 0, 0);

		// Calculate shader binding offsets, which is pretty straight forward in our example
		VkDeviceSize binding_offset_ray_gen_shader = static_cast<VkDeviceSize>(ray_tracing_properties.shaderGroupHandleSize * INDEX_RAYGEN);
		VkDeviceSize binding_offset_miss_shader    = static_cast<VkDeviceSize>(ray_tracing_properties.shaderGroupHandleSize * INDEX_MISS);
		VkDeviceSize binding_offset_hit_shader     = static_cast<VkDeviceSize>(ray_tracing_properties.shaderGroupHandleSize * INDEX_CLOSEST_HIT);
		VkDeviceSize binding_stride                = ray_tracing_properties.shaderGroupHandleSize;

		vkCmdTraceRaysNV(draw_cmd_buffers[i],
		                 shader_binding_table->get_handle(), binding_offset_ray_gen_shader,
		                 shader_binding_table->get_handle(), binding_offset_miss_shader, binding_stride,
		                 shader_binding_table->get_handle(), binding_offset_hit_shader, binding_stride,
		                 VK_NULL_HANDLE, 0, 0,
		                 width, height, 1);

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

		//@todo: Default render pass setup willl overwrite contents
		//vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		//drawUI(drawCmdBuffers[i]);
		//vkCmdEndRenderPass(drawCmdBuffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void RaytracingBasic::update_uniform_buffers()
{
	uniform_data.proj_inverse = glm::inverse(camera.matrices.perspective);
	uniform_data.view_inverse = glm::inverse(camera.matrices.view);
	memcpy(ubo->map(), &uniform_data, sizeof(uniform_data));
}

bool RaytracingBasic::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	// Query the ray tracing properties of the current implementation, we will need them later on
	ray_tracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
	VkPhysicalDeviceProperties2 device_properties{};
	device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	device_properties.pNext = &ray_tracing_properties;
	vkGetPhysicalDeviceProperties2(get_device().get_physical_device(), &device_properties);

	// Get VK_NV_ray_tracing related function pointers
	vkCreateAccelerationStructureNV                = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(vkGetDeviceProcAddr(get_device().get_handle(), "vkCreateAccelerationStructureNV"));
	vkDestroyAccelerationStructureNV               = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(vkGetDeviceProcAddr(get_device().get_handle(), "vkDestroyAccelerationStructureNV"));
	vkBindAccelerationStructureMemoryNV            = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(vkGetDeviceProcAddr(get_device().get_handle(), "vkBindAccelerationStructureMemoryNV"));
	vkGetAccelerationStructureHandleNV             = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(vkGetDeviceProcAddr(get_device().get_handle(), "vkGetAccelerationStructureHandleNV"));
	vkGetAccelerationStructureMemoryRequirementsNV = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(vkGetDeviceProcAddr(get_device().get_handle(), "vkGetAccelerationStructureMemoryRequirementsNV"));
	vkCmdBuildAccelerationStructureNV              = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(vkGetDeviceProcAddr(get_device().get_handle(), "vkCmdBuildAccelerationStructureNV"));
	vkCreateRayTracingPipelinesNV                  = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(vkGetDeviceProcAddr(get_device().get_handle(), "vkCreateRayTracingPipelinesNV"));
	vkGetRayTracingShaderGroupHandlesNV            = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(vkGetDeviceProcAddr(get_device().get_handle(), "vkGetRayTracingShaderGroupHandlesNV"));
	vkCmdTraceRaysNV                               = reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(get_device().get_handle(), "vkCmdTraceRaysNV"));

	camera.type = vkb::CameraType::LookAt;
	camera.set_perspective(60.0f, (float) width / (float) height, 0.1f, 512.0f);
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
