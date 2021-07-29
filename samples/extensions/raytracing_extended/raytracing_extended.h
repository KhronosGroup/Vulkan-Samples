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

#pragma once

#include "api_vulkan_sample.h"
#include "glsl_compiler.h"





class RaytracingExtended : public ApiVulkanSample
{
  public:
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  ray_tracing_pipeline_properties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{};

	enum RenderMode : uint32_t
	{
		RENDER_DEFAULT,
		RENDER_BARYCENTRIC,
		RENDER_INSTANCE_ID,
		RENDER_DISTANCE,
		RENDER_GLOBAL_XYZ,
		RENDER_SHADOW_MAP,
		RENDER_AO
	};

	enum ObjectType : uint32_t
	{
		OBJECT_NORMAL, // has AO and ray traced shadows
		OBJECT_REFRACTION, // pass-through with IOR
		OBJECT_FLAME // emission surface; constant amplitude
	};

	// Wraps all data required for an acceleration structure
	struct AccelerationStructureExtended
	{
		VkAccelerationStructureKHR         handle;
		uint64_t                           device_address;
		std::unique_ptr<vkb::core::Buffer> buffer;
	};

	struct NewVertex;
	struct Model;

	struct ModelBuffer
	{
		size_t                                   vertex_offset = std::numeric_limits<size_t>::max();        // in bytes
		size_t                                   index_offset  = std::numeric_limits<size_t>::max();        // in bytes
		size_t                                   num_vertices  = std::numeric_limits<size_t>::max();
		size_t                                   num_triangles = std::numeric_limits<size_t>::max();
		std::unique_ptr<vkb::core::Buffer>       transform_matrix_buffer = nullptr;
		VkAccelerationStructureBuildSizesInfoKHR buildSize;
		VkAccelerationStructureGeometryKHR       acceleration_structure_geometry;
		VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo;
		AccelerationStructureExtended            bottom_level_acceleration_structure;
		VkTransformMatrixKHR                     default_transform;
		uint32_t                                 object_type = 0;
		bool                                     is_static = true;
	};

	struct SceneOptions
	{
		bool use_vertex_staging_buffer = true;
	} scene_options;

	// fixed buffers
	std::unique_ptr<vkb::core::Buffer> vertex_buffer = nullptr;
	std::unique_ptr<vkb::core::Buffer> index_buffer = nullptr;
	std::unique_ptr<vkb::core::Buffer> dynamic_vertex_buffer = nullptr;
	std::unique_ptr<vkb::core::Buffer> dynamic_index_buffer  = nullptr;

	struct SceneLoadInfo
	{
		SceneLoadInfo() = default;
		SceneLoadInfo(const char *filename, glm::mat3x4 transform, uint32_t object_type) :
		    filename(filename), transform(transform), object_type(object_type)
		{}
		const char *filename = "";
		glm::mat3x4 transform;
		uint32_t object_type = 0;
	};

	struct RaytracingScene
	{
		RaytracingScene() = default;
		~RaytracingScene() = default;
		RaytracingScene(vkb::Device& device, const std::vector<SceneLoadInfo> &scenesToLoad);
		std::vector<std::unique_ptr<vkb::sg::Scene>> scenes;
		std::vector<vkb::sg::Image *>      images;
		std::vector<VkDescriptorImageInfo> imageInfos;
		std::vector<Model>                 models;
		std::vector<ModelBuffer>		   modelBuffers;
	};

	std::unique_ptr<RaytracingScene> raytracing_scene;


	AccelerationStructureExtended top_level_acceleration_structure;

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
		StorageImage()
		    : memory(nullptr)
		    , image(VK_NULL_HANDLE)
		    , view(nullptr)
		    , format()
		    , width(0)
		    , height(0) {}
	} storage_image;

	struct UniformData
	{
		glm::mat4 view_inverse;
		glm::mat4 proj_inverse;
	} uniform_data;
	std::unique_ptr<vkb::core::Buffer> ubo;

	struct SceneInstanceData
	{
		uint32_t vertex_index; // index of first data
		uint32_t indices_index;
		uint32_t image_index;
		uint32_t object_type; // controls how shader handles object / whether to load from buffer for static objects or dynamic objects
	};
	std::unique_ptr<vkb::core::Buffer> data_to_model_buffer;

	std::vector<VkCommandBuffer> raytracing_command_buffers;
	VkPipeline            pipeline;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;


	RaytracingExtended();
	~RaytracingExtended() override;

	void          request_gpu_features(vkb::PhysicalDevice &gpu) override;
	uint64_t      get_buffer_device_address(VkBuffer buffer);
	void          create_storage_image();
	void          create_static_object_buffers();
	void          create_dynamic_object_buffers();
	void          create_bottom_level_acceleration_structure();
	void          create_top_level_acceleration_structure();
	void          delete_acceleration_structure(AccelerationStructureExtended &acceleration_structure);

	void          create_scene();
	void          create_shader_binding_tables();
	void          create_descriptor_sets();
	void          create_ray_tracing_pipeline();
	void          create_display_pipeline();
	void          create_uniform_buffer();
	void          build_command_buffers() override;
	void          update_uniform_buffers();
	void          draw();
	void          draw_gui() override;
	bool          prepare(vkb::Platform &platform) override;
	void          render(float delta_time) override;
};

std::unique_ptr<vkb::VulkanSample> create_raytracing_extended();
