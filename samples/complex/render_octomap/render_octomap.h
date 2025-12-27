/* Copyright (c) 2024-2025, Holochip Inc.
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
#ifndef RENDER_OCTOMAP_H
#define RENDER_OCTOMAP_H

#include "api_vulkan_sample.h"
#include <ImGUIUtil.h>
#include <Screens/MapView.h>
#include "scene_graph/node.h"
#include <chrono>
#include <functional>
#include <memory>
#include <vector>

namespace octomap {
	class OcTree;
}

namespace vkb {
namespace sg {
	class Scene;
	class SubMesh;
}

}

class render_octomap : public ApiVulkanSample
{
  public:
	render_octomap();
	~render_octomap() override;
	void BuildCubes();
	void build_command_buffers() override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void update_overlay(float delta_time, const std::function<void()> &additional_ui) override;
	void render(float delta_time) override;
	void input_event(const vkb::InputEvent &input_event) override;
	void createPipelines(VkRenderPass renderPass);
	void create_gltf_pipeline(VkRenderPass renderPass);
	void create_splat_pipeline(VkRenderPass renderPass);
	void prepareUBO();
	void updateUBO();
	void generateMasterCube();
	void SetupVertexDescriptions();
	bool resize(const uint32_t width, const uint32_t height) override;

	// View state handling
	void onViewStateChanged(MapView::ViewState newState);
	void loadGLTFScene(const std::string &filename);
	void loadGaussianSplatsScene(const std::string &filename);
	void loadGaussianSplatsData(const std::string &filename);

  private:
	struct
	{
		VkPipelineVertexInputStateCreateInfo           inputState;
		std::vector<VkVertexInputBindingDescription>   bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;
	struct InstanceData
	{
		float pos[3];
		float col[4];
		float scale{0.0f};
	};
	struct
	{
		glm::mat4 projection;
		glm::mat4 camera;
	} uboVS;
	std::unique_ptr<vkb::core::BufferC> vertexBuffer;
	std::unique_ptr<vkb::core::BufferC> indexBuffer;
	std::unique_ptr<vkb::core::BufferC> instanceBuffer;
	std::unique_ptr<vkb::core::BufferC> uniformBufferVS;
	uint32_t                            indexCount;
	VkPipelineCache                     pipelineCache;
	VkPipelineLayout                    pipelineLayout;
	VkPipeline                          pipeline;
	VkDescriptorPool                    descriptorPool;
	VkDescriptorSetLayout               descriptorSetLayout;
	VkDescriptorSet                     descriptorSet;
	octomap::OcTree                    *map;
	ImGUIUtil                          *gui;
	unsigned int                        mMaxTreeDepth;

	float                                              m_zMin;
	float                                              m_zMax;
	uint64_t                                           lastMapBuildSize;
	std::chrono::time_point<std::chrono::system_clock> lastBuildTime;
	std::vector<InstanceData>                          instances;

	// View state management
	MapView::ViewState                   currentViewState = MapView::ViewState::Octomap;
	std::unique_ptr<vkb::sg::Scene>      gltfScene;
	std::unique_ptr<vkb::sg::Scene>      splatsScene;

	struct GltfNodeDraw
	{
		vkb::scene_graph::NodeC *node{nullptr};
		vkb::sg::SubMesh        *sub_mesh{nullptr};
	};
	std::vector<GltfNodeDraw>            gltf_nodes;

	VkPipelineLayout                     gltf_pipeline_layout{VK_NULL_HANDLE};
	VkPipeline                           gltf_pipeline{VK_NULL_HANDLE};

	// Gaussian splats rendering
	struct SplatInstance
	{
		float pos[3];
		float rot[4];
		float scale[3];
		float opacity;
		float color[3];
		float _pad;
	};
	std::unique_ptr<vkb::core::BufferC>  splat_instance_buffer;
	uint32_t                             splat_count{0};

	struct
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec2 viewport;
		float     focalX;
		float     focalY;
	} splat_ubo;
	std::unique_ptr<vkb::core::BufferC>  splat_uniform_buffer;
	VkDescriptorPool                     splat_descriptor_pool{VK_NULL_HANDLE};
	VkDescriptorSetLayout                splat_descriptor_set_layout{VK_NULL_HANDLE};
	VkDescriptorSet                      splat_descriptor_set{VK_NULL_HANDLE};
	VkPipelineLayout                     splat_pipeline_layout{VK_NULL_HANDLE};
	VkPipeline                           splat_pipeline{VK_NULL_HANDLE};
};

std::unique_ptr<vkb::Application> create_render_octomap();

#endif //RENDER_OCTOMAP_H
