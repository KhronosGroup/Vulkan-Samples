/* Copyright (c) 2022, Holochip Corporation
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
  private:
	// Introduce Timestamps related variables:
	VkCalibratedTimestampInfoEXT timestampsInfo_device{};
	VkCalibratedTimestampInfoEXT timestampsInfo_queryPerformanceCount{};

	struct TimeInfo
	{
		uint64_t timestamp = 0;
		uint64_t deviation = 0;
	};

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

  private:
	// TODO: @JEREMY: (*) may want to add a timestamp "translate" function based on the runtime test results
	TimeInfo get_timestamp(VkCalibratedTimestampInfoEXT &inputTimeStampsInfo, uint32_t inputTimestampsCount = 1);        // This is to get then sync the timestamp and the screen max deviation

  public:
	CalibratedTimestamps();
	~CalibratedTimestamps() override;

	// MOD:
	void initialize();        // this is to initialize all VkCalibratedTimestampInfoEXT related variables

	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;
	void create_attachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment);
	void prepare_offscreen_buffer();
	void load_assets();
	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	void setup_descriptor_sets();

	// TODO: @JEREMY: (1) introduce timestamps return value here:
	void prepare_pipelines();

	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void update_params();
	void draw();
	bool prepare(vkb::Platform &platform) override;
	void render(float delta_time) override;

	// TODO: @JEREMY: (2) update timestamp information when finalizing the runtime test:
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

	bool resize(uint32_t width, uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_calibrated_timestamps();
