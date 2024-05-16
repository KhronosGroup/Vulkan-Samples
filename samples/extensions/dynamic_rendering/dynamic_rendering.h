/*
 * Copyright (c) 2021-2024, Holochip Corporation
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

class DynamicRendering : public ApiVulkanSample
{
  public:
	DynamicRendering();
	~DynamicRendering() override;

	bool prepare(const vkb::ApplicationOptions &options) override;

	void render(float delta_time) override;
	void build_command_buffers() override;
	void view_changed() override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;

  private:
	void load_assets();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void setup_descriptor_set_layout();
	void create_descriptor_sets();
	void create_descriptor_pool();
	void create_pipeline();
	void create_render_pass_non_dynamic();
	void draw();

	struct
	{
		Texture envmap;
	} textures;

	struct UBOVS
	{
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::mat4 skybox_modelview;
		float     modelscale = 0.05f;
	} ubo_vs;

	std::unique_ptr<vkb::sg::SubMesh>  skybox;
	std::unique_ptr<vkb::sg::SubMesh>  object;
	std::unique_ptr<vkb::core::Buffer> ubo;

	VkPipeline            model_pipeline{VK_NULL_HANDLE};
	VkPipeline            skybox_pipeline{VK_NULL_HANDLE};
	VkPipelineLayout      pipeline_layout{VK_NULL_HANDLE};
	VkDescriptorSet       descriptor_set{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
	VkDescriptorPool      descriptor_pool{VK_NULL_HANDLE};

#if VK_NO_PROTOTYPES
	PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR{VK_NULL_HANDLE};
	PFN_vkCmdEndRenderingKHR   vkCmdEndRenderingKHR{VK_NULL_HANDLE};
#endif
	bool enable_dynamic;
};

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_dynamic_rendering();
