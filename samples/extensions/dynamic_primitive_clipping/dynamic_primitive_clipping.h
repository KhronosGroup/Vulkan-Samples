/* Copyright (c) 2024, Mobica Limited
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
 * Rendering using primitive clipping configured by dynamic pipeline state
 */

#pragma once

#include "api_vulkan_sample.h"

class DynamicPrimitiveClipping : public ApiVulkanSample
{
  public:
	DynamicPrimitiveClipping();
	virtual ~DynamicPrimitiveClipping();

	// Override basic framework functionality
	bool prepare(const vkb::ApplicationOptions &options) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

	// Auxiliary functions
	void load_assets();
	void setup_layouts();
	void prepare_pipelines();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void setup_descriptor_pool();
	void setup_descriptor_sets();

  private:
	// vector of models rendered by sample
	struct Models
	{
		std::vector<std::unique_ptr<vkb::sg::SubMesh>> objects;
		std::vector<glm::mat4>                         transforms;
		int32_t                                        object_index = 0;
	} models;

	std::vector<std::string> model_names;
	std::vector<std::string> visualization_names;

	// sample parameters used on CPU side
	struct Params
	{
		bool    usePrimitiveClipping = true;
		bool    drawObject[2]        = {true, true};
		int32_t visualization        = 0;
		bool    useDepthClipping     = false;
	} params;

	// parameters sent to shaders through uniform buffers
	struct UBOVS
	{
		alignas(16) glm::mat4 projection;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 model;
		alignas(16) glm::vec4 colorTransformation;
		alignas(8) glm::ivec2 sceneTransformation;
		alignas(4) float usePrimitiveClipping;
	} ubo_positive, ubo_negative;

	struct
	{
		std::unique_ptr<vkb::core::Buffer> buffer_positive;
		std::unique_ptr<vkb::core::Buffer> buffer_negative;
	} uniform_buffers;

	// pipeline infrastructure
	struct
	{
		VkPipelineLayout models = VK_NULL_HANDLE;
	} pipeline_layouts;

	struct
	{
		VkDescriptorSetLayout models = VK_NULL_HANDLE;
	} descriptor_set_layouts;

	struct
	{
		VkDescriptorSet descriptor_positive = VK_NULL_HANDLE;
		VkDescriptorSet descriptor_negative = VK_NULL_HANDLE;
	} descriptor_sets;

	VkPipeline sample_pipeline{};
};

std::unique_ptr<vkb::VulkanSample> create_dynamic_primitive_clipping();
