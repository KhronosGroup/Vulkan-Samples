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

#define DISPLACEMENT_MAP_DIM 256u

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

	void build_compute_command_buffer();

	void generate_plane();
	void prepare_uniform_buffers();
	void setup_descriptor_pool();
	void create_semaphore();
	void create_descriptor_set_layout();
	void create_descriptor_set();
	void create_pipelines();

	void create_initial_tildas();
	void create_tildas();
	void create_butterfly_texture();
	void create_fft();
	void create_fft_inversion();

	void update_uniform_buffers();

	float               phillips_spectrum(int32_t n, int32_t m, float sign);
	std::complex<float> hTilde_0(uint32_t n, uint32_t m, float sign);
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
		std::unique_ptr<vkb::core::Buffer> vertex      = {VK_NULL_HANDLE};
		std::unique_ptr<vkb::core::Buffer> index       = {VK_NULL_HANDLE};
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

	struct FFTPage
	{
		alignas(4) int32_t page = {0u};
	};

	struct FFTInvert
	{
		alignas(4) int32_t page_idx   = {-1};
		alignas(4) uint32_t grid_size = {DISPLACEMENT_MAP_DIM};
	};

	struct TimeUbo
	{
		alignas(4) float time = {0.0f};
	} fftTime;

	struct GuiConfig
	{
		bool      wireframe = {false};
		float     amplitude = {2.0f};
		float     length    = {1000.0f};
		glm::vec2 wind      = {100.0f, -100.0f};
	} ui;

	uint32_t                           grid_size      = {DISPLACEMENT_MAP_DIM};
	std::unique_ptr<vkb::core::Buffer> camera_ubo     = {VK_NULL_HANDLE};
	std::unique_ptr<vkb::core::Buffer> fft_params_ubo = {VK_NULL_HANDLE};
	std::unique_ptr<vkb::core::Buffer> fft_time_ubo   = {VK_NULL_HANDLE};
	std::unique_ptr<vkb::core::Buffer> fft_page_ubo   = {VK_NULL_HANDLE};
	std::unique_ptr<vkb::core::Buffer> invert_fft_ubo = {VK_NULL_HANDLE};

	std::vector<std::complex<float>> h_tilde_0;
	std::vector<std::complex<float>> h_tilde_0_conj;
	std::vector<std::complex<float>> weights;

	struct FBAttachment
	{
		VkImage        image;
		VkDeviceMemory memory;
		VkImageView    view;
		VkFormat       format;
		void           destroy(VkDevice device)
		{
			vkDestroyImageView(device, view, nullptr);
			vkDestroyImage(device, image, nullptr);
			vkFreeMemory(device, memory, nullptr);
		};
	} butterfly_precomp;

	uint32_t log_2_N;

	std::unique_ptr<vkb::core::Buffer> bit_reverse_buffer = {VK_NULL_HANDLE};

	struct
	{
		std::unique_ptr<vkb::core::Buffer> fft_input_htilde0      = {VK_NULL_HANDLE};
		std::unique_ptr<vkb::core::Buffer> fft_input_htilde0_conj = {VK_NULL_HANDLE};
		std::unique_ptr<vkb::core::Buffer> fft_input_weight       = {VK_NULL_HANDLE};
		std::unique_ptr<FBAttachment>      fft_tilde_h_kt_dx      = {VK_NULL_HANDLE};
		std::unique_ptr<FBAttachment>      fft_tilde_h_kt_dy      = {VK_NULL_HANDLE};
		std::unique_ptr<FBAttachment>      fft_tilde_h_kt_dz      = {VK_NULL_HANDLE};
		std::unique_ptr<FBAttachment>      fft_displacement       = {VK_NULL_HANDLE};
	} fft_buffers;

	struct
	{
		VkQueue         queue              = {VK_NULL_HANDLE};
		VkCommandPool   command_pool       = {VK_NULL_HANDLE};
		VkCommandBuffer command_buffer     = {VK_NULL_HANDLE};
		VkSemaphore     semaphore          = {VK_NULL_HANDLE};
		uint32_t        queue_family_index = {-1u};
	} compute;

	struct
	{
		VkDescriptorSetLayout descriptor_set_layout = {VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set_axis_y = {VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set_axis_x = {VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set_axis_z = {VK_NULL_HANDLE};

		struct
		{
			Pipeline horizontal;
			Pipeline vertical;
		} pipelines;

		std::unique_ptr<FBAttachment> tilde_axis_y = {VK_NULL_HANDLE};
		std::unique_ptr<FBAttachment> tilde_axis_x = {VK_NULL_HANDLE};
		std::unique_ptr<FBAttachment> tilde_axis_z = {VK_NULL_HANDLE};
	} fft;

	struct
	{
		VkDescriptorSetLayout descriptor_set_layout = {VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set        = {VK_NULL_HANDLE};
		Pipeline              pipeline;
	} fft_inversion;

	struct
	{
		VkDescriptorSetLayout descriptor_set_layout = {VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set        = {VK_NULL_HANDLE};
		Pipeline              pipeline;
	} initial_tildas;

	struct
	{
		VkDescriptorSetLayout descriptor_set_layout = {VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set        = {VK_NULL_HANDLE};
		Pipeline              pipeline;
	} tildas;

	struct
	{
		VkDescriptorSetLayout descriptor_set_layout = {VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set        = {VK_NULL_HANDLE};
		Pipeline              pipeline;
	} precompute;

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

  private:
	uint32_t              reverse(uint32_t i);
	VkDescriptorImageInfo create_fb_descriptor(FBAttachment &attachment);
	void                  createFBAttachement(VkFormat format, uint32_t width, uint32_t height, FBAttachment &result);
};

std::unique_ptr<vkb::VulkanSample> create_subgroups_operations();
