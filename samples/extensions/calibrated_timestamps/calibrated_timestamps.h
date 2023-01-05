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
#include "core/image.h"
#include "rendering/render_frame.h"

class CalibratedTimestamps : public ApiVulkanSample
{
  public:
	Texture skybox_map{};

	struct Models
	{
		std::unique_ptr<vkb::sg::SubMesh> skybox = nullptr;
		std::unique_ptr<vkb::sg::SubMesh> object = nullptr;
		glm::mat4                         transform{};
	} models{};

	struct UniformBuffers
	{
		std::unique_ptr<vkb::core::Buffer> matrices = nullptr;
		std::unique_ptr<vkb::core::Buffer> params   = nullptr;
	} uniform_buffers{};

	struct UBOVS
	{
		glm::mat4 projection{};
		glm::mat4 model_view{};
		glm::mat4 skybox_model_view{};
		float     model_scale = 0.05f;
	} ubo_vs{};

	const struct UBOParams
	{
		float exposure = 1.0f;
	} ubo_params{};

	struct Pipelines
	{
		VkPipeline skybox      = VK_NULL_HANDLE;
		VkPipeline reflect     = VK_NULL_HANDLE;
		VkPipeline composition = VK_NULL_HANDLE;
	} pipelines{};

	struct PipelineLayouts
	{
		VkPipelineLayout models      = VK_NULL_HANDLE;
		VkPipelineLayout composition = VK_NULL_HANDLE;
	} pipeline_layouts{};

	struct DescriptorSets
	{
		VkDescriptorSet skybox      = VK_NULL_HANDLE;
		VkDescriptorSet object      = VK_NULL_HANDLE;
		VkDescriptorSet composition = VK_NULL_HANDLE;
	} descriptor_sets{};

	struct DescriptorSetLayouts
	{
		VkDescriptorSetLayout models      = VK_NULL_HANDLE;
		VkDescriptorSetLayout composition = VK_NULL_HANDLE;
	} descriptor_set_layouts{};

	struct FrameBufferAttachment
	{
		VkImage        image{};
		VkDeviceMemory mem{};
		VkImageView    view{};
		VkFormat       format{};
		const void     destroy(VkDevice device)
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

	struct FilterPass
	{
		int32_t               width{};
		int32_t               height{};
		VkFramebuffer         framebuffer{};
		FrameBufferAttachment color[1]{};
		VkRenderPass          render_pass{};
		VkSampler             sampler{};
	} filter_pass{};

	// TODO: @JEREMY *) Move it back to the top when finalized:
  private:
	// TODO: @JEREMY Legacy, remove it when new functions passed the run-time:
	VkCalibratedTimestampInfoEXT timestampsInfo_device{};
	VkCalibratedTimestampInfoEXT timestampsInfo_queryPerformanceCount{};
	struct TimeInfo
	{
		uint64_t timestamp = 0;
		uint64_t deviation = 0;
	};

	//MODs:
	uint32_t time_domain_count{};
	// this can be done locally, or by figuring out the size of time_domain vector, but since it will be calibrated anyways, therefore why not make it a clas variable
	std::vector<VkTimeDomainEXT> time_domains{};
	// this vector is initialized as empty, but it holds all time domains extracted from the current Instance
	std::vector<uint64_t> timestamps{};                       // timestamps vector
	std::vector<uint64_t> max_deviations{};                   // max deviations vector
	bool                  isTimeDomainUpdated = false;        // this is just to tell if time domain update has a VK_SUCCESS in the end
	bool                  isTimestampUpdated  = false;        // this is just to tell the GUI whether timestamps operation has a VK_SUCCESS in the end

  private:
	// MODs:
	std::string read_time_domain(VkTimeDomainEXT inputTimeDomain);
	// this returns a human-readable information for what time domain corresponds to what
	void update_time_domains();
	// this extracts total number of time domain the (physical) device has, and then sync the time domain EXT data to its vector
	void update_timestamps();
	// creates local timestamps information vector, update timestamps vector and deviation vector

	// TODO: @JEREMY Legacy, remove it when new functions passed the run-time:
	TimeInfo get_timestamp(VkCalibratedTimestampInfoEXT &inputTimeStampsInfo, uint32_t inputTimestampsCount = 1);
	// This is to get then sync the timestamp and the screen max deviation

  public:
	CalibratedTimestamps();
	~CalibratedTimestamps() override;

	// TODO: @JEREMY Legacy, remove it when new functions passed the run-time:
	void initialize();
	// this is to initialize all VkCalibratedTimestampInfoEXT related variables

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
	void update_params();
	void draw();
	bool prepare(vkb::Platform &platform) override;
	void render(float delta_time) override;

	// TODO: @JEREMY: 4-1) get a update timestamps button;
	//                4-2) display: a) time domain, b) timestamps, c) deviation
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

	bool resize(uint32_t width, uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_calibrated_timestamps();
