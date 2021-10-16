/* Copyright (c) 2021, Sascha Willems
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
 * Using HLSL shaders in Vulkan with the glslang library
 */

#pragma once

#include <ktx.h>

#include <glslang/Public/ShaderLang.h>

#include "common/vk_common.h"
#include "core/shader_module.h"

#include "api_vulkan_sample.h"

class HlslShaders : public ApiVulkanSample
{
  public:
	// Vertex layout for this example
	struct VertexStructure
	{
		float pos[3];
		float uv[2];
		float normal[3];
	};

	Texture texture;

	std::array<VkSampler, 2>       samplers{};
	int32_t                        selected_sampler = 0;
	std::array<VkDescriptorSet, 2> sampler_descriptor_sets{};

	std::unique_ptr<vkb::core::Buffer> vertex_buffer;
	std::unique_ptr<vkb::core::Buffer> index_buffer;
	uint32_t                           index_count;

	std::unique_ptr<vkb::core::Buffer> uniform_buffer_vs;

	struct
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 view_pos;
	} ubo_vs;

	VkPipeline       pipeline            = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout     = VK_NULL_HANDLE;
	VkDescriptorSet  base_descriptor_set = VK_NULL_HANDLE;

	VkDescriptorSetLayout base_descriptor_set_layout;
	VkDescriptorSetLayout sampler_descriptor_set_layout;

	HlslShaders();
	~HlslShaders() override;
	virtual void                    request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void                            build_command_buffers() override;
	void                            setup_samplers();
	void                            load_assets();
	void                            draw();
	void                            generate_quad();
	void                            setup_descriptor_pool();
	void                            setup_descriptor_set_layout();
	void                            setup_descriptor_set();
	void                            prepare_pipelines();
	void                            prepare_uniform_buffers();
	void                            update_uniform_buffers();
	bool                            prepare(vkb::Platform &platform) override;
	VkPipelineShaderStageCreateInfo load_hlsl_shader(const std::string &file, VkShaderStageFlagBits stage);
	virtual void                    render(float delta_time) override;
	virtual void                    view_changed() override;
	virtual void                    on_update_ui_overlay(vkb::Drawer &drawer) override;
};

std::unique_ptr<vkb::Application> create_hlsl_shaders();
