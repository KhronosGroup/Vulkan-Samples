/* Copyright (c) 2024-2026, Holochip Inc.
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
#include "scene_graph/node.h"
#include <chrono>
#include <functional>
#include <memory>
#include <vector>

namespace octomap
{
class OcTree;
}

namespace vkb
{
namespace scene_graph
{
template <BindingType bindingType>
class Scene;
using SceneC = Scene<BindingType::C>;
}        // namespace scene_graph

namespace sg
{
class SubMesh;
}        // namespace sg

}        // namespace vkb

class render_octomap : public ApiVulkanSample
{
  public:
	// Which of the three demo views is currently being rendered.
	enum class ViewState
	{
		Octomap,
		GLTFRegular,
		GLTFSplats
	};

	render_octomap();
	~render_octomap() override;
	void build_cubes();
	void build_command_buffers() override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void render(float delta_time) override;
	void input_event(const vkb::InputEvent &input_event) override;
	void create_octomap_pipeline(VkRenderPass renderPass);
	void create_gltf_pipeline(VkRenderPass renderPass);
	void create_splat_pipeline(VkRenderPass renderPass);
	void prepare_ubo();
	void update_ubo();
	void generate_master_cube();
	void setup_vertex_descriptions();
	bool resize(const uint32_t width, const uint32_t height) override;

	// View state handling
	void on_view_state_changed(ViewState new_state);
	void load_gltf_scene(const std::string &filename);
	void load_gaussian_splats_scene(const std::string &filename);
	void load_gaussian_splats_data(const std::string &filename);

  private:
	struct
	{
		VkPipelineVertexInputStateCreateInfo           input_state;
		std::vector<VkVertexInputBindingDescription>   binding_descriptions;
		std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
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
	std::unique_ptr<vkb::core::BufferC> vertex_buffer;
	std::unique_ptr<vkb::core::BufferC> index_buffer;
	std::unique_ptr<vkb::core::BufferC> instance_buffer;
	std::unique_ptr<vkb::core::BufferC> uniform_buffer_vs;
	uint32_t                            index_count{0};
	VkPipelineCache                     pipeline_cache{VK_NULL_HANDLE};
	VkPipelineLayout                    pipeline_layout{VK_NULL_HANDLE};
	VkPipeline                          pipeline{VK_NULL_HANDLE};
	VkDescriptorPool                    descriptor_pool{VK_NULL_HANDLE};
	VkDescriptorSetLayout               descriptor_set_layout{VK_NULL_HANDLE};
	VkDescriptorSet                     descriptor_set{VK_NULL_HANDLE};
	octomap::OcTree                    *map{nullptr};
	unsigned int                        max_tree_depth{16};

	float                                              z_min{0.0f};
	float                                              z_max{0.0f};
	uint64_t                                           last_map_build_size{0};
	std::chrono::time_point<std::chrono::system_clock> last_build_time;
	std::vector<InstanceData>                          instances;

	// Numpad look state (KP_4/6 = yaw left/right, KP_8/2 = pitch up/down)
	struct
	{
		bool left  = false;        // KP_4
		bool right = false;        // KP_6
		bool up    = false;        // KP_8
		bool down  = false;        // KP_2
	} numpad_look;

	// View state management
	ViewState                                 current_view_state = ViewState::Octomap;
	std::unique_ptr<vkb::scene_graph::SceneC> gltf_scene;

	struct GltfNodeDraw
	{
		vkb::scene_graph::NodeC *node{nullptr};
		vkb::sg::SubMesh        *sub_mesh{nullptr};
	};
	std::vector<GltfNodeDraw> gltf_nodes;

	VkPipelineLayout gltf_pipeline_layout{VK_NULL_HANDLE};
	VkPipeline       gltf_pipeline{VK_NULL_HANDLE};

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
	std::vector<SplatInstance>          splat_instances_cpu;
	std::unique_ptr<vkb::core::BufferC> splat_instance_buffer;
	uint32_t                            splat_count{0};

	struct
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec2 viewport;
		float     focalX;
		float     focalY;
	} splat_ubo;
	std::unique_ptr<vkb::core::BufferC> splat_uniform_buffer;
	VkDescriptorPool                    splat_descriptor_pool{VK_NULL_HANDLE};
	VkDescriptorSetLayout               splat_descriptor_set_layout{VK_NULL_HANDLE};
	VkDescriptorSet                     splat_descriptor_set{VK_NULL_HANDLE};
	VkPipelineLayout                    splat_pipeline_layout{VK_NULL_HANDLE};
	VkPipeline                          splat_pipeline{VK_NULL_HANDLE};
};

std::unique_ptr<vkb::Application> create_render_octomap();

#endif        // RENDER_OCTOMAP_H
