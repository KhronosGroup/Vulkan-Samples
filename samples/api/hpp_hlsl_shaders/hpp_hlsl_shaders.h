/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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
 * Using HLSL shaders in Vulkan with the glslang library, using vulkan.hpp
 */

#pragma once

#include <ktx.h>

#include <glslang/Public/ShaderLang.h>

#include "common/vk_common.h"
#include "core/shader_module.h"

#include "hpp_api_vulkan_sample.h"

class HPPHlslShaders : public HPPApiVulkanSample
{
  public:
	HPPHlslShaders();
	~HPPHlslShaders() override;

  private:
	struct UBOVS
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 view_pos;
	};

	// Vertex layout for this example
	struct VertexStructure
	{
		float pos[3];
		float uv[2];
		float normal[3];
	};

  private:
	// from platform::HPPApplication
	bool prepare(vkb::platform::HPPPlatform &platform) override;

	// from HPPVulkanSample
	void request_gpu_features(vkb::core::HPPPhysicalDevice &gpu) override;

	// from HPPApiVulkanSample
	void build_command_buffers() override;
	void render(float delta_time) override;
	void view_changed() override;

	void                              draw();
	void                              generate_quad();
	void                              load_assets();
	vk::PipelineShaderStageCreateInfo load_hlsl_shader(const std::string &file, vk::ShaderStageFlagBits stage);
	void                              prepare_pipelines();
	void                              prepare_uniform_buffers();
	void                              setup_descriptor_pool();
	void                              setup_descriptor_set();
	void                              setup_descriptor_set_layout();
	void                              update_uniform_buffers();

  private:
	vk::DescriptorSet                     base_descriptor_set;
	vk::DescriptorSetLayout               base_descriptor_set_layout;
	std::unique_ptr<vkb::core::HPPBuffer> index_buffer;
	vk::Pipeline                          pipeline;
	vk::PipelineLayout                    pipeline_layout;
	vk::DescriptorSetLayout               sampler_descriptor_set_layout;
	HPPTexture                            texture;
	UBOVS                                 ubo_vs;
	std::unique_ptr<vkb::core::HPPBuffer> uniform_buffer_vs;
	std::unique_ptr<vkb::core::HPPBuffer> vertex_buffer;
};

std::unique_ptr<vkb::Application> create_hpp_hlsl_shaders();
