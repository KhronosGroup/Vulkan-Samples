/* Copyright (c) 2021, Holochip Corporation
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
 * Calculate shadows by extending a standard pipeline with ray queries
 */

#pragma once

#include "api_vulkan_sample.h"

namespace vkb
{
namespace sg
{
class Scene;
class Node;
class Mesh;
class SubMesh;
class Camera;
}        // namespace sg
}

class RayQueries : public ApiVulkanSample
{
public:
    RayQueries();
    ~RayQueries();
    void          request_gpu_features(vkb::PhysicalDevice &gpu) override;
    virtual void prepare_render_context() override;
    virtual void render(float delta_time) override;
    bool          prepare(vkb::Platform &platform) override;
    virtual void draw(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target) override;
    //virtual void draw_render(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target) override;
    //void          build_command_buffers() override;

private:
    struct GlobalUniform
    {
        alignas(16) glm::mat4x4 model;
        alignas(16) glm::mat4x4 view_proj;
        alignas(4) glm::vec3 camera_position;
        alignas(4) glm::vec3 light_position;
    } global_uniform;

    struct Vertex
    {
        alignas(4) glm::vec3 position;
        alignas(4) glm::vec3 normal;
    };

    struct Model
    {
        std::vector<Vertex> vertices;
        std::vector<std::array<uint32_t, 3>> indices;
    } model;


    // memory
    std::unique_ptr<vkb::core::Buffer> vertex_buffer{nullptr};
    std::unique_ptr<vkb::core::Buffer> index_buffer{nullptr};
    std::unique_ptr<vkb::core::Buffer> uniform_buffer{nullptr};

    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkDescriptorSet descriptor_set;
    VkDescriptorSetLayout descriptor_set_layout;

    void build_command_buffers() override;
    void create_uniforms();
    void load_scene();
    void setup_descriptor_pool();
    void setup_descriptor_set_layout();
    void setup_descriptor_set();
    void prepare_pipelines();
    void update_uniform_buffers();

    uint32_t max_thread_count{1};
    vkb::Camera camera;
    bool enable_shadows{false};
};



std::unique_ptr<vkb::VulkanSample> create_ray_queries();
