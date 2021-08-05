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

class RayQueries : public vkb::VulkanSample
{
public:
    void          request_gpu_features(vkb::PhysicalDevice &gpu) override;
    virtual void prepare_render_context() override;
    //void render(float delta_time) override;
    bool          prepare(vkb::Platform &platform) override;
    virtual void draw(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target) override;
    //virtual void draw_render(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target) override;
    //void          build_command_buffers() override;

private:
    uint32_t max_thread_count{1};
    vkb::sg::Camera *camera{nullptr};
    bool enable_shadows{false};
};



std::unique_ptr<vkb::VulkanSample> create_ray_queries();
