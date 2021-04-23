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
 * Basic example for hardware accelerated ray tracing using VK_KHR_ray_tracing_pipeline and VK_KHR_acceleration_structure
 */

#pragma once

#include "api_vulkan_sample.h"
#include "glsl_compiler.h"

namespace rt_refl
{
// Wraps all data required for an acceleration structure
struct AccelerationStructure
{
	VkAccelerationStructureKHR         handle;
	uint64_t                           device_address;
	std::unique_ptr<vkb::core::Buffer> buffer;
};

}        // namespace rt_refl

// Structure holding the material
struct MaterialObj
{
	glm::vec3 ambient       = {0.1f, 0.1f, 0.1f};
	glm::vec3 diffuse       = {0.7f, 0.7f, 0.7f};
	glm::vec3 specular      = {1.0f, 1.0f, 1.0f};
	glm::vec3 transmittance = {0.0f, 0.0f, 0.0f};
	glm::vec3 emission      = {0.0f, 0.0f, 0.10};
	float     shininess     = 0.f;
	float     ior           = 1.0f;        // index of refraction
	float     dissolve      = 1.f;         // 1 == opaque; 0 == fully transparent
	    // illumination model (see http://www.fileformat.info/format/material/)
	int illum     = 0;
	int textureID = -1;
};

// OBJ representation of a vertex
// NOTE: BLAS builder depends on pos being the first member
struct ObjVertex
{
	glm::vec3 pos;
	glm::vec3 nrm;
	glm::vec3 color;
	glm::vec2 texCoord;
};

struct ObjModel
{
	uint32_t                           nb_indices{0};
	uint32_t                           nb_vertices{0};
	std::unique_ptr<vkb::core::Buffer> vertex_buffer;           // Device buffer of all 'Vertex'
	std::unique_ptr<vkb::core::Buffer> index_buffer;            // Device buffer of the indices forming triangles
	std::unique_ptr<vkb::core::Buffer> mat_color_buffer;        // Device buffer of array of 'Wavefront material'
	std::unique_ptr<vkb::core::Buffer> mat_index_buffer;        // Device buffer of array of 'Wavefront material'
};

// Instance of the OBJ
struct ObjInstance
{
	uint32_t  objIndex{0};           // Reference to the `m_objModel`
	uint32_t  txtOffset{0};          // Offset in `m_textures`
	glm::mat4 transform{1};          // Position of the instance
	glm::mat4 transformIT{1};        // Inverse transpose
};

class RaytracingReflection : public ApiVulkanSample
{
  public:
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  ray_tracing_pipeline_properties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{};

	rt_refl::AccelerationStructure bottom_level_acceleration_structure;
	rt_refl::AccelerationStructure top_level_acceleration_structure;

	// Array of objects and instances in the scene
	std::vector<ObjModel>    m_objModel;
	std::vector<ObjInstance> m_objInstance;

	//std::vector<std::unique_ptr<const vkb::core::Buffer>> mat_buffers;
	std::unique_ptr<vkb::core::Buffer>                vertex_buffer;
	std::unique_ptr<vkb::core::Buffer>                index_buffer;
	uint32_t                                          index_count;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups{};

	std::unique_ptr<vkb::core::Buffer> raygen_shader_binding_table;
	std::unique_ptr<vkb::core::Buffer> miss_shader_binding_table;
	std::unique_ptr<vkb::core::Buffer> hit_shader_binding_table;

	struct StorageImage
	{
		VkDeviceMemory memory;
		VkImage        image = VK_NULL_HANDLE;
		VkImageView    view;
		VkFormat       format;
		uint32_t       width;
		uint32_t       height;
	} storage_image;

	struct UniformData
	{
		glm::mat4 view_inverse;
		glm::mat4 proj_inverse;
	} uniform_data;
	std::unique_ptr<vkb::core::Buffer> ubo;

	VkPipeline            pipeline;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;

	RaytracingReflection();
	~RaytracingReflection();

	void         request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void         create_storage_image();
	void         create_bottom_level_acceleration_structure(ObjModel &obj_model);
	void         create_top_level_acceleration_structure();
	void         load_model(const std::string &file_name);
	void         delete_acceleration_structure(rt_refl::AccelerationStructure &acceleration_structure);
	void         create_scene();
	void         create_shader_binding_tables();
	void         create_descriptor_sets();
	void         create_ray_tracing_pipeline();
	void         create_uniform_buffer();
	void         build_command_buffers() override;
	void         update_uniform_buffers();
	void         draw();
	bool         prepare(vkb::Platform &platform) override;
	virtual void render(float delta_time) override;
};

std::unique_ptr<vkb::VulkanSample> create_raytracing_reflection();
