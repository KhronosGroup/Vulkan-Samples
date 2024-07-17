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

class SubgroupsOperations : public ApiVulkanSample
{
  public:
	SubgroupsOperations();
	~SubgroupsOperations();

	bool prepare(const vkb::ApplicationOptions &options) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;
	void render(float delta_time) override;
	bool resize(const uint32_t width, const uint32_t height) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void create_command_pool() override;

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
	void create_skybox();

	void create_initial_tides();
	void create_tildas();
	void create_butterfly_texture();
	void create_fft();
	void create_fft_inversion();
	void create_fft_normal_map();

	void update_uniform_buffers();

	static glm::vec2 rndGaussian();

	struct Pipeline
	{
		void destroy(VkDevice device) const;

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

	struct SkyboxUbo
	{
		alignas(16) glm::mat4 mvp;
	};

	struct OceanParamsUbo
	{
		alignas(16) glm::vec3 light_color;
		alignas(16) glm::vec3 light_position;
		alignas(16) glm::vec3 ocean_color;
	};

	struct CameraPositionUbo
	{
		alignas(16) glm::vec4 position;
	};

	struct TessellationParamsUbo
	{
		alignas(4) float choppines;
		alignas(4) float displacement_scale;
	};

	struct FFTParametersUbo
	{
		alignas(4) float amplitude;
		alignas(4) float length;
		alignas(4) uint32_t grid_size;
		alignas(8) glm::vec2 wind;
	};

	struct FFTInvertUbo
	{
		alignas(4) int32_t page_idx   = {-1};
		alignas(4) uint32_t grid_size = {grid_size};
	};

	struct TimeUbo
	{
		alignas(4) float time = {0.0f};
	};

	struct Wind
	{
		Wind()
		{
			recalc();
		}
		void      recalc();
		glm::vec2 vec;
		float     angle = {180.0f};
		float     force = {25.0f};
	};

	struct GuiConfig
	{
		bool  wireframe          = {true};
		float choppines          = {0.1f};
		float displacement_scale = {0.5f};
		float amplitude          = {32.0f};
		float length             = {1900.0f};
		Wind  wind;

		glm::vec3 light_pos   = {100.0f, 15.0f, 10.0f};
		glm::vec3 light_color = {1.0f, 1.0f, 1.0f};
		glm::vec3 ocean_color = {0.0f, 0.2423423f, 0.434335435f};
	} ui;

	uint32_t                           grid_size               = {256U};
	std::unique_ptr<vkb::core::Buffer> skybox_ubo              = {VK_NULL_HANDLE};
	std::unique_ptr<vkb::core::Buffer> ocean_params_ubo        = {VK_NULL_HANDLE};
	std::unique_ptr<vkb::core::Buffer> camera_position_ubo     = {VK_NULL_HANDLE};
	std::unique_ptr<vkb::core::Buffer> camera_ubo              = {VK_NULL_HANDLE};
	std::unique_ptr<vkb::core::Buffer> tessellation_params_ubo = {VK_NULL_HANDLE};
	std::unique_ptr<vkb::core::Buffer> fft_params_ubo          = {VK_NULL_HANDLE};
	std::unique_ptr<vkb::core::Buffer> fft_time_ubo            = {VK_NULL_HANDLE};
	std::unique_ptr<vkb::core::Buffer> invert_fft_ubo          = {VK_NULL_HANDLE};
	std::unique_ptr<vkb::core::Buffer> bit_reverse_buffer      = {VK_NULL_HANDLE};

	std::vector<glm::vec4> input_random;

	struct ImageAttachment
	{
		VkImage        image;
		VkDeviceMemory memory;
		VkImageView    view;
		VkFormat       format;
		VkSampler      sampler;
		void           destroy(VkDevice device) const
		{
			vkDestroySampler(device, sampler, nullptr);
			vkDestroyImageView(device, view, nullptr);
			vkDestroyImage(device, image, nullptr);
			vkFreeMemory(device, memory, nullptr);
		};
	};

	ImageAttachment butterfly_precomp{};

	uint32_t   log_2_N{};
	vkb::Timer timer;

	struct
	{
		std::unique_ptr<vkb::core::Buffer> fft_input_random;
		std::unique_ptr<ImageAttachment>   fft_input_htilde0;
		std::unique_ptr<ImageAttachment>   fft_input_htilde0_conj;

		std::unique_ptr<ImageAttachment> fft_tilde_h_kt_dx;
		std::unique_ptr<ImageAttachment> fft_tilde_h_kt_dy;
		std::unique_ptr<ImageAttachment> fft_tilde_h_kt_dz;
		std::unique_ptr<ImageAttachment> fft_displacement;
		std::unique_ptr<ImageAttachment> fft_normal_map;
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

		std::unique_ptr<ImageAttachment> tilde_axis_y = {VK_NULL_HANDLE};
		std::unique_ptr<ImageAttachment> tilde_axis_x = {VK_NULL_HANDLE};
		std::unique_ptr<ImageAttachment> tilde_axis_z = {VK_NULL_HANDLE};
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
	} fft_normal_map;

	struct
	{
		VkDescriptorSetLayout descriptor_set_layout = {VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set        = {VK_NULL_HANDLE};
		Pipeline              pipeline;
	} initial_tildes;

	struct
	{
		VkDescriptorSetLayout descriptor_set_layout = {VK_NULL_HANDLE};
		VkDescriptorSet       descriptor_set        = {VK_NULL_HANDLE};
		Pipeline              pipeline;
	} tildes;

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

	struct Skybox
	{
		void destroy(VkDevice device)
		{
			pipeline.destroy(device);
			vkDestroySampler(device, skybox_texture.sampler, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
			skybox_shape.reset();
		}

		Pipeline              pipeline;
		VkDescriptorSetLayout descriptor_set_layout;
		VkDescriptorSet       descriptor_set;

		Texture                           skybox_texture;
		std::unique_ptr<vkb::sg::SubMesh> skybox_shape;
	} skybox;

	VkPhysicalDeviceSubgroupProperties subgroups_properties{};

  private:
	uint32_t              reverse(uint32_t i) const;
	VkDescriptorImageInfo create_ia_descriptor(ImageAttachment &attachment);
	void                  create_image_attachement(VkFormat format, uint32_t width, uint32_t height, ImageAttachment &result);
};

std::unique_ptr<vkb::VulkanSampleC> create_subgroups_operations();
