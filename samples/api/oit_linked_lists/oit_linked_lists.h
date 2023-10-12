/* Copyright (c) 2023, Maximilien Dagois
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

#include "rendering/render_pipeline.h"
#include "scene_graph/components/camera.h"
#include "api_vulkan_sample.h"

class OITLinkedLists : public ApiVulkanSample
{
  public:
	OITLinkedLists();
	~OITLinkedLists();

	bool prepare(const vkb::ApplicationOptions &options) override;
	void render(float delta_time) override;
	void build_command_buffers() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;

  private:
	void load_assets();
	void create_resources();
	void create_descriptors();
	void create_pipelines();

	void update_scene_constants();
	void fill_object_data();

  private:
	enum
	{
		kObjectRowCount = 4,
		kObjectColumnCount = 4,
		kObjectLayerCount = 4,
		kObjectCount = kObjectRowCount * kObjectColumnCount * kObjectLayerCount,
	};

	struct SceneConstants
	{
		glm::mat4 projection;
		glm::mat4 view;
	};

	struct ObjectDesc
	{
		glm::mat4 model;
		glm::vec4 color;
	};

  private:
	std::unique_ptr<vkb::sg::SubMesh> object;
	std::unique_ptr<vkb::core::Buffer> scene_constants;
	std::unique_ptr<vkb::core::Buffer> object_desc;
	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout;
    VkPipeline combine_pipeline;
};

std::unique_ptr<vkb::VulkanSample> create_oit_linked_lists();
