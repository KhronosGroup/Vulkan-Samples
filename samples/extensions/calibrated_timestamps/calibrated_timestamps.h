/* Copyright (c) 2023-2024, Holochip Corporation
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

static std::string time_domain_to_string(VkTimeDomainEXT input_time_domain);        // this returns a string the input time domain Enum value

class CalibratedTimestamps : public ApiVulkanSample
{
  private:
	bool                         is_time_domain_init = false;        // this is just to tell if time domain update has a VK_SUCCESS in the end
	std::vector<VkTimeDomainEXT> time_domains{};                     // this holds all time domains extracted from the current Instance
	std::vector<uint64_t>        timestamps{};                       // timestamps vector
	std::vector<uint64_t>        max_deviations{};                   // max deviations vector

	struct
	{
		int             index = 0;
		VkTimeDomainEXT timeDomainEXT{};
	} selected_time_domain{};

	struct DeltaTimestamp
	{
		uint64_t    begin = 0;
		uint64_t    end   = 0;
		uint64_t    delta = 0;
		std::string tag   = "Untagged";
	};
	std::vector<VkCalibratedTimestampInfoEXT>       timestamps_info{};        // This vector is essential to vkGetCalibratedTimestampsEXT, and only need to be registered once.
	std::unordered_map<std::string, DeltaTimestamp> delta_timestamps{};

  public:
	bool bloom          = true;
	bool display_skybox = true;

	struct
	{
		Texture environment_map{};
	} textures{};

	struct
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

	struct
	{
		glm::mat4 projection{};
		glm::mat4 model_view{};
		glm::mat4 skybox_model_view{};
		glm::mat4 inverse_modelview{};
		float     model_scale = 0.05f;
	} ubo_vs{};

	struct
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
		VkImage        image  = VK_NULL_HANDLE;
		VkDeviceMemory mem    = VK_NULL_HANDLE;
		VkImageView    view   = VK_NULL_HANDLE;
		VkFormat       format = VK_FORMAT_UNDEFINED;
		void           destroy(VkDevice device) const
		{
			vkDestroyImageView(device, view, nullptr);
			vkDestroyImage(device, image, nullptr);
			vkFreeMemory(device, mem, nullptr);
		}
	};

	struct
	{
		int32_t               width       = 0;
		int32_t               height      = 0;
		VkFramebuffer         framebuffer = VK_NULL_HANDLE;
		FrameBufferAttachment color[2]{};
		FrameBufferAttachment depth{};
		VkRenderPass          render_pass = VK_NULL_HANDLE;
		VkSampler             sampler     = VK_NULL_HANDLE;
	} offscreen{};

	struct
	{
		int32_t               width       = 0;
		int32_t               height      = 0;
		VkFramebuffer         framebuffer = VK_NULL_HANDLE;
		FrameBufferAttachment color[1]{};
		VkRenderPass          render_pass = VK_NULL_HANDLE;
		VkSampler             sampler     = VK_NULL_HANDLE;
	} filter_pass{};

	std::vector<std::string> object_names{};

  private:
	void     get_time_domains();                                                 // this extracts total number of time domain the (physical device has, and then sync the time domain EXT data to its vector
	VkResult get_timestamps();                                                   // this creates local timestamps information vector, update timestamps vector and deviation vector
	void     get_device_time_domain();                                           // this gets the optimal time domain which has the minimal value on its max deviation.
	void     timestamps_begin(const std::string &input_tag = "Untagged");        // this marks the timestamp begin and partially updates the delta_timestamps
	void     timestamps_end(const std::string &input_tag = "Untagged");          // this marks the timestamp ends and updates the delta_timestamps

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
	bool prepare(const vkb::ApplicationOptions &options) override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	bool resize(uint32_t width, uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_calibrated_timestamps();
