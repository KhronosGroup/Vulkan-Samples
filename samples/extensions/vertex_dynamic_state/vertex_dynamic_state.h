/* Copyright (c) 2022, Mobica Limited
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

class vertex_dynamic_state : public ApiVulkanSample
{
  public:
	struct
	{
		Texture envmap;
	} textures;

	struct UBOVS
	{
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::mat4 skybox_modelview;
		float     modelscale = 0.15f;
	} ubo_vs;

	struct SampleVertex
	{
		glm::vec3 pos;
		glm::vec3 shaderUnusableData; /* Placeholder for generate offset between position and normal data */
		glm::vec3 normal;
	};

	struct Cube
	{
		std::unique_ptr<vkb::core::Buffer> vertices;
		std::unique_ptr<vkb::core::Buffer> indices;
		uint32_t                           index_count;
	} cube;

	typedef enum
	{
		VERTEX_DYNAMIC_STATE_USE_FRAMEWORK_VERTEX_STRUCT,
		VERTEX_DYNAMIC_STATE_USE_EXT_VERTEX_STRUCT
	} vertexDynamicStateVertexStruct_t;

	VkPipelineLayout                                   pipeline_layout{VK_NULL_HANDLE};
	VkPipeline                                         model_pipeline{VK_NULL_HANDLE};
	VkPipeline                                         skybox_pipeline{VK_NULL_HANDLE};
	VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT vertex_input_features{};
	VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT graphics_pipeline_library{};
	VkVertexInputBindingDescription2EXT                vertex_bindings_description_ext{};
	VkVertexInputAttributeDescription2EXT              vertex_attribute_description_ext[2]{};

	VkDescriptorSet       descriptor_set{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
	VkDescriptorPool      descriptor_pool{VK_NULL_HANDLE};

	std::unique_ptr<vkb::sg::SubMesh>  skybox;
	std::unique_ptr<vkb::sg::SubMesh>  object;
	std::unique_ptr<vkb::core::Buffer> ubo;

	vertex_dynamic_state();
	~vertex_dynamic_state();

	virtual void render(float delta_time) override;
	virtual void build_command_buffers() override;
	virtual bool prepare(vkb::Platform &platform) override;
	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;

	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void create_pipeline();
	void draw();

	void load_assets();
	void create_descriptor_pool();
	void setup_descriptor_set_layout();
	void create_descriptor_sets();

	void model_data_creation();
	void draw_created_model(VkCommandBuffer commandBuffer);
	void change_vertex_input_data(vertexDynamicStateVertexStruct_t variant);

#if VK_NO_PROTOTYPES
	PFN_vkCmdSetVertexInputEXT vkCmdSetVertexInputEXT{VK_NULL_HANDLE};
#endif
};

std::unique_ptr<vkb::VulkanSample> create_vertex_dynamic_state();
