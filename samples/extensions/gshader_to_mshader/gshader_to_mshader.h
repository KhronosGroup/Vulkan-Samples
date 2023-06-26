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
#include "shader_compiler.h"
#include "gltf_loader.h"

class GshaderToMshader : public ApiVulkanSample
{
  public:
	bool showNormalsGeo  = false;
	bool showNormalsMesh = false;

	struct UBOVS
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	std::array<UBOVS, 3> ubos;

	VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};

	VkPipeline model_pipeline{VK_NULL_HANDLE};
	VkPipeline geometry_pipeline{VK_NULL_HANDLE};
	VkPipeline mesh_pipeline{VK_NULL_HANDLE};

	std::unique_ptr<vkb::sg::SubMesh> object;
	std::unique_ptr<vkb::sg::SubMesh> storage_buffer_object;

	std::unique_ptr<vkb::core::Buffer> uniform_buffer_vs;
	std::unique_ptr<vkb::core::Buffer> uniform_buffer_gs;
	std::unique_ptr<vkb::core::Buffer> uniform_buffer_ms;
	std::unique_ptr<vkb::core::Buffer> meshlet_buffer;

	VkDescriptorSet       descriptor_set{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};

	GshaderToMshader();
	~GshaderToMshader();

	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	virtual void render(float delta_time) override;
	virtual void build_command_buffers() override;
	virtual bool prepare(const vkb::ApplicationOptions &options) override;
	void         prepare_pipelines();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	void         draw();

	void load_assets();
	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	void setup_descriptor_sets();

	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
	virtual bool resize(const uint32_t width, const uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_gshader_to_mshader();
