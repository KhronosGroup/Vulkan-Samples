/* Copyright (c) 2018-2026, Arm Limited and Contributors
 * Copyright (c) 2026, NVIDIA CORPORATION. All rights reserved.
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

#include "buffer_pool.h"
#include "rendering/subpasses/geometry_subpass.h"

// This value is per type of light that we feed into the shader
#define MAX_FORWARD_LIGHT_COUNT 8

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

struct alignas(16) ForwardLights
{
	vkb::rendering::Light directional_lights[MAX_FORWARD_LIGHT_COUNT];
	vkb::rendering::Light point_lights[MAX_FORWARD_LIGHT_COUNT];
	vkb::rendering::Light spot_lights[MAX_FORWARD_LIGHT_COUNT];
};

namespace rendering
{
namespace subpasses
{

/**
 * @brief This subpass is responsible for rendering a Scene
 */
template <vkb::BindingType bindingType>
class ForwardSubpass : public vkb::rendering::subpasses::GeometrySubpass<bindingType>
{
  public:
	using SceneType        = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::scene_graph::HPPScene, vkb::sg::Scene>::type;
	using ShaderSourceType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPShaderSource, vkb::ShaderSource>::type;

  public:
	/**
	 * @brief Constructs a subpass designed for forward rendering
	 * @param render_context Render context
	 * @param vertex_shader Vertex shader source
	 * @param fragment_shader Fragment shader source
	 * @param scene Scene to render on this subpass
	 * @param camera Camera used to look at the scene
	 */
	ForwardSubpass(vkb::rendering::RenderContext<bindingType> &render_context, ShaderSourceType &&vertex_shader, ShaderSourceType &&fragment_shader,
	               SceneType &scene, sg::Camera &camera);

	virtual ~ForwardSubpass() = default;

	// from vkb::rendering::Subpass
	void draw(vkb::core::CommandBuffer<bindingType> &command_buffer) override;
	void prepare() override;
};

using ForwardSubpassC   = ForwardSubpass<vkb::BindingType::C>;
using ForwardSubpassCpp = ForwardSubpass<vkb::BindingType::Cpp>;

// Member function definitions

template <vkb::BindingType bindingType>
inline ForwardSubpass<bindingType>::ForwardSubpass(vkb::rendering::RenderContext<bindingType> &render_context, ShaderSourceType &&vertex_source,
                                                   ShaderSourceType &&fragment_source, SceneType &scene_, sg::Camera &camera) :
    GeometrySubpass<bindingType>{render_context, std::move(vertex_source), std::move(fragment_source), scene_, camera}
{}

template <vkb::BindingType bindingType>
inline void ForwardSubpass<bindingType>::draw(vkb::core::CommandBuffer<bindingType> &command_buffer)
{
	this->template allocate_lights<ForwardLights>(this->get_scene().template get_components<sg::Light>(), MAX_FORWARD_LIGHT_COUNT);
	command_buffer.bind_lighting(this->get_lighting_state(), 0, 4);

	GeometrySubpass<bindingType>::draw(command_buffer);
}

template <vkb::BindingType bindingType>
inline void ForwardSubpass<bindingType>::prepare()
{
	auto &device = this->get_render_context_impl().get_device();
	for (auto &mesh : this->get_meshes_impl())
	{
		for (auto &sub_mesh : mesh->get_submeshes())
		{
			auto &variant     = sub_mesh->get_mut_shader_variant();
			auto &vert_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eVertex, this->get_vertex_shader_impl(), variant);
			auto &frag_module =
			    device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, this->get_fragment_shader_impl(), variant);
		}
	}
}

}        // namespace subpasses
}        // namespace rendering
}        // namespace vkb
