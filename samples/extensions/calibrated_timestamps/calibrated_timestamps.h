/* Copyright (c) 2023, Holochip Corporation
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
 * Demonstrates and showcase calibrated timestamps extension related functionalities
 */

#pragma once

#include "api_vulkan_sample.h"

class CalibratedTimestamps : public ApiVulkanSample
{
  private:
	bool                         isTimeDomainUpdated = false;        // this is just to tell if time domain update has a VK_SUCCESS in the end
	bool                         isTimestampUpdated  = false;        // this is just to tell the GUI whether timestamps operation has a VK_SUCCESS in the end
	bool                         isOptimalTimeDomain = false;        // this tells if the optimal time domain is found
	bool                         isDisplay           = false;
	uint32_t                     time_domain_count   = 0;
	std::vector<VkTimeDomainEXT> time_domains{};          // this holds all time domains extracted from the current Instance
	std::vector<uint64_t>        timestamps{};            // timestamps vector
	std::vector<uint64_t>        max_deviations{};        // max deviations vector

	struct
	{
		int             index = 0;
		VkTimeDomainEXT timeDomainEXT{};
	} optimal_time_domain{};

	struct DeltaTimestamp
	{
		uint64_t    begin = 0;
		uint64_t    end   = 0;
		uint64_t    delta = 0;
		std::string tag   = "Untagged";

		void get_delta()
		{
			this->delta = this->end - this->begin;
		}
	};

	std::vector<DeltaTimestamp> delta_timestamps{};

  public:
	bool bloom          = true;
	bool display_skybox = true;

	struct
	{
		Texture environment_map{};
	} textures{};

	struct Models
	{
		std::unique_ptr<vkb::sg::SubMesh>              skybox{};
		std::vector<std::unique_ptr<vkb::sg::SubMesh>> objects{};
		std::vector<glm::mat4>                         transforms{};
		int32_t                                        object_index = 0;
	} models{};

	struct
	{
		std::unique_ptr<vkb::core::Buffer> matrices{};
		std::unique_ptr<vkb::core::Buffer> params{};
	} uniform_buffers{};

	struct UBOVS
	{
		glm::mat4 projection{};
		glm::mat4 model_view{};
		glm::mat4 skybox_model_view{};
		float     model_scale = 0.05f;
	} ubo_vs{};

	struct UBOParams
	{
		float exposure = 1.0f;
	} ubo_params{};

	struct
	{
		VkPipeline skybox      = VK_NULL_HANDLE;
		VkPipeline reflect     = VK_NULL_HANDLE;
		VkPipeline composition = VK_NULL_HANDLE;
		VkPipeline bloom[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
	} pipelines{};

	struct
	{
		VkPipelineLayout models       = VK_NULL_HANDLE;
		VkPipelineLayout composition  = VK_NULL_HANDLE;
		VkPipelineLayout bloom_filter = VK_NULL_HANDLE;
	} pipeline_layouts{};

	struct
	{
		VkDescriptorSet object       = VK_NULL_HANDLE;
		VkDescriptorSet skybox       = VK_NULL_HANDLE;
		VkDescriptorSet composition  = VK_NULL_HANDLE;
		VkDescriptorSet bloom_filter = VK_NULL_HANDLE;
	} descriptor_sets{};

	struct
	{
		VkDescriptorSetLayout models       = VK_NULL_HANDLE;
		VkDescriptorSetLayout composition  = VK_NULL_HANDLE;
		VkDescriptorSetLayout bloom_filter = VK_NULL_HANDLE;
	} descriptor_set_layouts{};

	struct FrameBufferAttachment
	{
		VkImage        image{};
		VkDeviceMemory mem{};
		VkImageView    view{};
		VkFormat       format{};
		void           destroy(VkDevice device) const
		{
			vkDestroyImageView(device, view, nullptr);
			vkDestroyImage(device, image, nullptr);
			vkFreeMemory(device, mem, nullptr);
		}
	};
	struct FrameBuffer
	{
		int32_t               width{};
		int32_t               height{};
		VkFramebuffer         framebuffer{};
		FrameBufferAttachment color[2]{};
		FrameBufferAttachment depth{};
		VkRenderPass          render_pass{};
		VkSampler             sampler{};
	} offscreen{};

	struct
	{
		int32_t               width{};
		int32_t               height{};
		VkFramebuffer         framebuffer{};
		FrameBufferAttachment color[1]{};
		VkRenderPass          render_pass{};
		VkSampler             sampler{};
	} filter_pass{};

	std::vector<std::string> object_names{};

  private:
	// MODs:
	static std::string read_time_domain(VkTimeDomainEXT inputTimeDomain);        // this returns a human-readable info for what time domain corresponds to what
	void               update_time_domains();                                    // this extracts total number of time domain the (physical device has, and then sync the time domain EXT data to its vector
	void               update_timestamps();                                      // creates local timestamps information vector, update timestamps vector and deviation vector
	void               init_timeDomains_and_timestamps();                        // this initializes all time domain and timestamps related variables, vectors, and booleans
	void               get_optimal_time_domain();                                // this gets the optimal time domain which has the minimal value on its max deviation.
	void               tic(const std::string &input_tag = "Untagged");           // this marks the timestamp begin and partially updates the delta_timestamps
	void               toc(const std::string &input_tag = "Untagged");           // this marks the timestamp ends and updates the delta_timestamps

  public:
	CalibratedTimestamps();
	~CalibratedTimestamps() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;
	void create_attachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment);
	void prepare_offscreen_buffer();
	void load_assets();
	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	void setup_descriptor_sets();
	void prepare_pipelines();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void draw();
	bool prepare(vkb::Platform &platform) override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	bool resize(uint32_t width, uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_calibrated_timestamps();
