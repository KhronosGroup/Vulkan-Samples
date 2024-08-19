/* Copyright (c) 2024, Sascha Willems
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

class ShaderDebugPrintf : public ApiVulkanSample
{
  public:
	bool display_skysphere = true;

	struct
	{
		Texture skysphere;
	} textures;

	struct
	{
		std::unique_ptr<vkb::sg::SubMesh> skysphere;
		std::unique_ptr<vkb::sg::SubMesh> scene;
	} models;

	struct
	{
		std::unique_ptr<vkb::core::Buffer> matrices;
	} uniform_buffers;

	struct UBOVS
	{
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::mat4 skysphere_modelview;
		float     modelscale = 0.05f;
	} ubo_vs;

	struct
	{
		VkPipeline skysphere{VK_NULL_HANDLE};
		VkPipeline sphere{VK_NULL_HANDLE};
	} pipelines;
	VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};

	struct
	{
		VkDescriptorSet skysphere{VK_NULL_HANDLE};
		VkDescriptorSet sphere{VK_NULL_HANDLE};
	} descriptor_sets;
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};

	struct
	{
		glm::vec4 offset;
		glm::vec4 color;
		uint32_t  object_type{0};
	} push_const_block;

	VkDebugUtilsMessengerEXT debug_utils_messenger{VK_NULL_HANDLE};

	static std::string debug_output;

	VKAPI_ATTR static VkBool32 VKAPI_CALL debug_utils_message_callback(
	    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
	    VkDebugUtilsMessageTypeFlagsEXT             messageType,
	    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	    void                                       *pUserData);

	ShaderDebugPrintf();
	~ShaderDebugPrintf();
	virtual void                    request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void                            build_command_buffers() override;
	void                            load_assets();
	void                            setup_descriptor_pool();
	void                            setup_descriptor_set_layout();
	void                            setup_descriptor_sets();
	void                            prepare_pipelines();
	void                            prepare_uniform_buffers();
	void                            update_uniform_buffers();
	void                            draw();
	bool                            prepare(const vkb::ApplicationOptions &options) override;
	const std::vector<const char *> get_validation_layers() override;
	std::unique_ptr<vkb::Instance>  create_instance(bool headless) override;
	virtual void                    render(float delta_time) override;
	virtual void                    on_update_ui_overlay(vkb::Drawer &drawer) override;
	virtual bool                    resize(const uint32_t width, const uint32_t height) override;
};

std::unique_ptr<vkb::Application> create_shader_debugprintf();
