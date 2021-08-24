/* Copyright (c) 2021, Holochip Corporation
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

#pragma once

#include "api_vulkan_sample.h"

/**
 * @brief Offloading processes from CPU to GPU
 */
class BindlessResources : public ApiVulkanSample
{
  public:
	BindlessResources();

	~BindlessResources() override;

	bool prepare(vkb::Platform &platform) override;

	void render(float delta_time) override;

	void finish() override;

  private:
	enum RenderMode
	{
		CPU,
		GPU,
		GPU_DEVICE_ADDRESS
	} render_mode = GPU;
	struct Vertex
	{
		glm::vec3 pt;
		glm::vec2 uv;
	};

	struct BoundingSphere
	{
		BoundingSphere() = default;
		explicit BoundingSphere(const std::vector<glm::vec3> &pts);
		glm::vec3 center = {0, 0, 0};
		float     radius = 0;
	};

	struct GpuModelInformation
	{
		glm::vec3 bounding_sphere_center;
		float     bounding_sphere_radius;
		uint32_t  texture_index = 0;
		uint32_t  firstIndex    = 0;
		uint32_t  indexCount    = 0;
		uint32_t  _pad          = 0;
	};

	struct SceneUniform
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 proj_view;
		uint32_t model_count;
	} scene_uniform;

	struct SceneModel
	{
		std::vector<Vertex>                  vertices;
		std::vector<std::array<uint16_t, 3>> triangles;
		size_t                               vertex_buffer_offset = 0;
		size_t                               index_buffer_offset  = 0;
		size_t                               texture_index        = 0;
		BoundingSphere                       bounding_sphere;
	};

	struct Texture
	{
		std::unique_ptr<vkb::core::Image>     image;
		std::unique_ptr<vkb::core::ImageView> image_view;
		uint32_t                              n_mip_maps;
	};

	std::vector<SceneModel>            models;
	std::unique_ptr<vkb::core::Buffer> vertex_buffer;
	std::unique_ptr<vkb::core::Buffer> index_buffer;
	std::unique_ptr<vkb::core::Buffer> model_information_buffer;
	std::unique_ptr<vkb::core::Buffer> scene_uniform_buffer;
	std::vector<Texture>               textures;
	std::vector<VkDescriptorImageInfo> image_descriptors;
	bool                               m_freeze_cull      = false;
	bool                               m_enable_mci       = true;
	bool                               m_requires_rebuild = false;

	VkPipeline            pipeline{nullptr};
	VkPipelineLayout      pipeline_layout{nullptr};
	VkDescriptorSetLayout descriptor_set_layout{nullptr};
	VkDescriptorSet       descriptor_set{nullptr};
	VkSampler             sampler{nullptr};

	// GPU Draw Calls
	void                  run_cull();
	void                  run_gpu_cull();
	VkPipeline            gpu_cull_pipeline{nullptr};
	VkPipelineLayout      gpu_cull_pipeline_layout{nullptr};
	VkDescriptorSetLayout gpu_cull_descriptor_set_layout{nullptr};
	VkDescriptorSet       gpu_cull_descriptor_set{nullptr};

	// Device Address
	VkPipeline                         device_address_pipeline{nullptr};
	VkPipelineLayout                   device_address_pipeline_layout{nullptr};
	VkDescriptorSetLayout              device_address_descriptor_set_layout{nullptr};
	VkDescriptorSet                    device_address_descriptor_set{nullptr};
	std::unique_ptr<vkb::core::Buffer> device_address_buffer{nullptr};

	std::vector<vkb::CommandBuffer> compute_command_buffers{};
	const vkb::Queue *              compute_queue{nullptr};

	// CPU Draw Calls
	void                                      cpu_cull();
	std::vector<VkDrawIndexedIndirectCommand> cpu_commands;
	std::unique_ptr<vkb::core::Buffer>        cpu_staging_buffer;
	std::unique_ptr<vkb::core::Buffer>        indirect_call_buffer;

	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void create_sampler();
	void load_scene();
	void initialize_resources();
	void create_pipeline();
	void create_compute_pipeline();
	void initialize_descriptors();
	void update_scene_uniform();
	void draw();
	bool m_supports_mdi            = false;
	bool m_supports_first_instance = false;
	bool m_supports_buffer_device  = false;
};

std::unique_ptr<vkb::VulkanSample> create_bindless();
