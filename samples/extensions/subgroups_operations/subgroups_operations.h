/* Copyright (c) 2023, Mobica Limited
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
#include <complex>

class SubgroupsOperations : public ApiVulkanSample
{
	struct OceanVertex
	{
		glm::vec3 position;
	};

  public:
	SubgroupsOperations();
	~SubgroupsOperations();

	bool prepare(vkb::Platform &platform) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;
	void render(float delta_time) override;
	bool resize(const uint32_t width, const uint32_t height) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

	void draw();
	void load_assets();

	void prepare_compute();
	void create_compute_queue();
	void create_compute_command_pool();
	void create_compute_command_buffer();
	void create_compute_descriptor_set_layout();
	void create_compute_descriptor_set();

	void preapre_compute_pipeline_layout();
	void prepare_compute_pipeline();

	void build_compute_command_buffer();

	void generate_plane();
	void prepare_uniform_buffers();
	void setup_descriptor_pool();
	void create_semaphore();
	void create_descriptor_set_layout();
	void create_descriptor_set();
	void create_pipelines();

	void update_uniform_buffers();
	void update_compute_descriptor();

	// ocean stuff
	float               phillips_spectrum(int32_t n, int32_t m);
	std::complex<float> hTilde_0(uint32_t n, uint32_t m);
	std::complex<float> rndGaussian();
	std::complex<float> calculate_weight(uint32_t x, uint32_t n);

	struct Pipeline
	{
		void destroy(VkDevice device);

		VkPipeline       pipeline        = {VK_NULL_HANDLE};
		VkPipelineLayout pipeline_layout = {VK_NULL_HANDLE};
	};

	struct GridBuffers
	{
		std::unique_ptr<vkb::core::Buffer> vertex;
		std::unique_ptr<vkb::core::Buffer> index;
		uint32_t                           index_count = {0u};
	};

	struct CameraUbo
	{
		alignas(16) glm::mat4 projection;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 model;
	};

	struct FFTParametersUbo
	{
		alignas(4) float amplitude;
		alignas(4) float length;
		alignas(4) uint32_t grid_size;
		alignas(8) glm::vec2 wind;
	};

	struct GuiConfig
	{
		bool      wireframe = {false};
		float     amplitude = {0.005f};
		float     length    = {16.0f};
		glm::vec2 wind      = {16.0f, 0.0f};
	} ui;

	uint32_t                           grid_size = {128u};
	std::unique_ptr<vkb::core::Buffer> camera_ubo;
	std::unique_ptr<vkb::core::Buffer> fft_params_ubo;

	std::vector<std::complex<float>> h_tilde_0;
	std::vector<std::complex<float>> h_tilde_0_conj;
	std::vector<std::complex<float>> weights;

	struct
	{
		std::unique_ptr<vkb::core::Buffer> fft_input_htilde0;
		std::unique_ptr<vkb::core::Buffer> fft_input_htilde0_conj;
		std::unique_ptr<vkb::core::Buffer> fft_input_weight;
		std::unique_ptr<vkb::sg::Image>    fft_height_map_image;
	} fft_buffers;

	struct
	{
		VkQueue               queue                 = {VK_NULL_HANDLE};
		VkCommandPool         command_pool          = {VK_NULL_HANDLE};
		VkCommandBuffer       command_buffer        = {VK_NULL_HANDLE};
		VkSemaphore           semaphore             = {VK_NULL_HANDLE};
		VkDescriptorSetLayout descriptor_set_layout = {VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set        = {VK_NULL_HANDLE};
		uint32_t              queue_family_index    = {-1u};

		struct
		{
			Pipeline _default;
		} pipelines;
	} compute;

	struct
	{
		GridBuffers           grid;
		uint32_t              graphics_queue_family_index = {-1u};
		VkDescriptorSetLayout descriptor_set_layout       = {VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set              = {VK_NULL_HANDLE};
		VkSemaphore           semaphore                   = {VK_NULL_HANDLE};
		struct
		{
			Pipeline _default;
			Pipeline wireframe;
		} pipelines;
	} ocean;

	VkPhysicalDeviceSubgroupProperties subgroups_properties;
};

std::unique_ptr<vkb::VulkanSample> create_subgroups_operations();
