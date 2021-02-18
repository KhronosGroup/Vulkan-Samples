/* Copyright (c) 2021, Arm Limited and Contributors
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
#include <random>
#include <vector>

class DescriptorIndexing : public ApiVulkanSample
{
  public:
	DescriptorIndexing();
	~DescriptorIndexing();

  private:
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	virtual void render(float delta_time) override;
	virtual void build_command_buffers() override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
	virtual bool prepare(vkb::Platform &platform) override;

	void create_bindless_descriptors();
	void create_immutable_sampler_descriptor_set();
	void create_pipelines();

	struct DescriptorHeap
	{
		VkDescriptorSetLayout set_layout{};
		VkDescriptorPool      descriptor_pool{};
		VkDescriptorSet       descriptor_set_update_after_bind{};
		VkDescriptorSet       descriptor_set_nonuniform{};
	} descriptors;

	struct ImmutableSampler
	{
		VkSampler             sampler{};
		VkDescriptorSetLayout set_layout{};
		VkDescriptorPool      descriptor_pool{};
		VkDescriptorSet       descriptor_set{};
	} sampler;

	struct Pipelines
	{
		VkPipelineLayout pipeline_layout{};
		VkPipeline       update_after_bind{};
		VkPipeline       non_uniform_indexing{};
	} pipelines;

	struct TestImage
	{
		VkImage        image{};
		VkImageView    image_view{};
		VkDeviceMemory memory{};
	};
	std::vector<TestImage> test_images;
	void                   create_images();
	TestImage              create_image(const float rgb[3], unsigned image_seed);

	VkPhysicalDeviceDescriptorIndexingPropertiesEXT descriptor_indexing_properties{};
	std::default_random_engine                      rnd{42};
	std::uniform_real_distribution<float>           distribution{0.0f, 0.1f};
	uint32_t                                        descriptor_offset{};
	float                                           accumulated_time{};
};

std::unique_ptr<vkb::VulkanSample> create_descriptor_indexing();
