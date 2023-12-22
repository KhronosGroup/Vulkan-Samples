/* Copyright (c) 2023, Arm Limited and Contributors
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

class DynamicDepthClipping : public ApiVulkanSample
{
  public:
	DynamicDepthClipping();
	virtual ~DynamicDepthClipping();

	// Create pipeline
	void setup_descriptor_set_layout();
	void prepare_pipelines();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void setup_descriptor_pool();
	void setup_descriptor_sets();

	// Override basic framework functionality
	bool prepare(const vkb::ApplicationOptions &options) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;


  private:
	// Sample specific data
	VkPipeline       sample_pipeline{};
	VkPipelineLayout sample_pipeline_layout{};

	struct Model
	{
		std::unique_ptr<vkb::sg::SubMesh> object;
	} models;

	struct Params
	{
		bool    useClipping;
		bool	drawObject[2];
		int32_t visualization;
	} params;

	struct
	{
		std::unique_ptr<vkb::core::Buffer> buffer_positive;
		std::unique_ptr<vkb::core::Buffer> buffer_negative;
	} uniform_buffers;

	struct UBOVS
	{
		glm::mat4  projection;
		glm::mat4  modelview;
		glm::vec4  colorTransformation;
		glm::ivec2 sceneTransformation;
	} ubo_positive, ubo_negative;

	struct
	{
		VkPipelineLayout models;
	} pipeline_layouts;

	struct
	{
		VkDescriptorSetLayout models;
	} descriptor_set_layouts;

	struct
	{
		VkDescriptorSet descriptor_positive;
		VkDescriptorSet descriptor_negative;
	} descriptor_sets;

	std::vector<std::string> visualization_names;
};

std::unique_ptr<vkb::VulkanSample> create_dynamic_depth_clipping();
