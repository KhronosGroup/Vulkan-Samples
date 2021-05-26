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
 * Loading a Basis Universal supercompressed texture (https://github.com/BinomialLLC/basis_universal) and transcoding it to a supported GPU texture format
 * Details on KTX 2.0 can be found at https://www.khronos.org/ktx/
 */

#pragma once

#include <ktx.h>
#include <vector>

#include "api_vulkan_sample.h"

class TextureCompressionBasisu : public ApiVulkanSample
{
  public:
	// Vertex layout for this example
	struct VertexStructure
	{
		float pos[3];
		float uv[2];
	};

	// Contains all Vulkan objects that are required to store and use a texture
	// Note that this repository contains a texture class (vulkan_texture.h) that encapsulates texture loading functionality in a class that is used in subsequent demos
	struct Texture
	{
		VkSampler      sampler;
		VkImage        image = VK_NULL_HANDLE;
		VkImageLayout  image_layout;
		VkDeviceMemory device_memory;
		VkImageView    view;
		uint32_t       width, height;
		uint32_t       mip_levels;
	} texture;

	std::unique_ptr<vkb::core::Buffer> vertex_buffer;
	std::unique_ptr<vkb::core::Buffer> index_buffer;
	uint32_t                           index_count;

	std::unique_ptr<vkb::core::Buffer> uniform_buffer_vs;

	struct
	{
		glm::mat4 projection;
		glm::mat4 model;
	} ubo_vs;

	VkPipeline            pipeline;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;

	// @todo: document
	std::vector<ktx_transcode_fmt_e> available_target_formats;
	std::vector<std::string>         available_target_formats_names;
	std::vector<std::string>         texture_file_names;

	// @todo: store some stats
	int32_t selected_transcode_target_format = 0;
	int32_t selected_input_texture = 0;
	float   transcode_time;

	TextureCompressionBasisu();
	~TextureCompressionBasisu();
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	bool         format_supported(VkFormat format);
	void         get_available_target_formats();
	void         transcode_texture(const std::string &input_file, ktx_transcode_fmt_e target_format);
	void         destroy_texture(Texture texture);
	void         build_command_buffers() override;
	void         draw();
	void         generate_quad();
	void         setup_descriptor_pool();
	void         setup_descriptor_set_layout();
	void         setup_descriptor_set();
	void         prepare_pipelines();
	void         prepare_uniform_buffers();
	void         update_uniform_buffers();
	bool         prepare(vkb::Platform &platform) override;
	virtual void render(float delta_time) override;
	virtual void view_changed() override;
	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;
};

std::unique_ptr<vkb::Application> create_texture_compression_basisu();
