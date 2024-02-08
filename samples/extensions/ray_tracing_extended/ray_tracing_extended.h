/* Copyright (c) 2021-2024 Holochip Corporation
 * Copyright (c) 2024, Mobica Limited
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

#define USE_FRAMEWORK_ACCELERATION_STRUCTURE

#include "api_vulkan_sample.h"
#include "shader_compiler.h"
#include <core/acceleration_structure.h>

class RaytracingExtended : public ApiVulkanSample
{
  public:
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  ray_tracing_pipeline_properties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{};

	enum RenderMode : uint32_t
	{
		RENDER_DEFAULT     = 0,
		RENDER_BARYCENTRIC = 1,
		RENDER_INSTANCE_ID = 2,
		RENDER_DISTANCE    = 3,
		RENDER_GLOBAL_XYZ  = 4,
		RENDER_SHADOW_MAP  = 5,
		RENDER_AO          = 6
	};

	enum ObjectType : uint32_t
	{
		OBJECT_NORMAL,            // has AO and ray traced shadows
		OBJECT_REFRACTION,        // pass-through with IOR
		OBJECT_FLAME              // emission surface; constant amplitude
	};

#ifndef USE_FRAMEWORK_ACCELERATION_STRUCTURE
	// Wraps all data required for an acceleration structure
	struct AccelerationStructureExtended
	{
		VkAccelerationStructureKHR         handle         = nullptr;
		uint64_t                           device_address = 0;
		std::unique_ptr<vkb::core::Buffer> buffer;
	};
#endif

	struct NewVertex;
	struct Model;

	struct FlameParticle
	{
		glm::vec3 position;
		glm::vec3 velocity;
		float     duration = 0.f;
	};

	struct FlameParticleGenerator
	{
		FlameParticleGenerator() = default;

		FlameParticleGenerator(glm::vec3 generator_origin, glm::vec3 generator_direction, float generator_radius, size_t n_particles) :
		    origin(generator_origin), direction(generator_direction), radius(generator_radius), n_particles(n_particles), generator(std::chrono::system_clock::now().time_since_epoch().count())
		{
			using namespace glm;
			u = normalize(abs(dot(generator_direction, vec3(0, 0, 1))) > 0.9f ? cross(generator_direction, vec3(1, 0, 0)) : cross(generator_direction, vec3(0, 0, 1)));
			v = normalize(cross(generator_direction, u));

			for (size_t i = 0; i < n_particles; ++i)
			{
				float starting_lifetime = generate_random() * lifetime;
				particles.emplace_back(generateParticle(starting_lifetime));
			}
		}
		~FlameParticleGenerator() = default;
		FlameParticle generateParticle(float _lifetime = 0.f) const
		{
			using namespace glm;
			const float theta              = 2.f * 3.14159f * generate_random();
			const float R                  = radius * generate_random();
			const vec3  velocity_direction = generate_random_direction();

			FlameParticle particle;
			particle.position = origin + R * (sin(theta) * u + cos(theta) * v);
			particle.velocity = generate_random() * 0.2f * velocity_direction;
			particle.duration = _lifetime;
			return particle;
		}
		glm::vec3 generate_random_direction() const
		{
			using namespace glm;
			return normalize(0.2f * generate_random() * u + 0.2f * generate_random() * v + 0.8f * direction * generate_random());
		}
		void update_particles(float time_delta)
		{
			particles.erase(std::remove_if(particles.begin(), particles.end(), [this, lifetime{this->lifetime}](const FlameParticle &particle) {
				                return particle.duration > (generate_random() * lifetime);
			                }),
			                particles.end());

			for (auto &&particle : particles)
			{
				particle.position += time_delta * particle.velocity;
				// particle.velocity = 0.75f * particle.velocity + 0.25f * generate_random_direction();
				particle.duration += time_delta;
			}

			for (size_t i = particles.size(); i < n_particles; ++i)
			{
				particles.emplace_back(generateParticle(0.f));
			}
		}

		float generate_random() const
		{
			std::uniform_real_distribution<float> distribution = std::uniform_real_distribution<float>(0, 1);
			return distribution(generator);
		}

		mutable std::default_random_engine generator;
		std::vector<FlameParticle>         particles;
		glm::vec3                          origin    = {0, 0, 0};
		glm::vec3                          direction = {0, 0, 0};
		glm::vec3                          u = {0, 0, 0}, v = {0, 0, 0};
		float                              lifetime    = 5;
		float                              radius      = 0.f;
		size_t                             n_particles = 0;
	};

	FlameParticleGenerator flame_generator;

	struct ModelBuffer
	{
		size_t                                   vertex_offset           = std::numeric_limits<size_t>::max();        // in bytes
		size_t                                   index_offset            = std::numeric_limits<size_t>::max();        // in bytes
		size_t                                   num_vertices            = std::numeric_limits<size_t>::max();
		size_t                                   num_triangles           = std::numeric_limits<size_t>::max();
		uint32_t                                 texture_index           = std::numeric_limits<uint32_t>::max();
		std::unique_ptr<vkb::core::Buffer>       transform_matrix_buffer = nullptr;
		VkAccelerationStructureBuildSizesInfoKHR buildSize;
		VkAccelerationStructureGeometryKHR       acceleration_structure_geometry;
		VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo;
#ifdef USE_FRAMEWORK_ACCELERATION_STRUCTURE
		std::unique_ptr<vkb::core::AccelerationStructure> bottom_level_acceleration_structure = nullptr;
#else
		AccelerationStructureExtended bottom_level_acceleration_structure;
#endif
		VkTransformMatrixKHR default_transform;
		uint32_t             object_type = 0;
		bool                 is_static   = true;
		uint64_t             object_id   = 0;
	};

	struct SceneOptions
	{
		bool use_vertex_staging_buffer = true;
	} scene_options;
	size_t                                         frame_count = 0;
	std::chrono::high_resolution_clock::time_point start       = std::chrono::high_resolution_clock::now();

	// fixed buffers
	std::unique_ptr<vkb::core::Buffer> vertex_buffer         = nullptr;
	std::unique_ptr<vkb::core::Buffer> index_buffer          = nullptr;
	std::unique_ptr<vkb::core::Buffer> dynamic_vertex_buffer = nullptr;
	std::unique_ptr<vkb::core::Buffer> dynamic_index_buffer  = nullptr;
	std::unique_ptr<vkb::core::Buffer> instances_buffer      = nullptr;

	struct SceneLoadInfo
	{
		SceneLoadInfo() = default;
		SceneLoadInfo(const char *filename, glm::mat3x4 transform, uint32_t object_type) :
		    filename(filename), transform(transform), object_type(object_type)
		{}
		const char *filename = "";
		glm::mat3x4 transform;
		uint32_t    object_type = 0;
	};

	struct RaytracingScene
	{
		RaytracingScene()  = default;
		~RaytracingScene() = default;
		RaytracingScene(vkb::Device &device, const std::vector<SceneLoadInfo> &scenesToLoad);
		std::vector<std::unique_ptr<vkb::sg::Scene>> scenes;
		std::vector<VkDescriptorImageInfo>           imageInfos;
		std::vector<Model>                           models;
		std::vector<ModelBuffer>                     model_buffers;
	};

	std::unique_ptr<RaytracingScene> raytracing_scene;
	Texture                          flame_texture;

#ifdef USE_FRAMEWORK_ACCELERATION_STRUCTURE
	std::unique_ptr<vkb::core::AccelerationStructure> top_level_acceleration_structure = nullptr;
#else
	AccelerationStructureExtended top_level_acceleration_structure;
#endif
	uint64_t                                          instance_uid = std::numeric_limits<uint64_t>::max();
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
		StorageImage() :
		    memory(VK_NULL_HANDLE), image(VK_NULL_HANDLE), view(VK_NULL_HANDLE), format(), width(0), height(0)
		{}
	} storage_image;

	struct UniformData
	{
		glm::mat4 view_inverse;
		glm::mat4 proj_inverse;
	} uniform_data;
	std::unique_ptr<vkb::core::Buffer> ubo;

	struct SceneInstanceData
	{
		uint32_t vertex_index;        // index of first data
		uint32_t indices_index;
		uint32_t image_index;
		uint32_t object_type;        // controls how shader handles object / whether to load from buffer for static objects or dynamic objects
	};
	std::unique_ptr<vkb::core::Buffer> data_to_model_buffer;

	std::vector<VkCommandBuffer> raytracing_command_buffers;
	VkPipeline                   pipeline;
	VkPipelineLayout             pipeline_layout;
	VkDescriptorSet              descriptor_set;
	VkDescriptorSetLayout        descriptor_set_layout;
	using Triangle                   = std::array<uint32_t, 3>;
	uint32_t               grid_size = 100;
	std::vector<NewVertex> refraction_model;
	std::vector<Triangle>  refraction_indices;

	RaytracingExtended();
	~RaytracingExtended() override;

	void                 request_gpu_features(vkb::PhysicalDevice &gpu) override;
	uint64_t             get_buffer_device_address(VkBuffer buffer);
	void                 create_storage_image();
	void                 create_static_object_buffers();
	void                 create_flame_model();
	void                 create_dynamic_object_buffers(float time);
	void                 create_bottom_level_acceleration_structure(bool is_update, bool print_time = true);
	VkTransformMatrixKHR calculate_rotation(glm::vec3 pt, float scale = 1.f, bool freeze_y = false);
	void                 create_top_level_acceleration_structure(bool print_time = true);
#ifndef USE_FRAMEWORK_ACCELERATION_STRUCTURE
	void delete_acceleration_structure(AccelerationStructureExtended &acceleration_structure);
#endif

	void create_scene();
	void create_shader_binding_tables();
	void create_descriptor_sets();
	void create_ray_tracing_pipeline();
	void create_uniform_buffer();
	void build_command_buffers() override;
	void update_uniform_buffers();
	void draw();
	bool prepare(const vkb::ApplicationOptions &options) override;
	void render(float delta_time) override;
};

std::unique_ptr<vkb::VulkanSample> create_ray_tracing_extended();
