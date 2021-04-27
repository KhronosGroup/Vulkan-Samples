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

struct BLAS
{
	VkAccelerationStructureKHR         handle;
	uint64_t                           device_address;
	std::unique_ptr<vkb::core::Buffer> buffer;
};

// Structure holding the material
struct ObjMaterial
{
	glm::vec3 diffuse   = {0.7f, 0.7f, 0.7f};
	float     shininess = 0.f;
	glm::vec4 specular  = {0.7f, 0.7f, 0.7f, 0.f};
};

// OBJ representation of a vertex
// NOTE: BLAS builder depends on pos being the first member
struct ObjVertex
{
	glm::vec4 pos;
	glm::vec4 nrm;
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

	std::vector<BLAS>              bottom_level_acceleration_structure;
	rt_refl::AccelerationStructure top_level_acceleration_structure;

	// Array of objects and instances in the scene
	std::vector<ObjModel>    obj_models;
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

	void                               request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void                               create_storage_image();
	void                               create_bottom_level_acceleration_structure(ObjModel &obj_model);
	void                               create_top_level_acceleration_structure(std::vector<VkAccelerationStructureInstanceKHR> &blas_instances);
	void                               load_model(const std::string &file_name, std::shared_ptr<ObjMaterial> mat = {});
	VkAccelerationStructureInstanceKHR create_blas_instance(uint32_t blas_id, glm::mat4 &mat);
	void                               delete_acceleration_structure(rt_refl::AccelerationStructure &acceleration_structure);
	void                               create_scene();
	void                               create_shader_binding_tables();
	void                               create_descriptor_sets();
	void                               create_ray_tracing_pipeline();
	void                               create_uniform_buffer();
	void                               build_command_buffers() override;
	void                               update_uniform_buffers();
	void                               draw();
	bool                               prepare(vkb::Platform &platform) override;
	virtual void                       render(float delta_time) override;
};

std::unique_ptr<vkb::VulkanSample> create_raytracing_reflection();
