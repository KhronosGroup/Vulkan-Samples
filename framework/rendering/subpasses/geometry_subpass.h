/* Copyright (c) 2019-2026, Arm Limited and Contributors
 * Copyright (c) 2025-2026, NVIDIA CORPORATION. All rights reserved.
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

#include "core/command_buffer.h"
#include "rendering/render_context.h"
#include "rendering/subpass.h"
#include "scene_graph/components/aabb.h"
#include "scene_graph/components/hpp_mesh.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/sub_mesh.h"
#include "scene_graph/hpp_scene.h"
#include "scene_graph/scene.h"

namespace vkb
{
namespace sg
{
class Mesh;
class Node;
class Camera;
}        // namespace sg

/**
 * @brief Global uniform structure for base shader
 */
struct alignas(16) GlobalUniform
{
	glm::mat4 model;
	glm::mat4 camera_view_proj;
	glm::vec3 camera_position;
};

/**
 * @brief PBR material uniform for base shader
 */
struct PBRMaterialUniform
{
	glm::vec4 base_color_factor;
	float     metallic_factor;
	float     roughness_factor;
};

namespace rendering
{
namespace subpasses
{
namespace
{
// type trait to get the default value for request_command_buffer
template <typename T>
struct DefaultFrontFaceTypeValue;
template <>
struct DefaultFrontFaceTypeValue<vk::FrontFace>
{
	static constexpr vk::FrontFace value = vk::FrontFace::eCounterClockwise;
};
template <>
struct DefaultFrontFaceTypeValue<VkFrontFace>
{
	static constexpr VkFrontFace value = VK_FRONT_FACE_COUNTER_CLOCKWISE;
};
}        // namespace

/**
 * @brief This subpass is responsible for rendering a Scene
 */
template <vkb::BindingType bindingType>
class GeometrySubpass : public vkb::rendering::Subpass<bindingType>
{
  public:
	using FrontFaceType = typename std::conditional<bindingType == BindingType::Cpp, vk::FrontFace, VkFrontFace>::type;

	using MeshType           = typename std::conditional<bindingType == BindingType::Cpp, vkb::scene_graph::components::HPPMesh, vkb::sg::Mesh>::type;
	using PipelineLayoutType = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPPipelineLayout, vkb::PipelineLayout>::type;
	using RasterizationStateType =
	    typename std::conditional<bindingType == BindingType::Cpp, vkb::rendering::HPPRasterizationState, vkb::RasterizationState>::type;
	using SceneType        = typename std::conditional<bindingType == BindingType::Cpp, vkb::scene_graph::HPPScene, vkb::sg::Scene>::type;
	using ShaderModuleType = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPShaderModule, vkb::ShaderModule>::type;
	using ShaderSourceType = typename std::conditional<bindingType == BindingType::Cpp, vkb::core::HPPShaderSource, vkb::ShaderSource>::type;
	using SubMeshType      = typename std::conditional<bindingType == BindingType::Cpp, vkb::scene_graph::components::HPPSubMesh, vkb::sg::SubMesh>::type;

  public:
	/**
	 * @brief Constructs a subpass for the geometry pass of Deferred rendering
	 * @param render_context Render context
	 * @param vertex_shader Vertex shader source
	 * @param fragment_shader Fragment shader source
	 * @param scene Scene to render on this subpass
	 * @param camera Camera used to look at the scene
	 */
	GeometrySubpass(vkb::rendering::RenderContext<bindingType> &render_context, ShaderSourceType &&vertex_shader, ShaderSourceType &&fragment_shader,
	                SceneType &scene, sg::Camera &camera);

	virtual ~GeometrySubpass() = default;

	// from vkb::rendering::Subpass
	virtual void prepare() override;

	/**
	 * @brief Record draw commands
	 */
	virtual void draw(vkb::core::CommandBuffer<bindingType> &command_buffer) override;

	/**
	 * @brief Thread index to use for allocating resources
	 */
	void set_thread_index(uint32_t index);

  protected:
	void                           draw_submesh(vkb::core::CommandBuffer<bindingType> &command_buffer, SubMeshType &sub_mesh,
	                                            FrontFaceType front_face = DefaultFrontFaceTypeValue<FrontFaceType>::value);
	virtual void                   draw_submesh_command(vkb::core::CommandBuffer<bindingType> &command_buffer, SubMeshType &sub_mesh);
	vkb::sg::Camera const         &get_camera() const;
	std::vector<MeshType *> const &get_meshes() const;
	RasterizationStateType const  &get_rasterization_state() const;
	SceneType const               &get_scene() const;

	/**
	 * @brief Sorts objects based on distance from camera and classifies them
	 *        into opaque and transparent in the arrays provided
	 */
	void get_sorted_nodes(std::multimap<float, std::pair<vkb::scene_graph::Node<bindingType> *, SubMeshType *>> &opaque_nodes,
	                      std::multimap<float, std::pair<vkb::scene_graph::Node<bindingType> *, SubMeshType *>> &transparent_nodes);

	uint32_t                    get_thread_index() const;
	void                        set_rasterization_state(const RasterizationStateType &rasterization_state);
	virtual PipelineLayoutType &prepare_pipeline_layout(vkb::core::CommandBuffer<bindingType> &command_buffer,
	                                                    const std::vector<ShaderModuleType *> &shader_modules);
	virtual void prepare_pipeline_state(vkb::core::CommandBuffer<bindingType> &command_buffer, FrontFaceType front_face, bool double_sided_material);
	virtual void prepare_push_constants(vkb::core::CommandBuffer<bindingType> &command_buffer, SubMeshType &sub_mesh);
	virtual void update_uniform(vkb::core::CommandBuffer<bindingType> &command_buffer, vkb::scene_graph::Node<bindingType> &node, size_t thread_index);

  protected:
	std::vector<vkb::scene_graph::components::HPPMesh *> const &get_meshes_impl() const;

  private:
	void draw_impl(vkb::core::CommandBufferCpp &command_buffer);
	void draw_submesh_impl(vkb::core::CommandBufferCpp &command_buffer, vkb::scene_graph::components::HPPSubMesh &sub_mesh,
	                       vk::FrontFace front_face = vk::FrontFace::eCounterClockwise);
	void get_sorted_nodes_impl(std::multimap<float, std::pair<vkb::scene_graph::NodeCpp *, vkb::scene_graph::components::HPPSubMesh *>> &opaque_nodes,
	                           std::multimap<float, std::pair<vkb::scene_graph::NodeCpp *, vkb::scene_graph::components::HPPSubMesh *>> &transparent_nodes);
	vkb::core::HPPPipelineLayout &prepare_pipeline_layout_impl(vkb::core::CommandBufferCpp                     &command_buffer,
	                                                           const std::vector<vkb::core::HPPShaderModule *> &shader_modules);
	void         prepare_pipeline_state_impl(vkb::core::CommandBufferCpp &command_buffer, vk::FrontFace front_face, bool double_sided_material);
	virtual void prepare_push_constants_impl(vkb::core::CommandBufferCpp &command_buffer, vkb::scene_graph::components::HPPSubMesh &sub_mesh);
	void         update_uniform_impl(vkb::core::CommandBufferCpp &command_buffer, vkb::scene_graph::NodeCpp &node, size_t thread_index);

  private:
	vkb::rendering::HPPRasterizationState                base_rasterization_state;
	vkb::sg::Camera                                     &camera;
	std::vector<vkb::scene_graph::components::HPPMesh *> meshes;
	vkb::scene_graph::HPPScene                          *scene;
	uint32_t                                             thread_index = 0;
};

using GeometrySubpassC   = GeometrySubpass<vkb::BindingType::C>;
using GeometrySubpassCpp = GeometrySubpass<vkb::BindingType::Cpp>;

// Member function definitions

template <vkb::BindingType bindingType>
inline GeometrySubpass<bindingType>::GeometrySubpass(vkb::rendering::RenderContext<bindingType> &render_context, ShaderSourceType &&vertex_source,
                                                     ShaderSourceType &&fragment_source, SceneType &scene_, sg::Camera &camera) :
    Subpass<bindingType>{render_context, std::move(vertex_source), std::move(fragment_source)}, camera{camera}
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		scene = &scene_;
	}
	else
	{
		scene = reinterpret_cast<vkb::scene_graph::HPPScene *>(&scene_);
	}
	meshes = scene->get_components<vkb::scene_graph::components::HPPMesh>();
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::draw(vkb::core::CommandBuffer<bindingType> &command_buffer)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		draw_impl(command_buffer);
	}
	else
	{
		draw_impl(reinterpret_cast<vkb::core::CommandBufferCpp &>(command_buffer));
	}
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::draw_impl(vkb::core::CommandBufferCpp &command_buffer)
{
	std::multimap<float, std::pair<vkb::scene_graph::NodeCpp *, vkb::scene_graph::components::HPPSubMesh *>> opaque_nodes;
	std::multimap<float, std::pair<vkb::scene_graph::NodeCpp *, vkb::scene_graph::components::HPPSubMesh *>> transparent_nodes;

	get_sorted_nodes_impl(opaque_nodes, transparent_nodes);

	// Draw opaque objects in front-to-back order
	{
		vkb::core::HPPScopedDebugLabel opaque_debug_label{command_buffer, "Opaque objects"};

		for (auto node_it = opaque_nodes.begin(); node_it != opaque_nodes.end(); node_it++)
		{
			if constexpr (bindingType == vkb::BindingType::Cpp)
			{
				update_uniform(command_buffer, *node_it->second.first, thread_index);
			}
			else
			{
				update_uniform(reinterpret_cast<vkb::core::CommandBufferC &>(command_buffer),
				               reinterpret_cast<vkb::scene_graph::NodeC &>(*node_it->second.first), thread_index);
			}

			// Invert the front face if the mesh was flipped
			const auto   &scale      = node_it->second.first->get_transform().get_scale();
			bool          flipped    = scale.x * scale.y * scale.z < 0;
			vk::FrontFace front_face = flipped ? vk::FrontFace::eClockwise : vk::FrontFace::eCounterClockwise;

			draw_submesh_impl(command_buffer, *node_it->second.second, front_face);
		}
	}

	if (!transparent_nodes.empty())
	{
		// Enable alpha blending
		vkb::rendering::HPPColorBlendAttachmentState color_blend_attachment{.blend_enable           = true,
		                                                                    .src_color_blend_factor = vk::BlendFactor::eSrcAlpha,
		                                                                    .dst_color_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha,
		                                                                    .src_alpha_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha};

		vkb::rendering::HPPColorBlendState color_blend_state{};
		color_blend_state.attachments.assign(this->get_output_attachments().size(), color_blend_attachment);

		command_buffer.set_color_blend_state(color_blend_state);
		command_buffer.set_depth_stencil_state(this->get_depth_stencil_state_impl());

		// Draw transparent objects in back-to-front order
		{
			vkb::core::HPPScopedDebugLabel transparent_debug_label{command_buffer, "Transparent objects"};

			for (auto node_it = transparent_nodes.rbegin(); node_it != transparent_nodes.rend(); node_it++)
			{
				if constexpr (bindingType == vkb::BindingType::Cpp)
				{
					update_uniform(command_buffer, *node_it->second.first, thread_index);
				}
				else
				{
					update_uniform(reinterpret_cast<vkb::core::CommandBufferC &>(command_buffer),
					               reinterpret_cast<vkb::scene_graph::NodeC &>(*node_it->second.first), thread_index);
				}
				draw_submesh_impl(command_buffer, *node_it->second.second);
			}
		}
	}
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::prepare()
{
	// Build all shader variance upfront
	auto &resource_cache = this->get_render_context_impl().get_device().get_resource_cache();
	for (auto &mesh : meshes)
	{
		for (auto &sub_mesh : mesh->get_submeshes())
		{
			auto &variant     = sub_mesh->get_shader_variant();
			auto &vert_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, this->get_vertex_shader_impl(), variant);
			auto &frag_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, this->get_fragment_shader_impl(), variant);
		}
	}
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::set_thread_index(uint32_t index)
{
	thread_index = index;
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::draw_submesh(vkb::core::CommandBuffer<bindingType> &command_buffer, SubMeshType &sub_mesh, FrontFaceType front_face)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		draw_submesh_impl(command_buffer, sub_mesh, front_face);
	}
	else
	{
		draw_submesh_impl(reinterpret_cast<vkb::core::CommandBufferCpp &>(command_buffer),
		                  reinterpret_cast<vkb::scene_graph::components::HPPSubMesh &>(sub_mesh), static_cast<vk::FrontFace>(front_face));
	}
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::draw_submesh_command(vkb::core::CommandBuffer<bindingType> &command_buffer, SubMeshType &sub_mesh)
{
	// Draw submesh indexed if indices exists
	if (sub_mesh.get_vertex_indices() != 0)
	{
		// Bind index buffer of submesh
		command_buffer.bind_index_buffer(sub_mesh.get_index_buffer(), sub_mesh.get_index_offset(), sub_mesh.get_index_type());

		// Draw submesh using indexed data
		command_buffer.draw_indexed(sub_mesh.get_vertex_indices(), 1, 0, 0, 0);
	}
	else
	{
		// Draw submesh using vertices only
		command_buffer.draw(sub_mesh.get_vertices_count(), 1, 0, 0);
	}
}

template <vkb::BindingType bindingType>
inline vkb::sg::Camera const &GeometrySubpass<bindingType>::get_camera() const
{
	return camera;
}

template <vkb::BindingType bindingType>
inline std::vector<typename GeometrySubpass<bindingType>::MeshType *> const &GeometrySubpass<bindingType>::get_meshes() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return get_meshes_impl();
	}
	else
	{
		return reinterpret_cast<std::vector<vkb::sg::Mesh *> const &>(get_meshes_impl());
	}
}

template <vkb::BindingType bindingType>
inline typename GeometrySubpass<bindingType>::RasterizationStateType const &GeometrySubpass<bindingType>::get_rasterization_state() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return base_rasterization_state;
	}
	else
	{
		return reinterpret_cast<vkb::RasterizationState const &>(base_rasterization_state);
	}
}

template <vkb::BindingType bindingType>
inline typename GeometrySubpass<bindingType>::SceneType const &GeometrySubpass<bindingType>::get_scene() const
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return *scene;
	}
	else
	{
		return *reinterpret_cast<vkb::sg::Scene const *>(scene);
	}
}

template <vkb::BindingType bindingType>
inline void
    GeometrySubpass<bindingType>::get_sorted_nodes(std::multimap<float, std::pair<vkb::scene_graph::Node<bindingType> *, SubMeshType *>> &opaque_nodes,
                                                   std::multimap<float, std::pair<vkb::scene_graph::Node<bindingType> *, SubMeshType *>> &transparent_nodes)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		get_sorted_nodes_impl(opaque_nodes, transparent_nodes);
	}
	else
	{
		get_sorted_nodes_impl(
		    reinterpret_cast<std::multimap<float, std::pair<vkb::scene_graph::NodeCpp *, vkb::scene_graph::components::HPPSubMesh *>> &>(opaque_nodes),
		    reinterpret_cast<std::multimap<float, std::pair<vkb::scene_graph::NodeCpp *, vkb::scene_graph::components::HPPSubMesh *>> &>(transparent_nodes));
	}
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::get_sorted_nodes_impl(
    std::multimap<float, std::pair<vkb::scene_graph::NodeCpp *, vkb::scene_graph::components::HPPSubMesh *>> &opaque_nodes,
    std::multimap<float, std::pair<vkb::scene_graph::NodeCpp *, vkb::scene_graph::components::HPPSubMesh *>> &transparent_nodes)
{
	auto camera_transform = camera.get_node()->get_transform().get_world_matrix();

	for (auto &mesh : meshes)
	{
		for (auto &node : mesh->get_nodes())
		{
			auto node_transform = node->get_transform().get_world_matrix();

			const sg::AABB &mesh_bounds = mesh->get_bounds();

			sg::AABB world_bounds{mesh_bounds.get_min(), mesh_bounds.get_max()};
			world_bounds.transform(node_transform);

			float distance = glm::length(glm::vec3(camera_transform[3]) - world_bounds.get_center());

			for (auto &sub_mesh : mesh->get_submeshes())
			{
				if (sub_mesh->get_material()->get_alpha_mode() == sg::AlphaMode::Blend)
				{
					transparent_nodes.emplace(distance, std::make_pair(node, sub_mesh));
				}
				else
				{
					opaque_nodes.emplace(distance, std::make_pair(node, sub_mesh));
				}
			}
		}
	}
}

template <vkb::BindingType bindingType>
inline uint32_t GeometrySubpass<bindingType>::get_thread_index() const
{
	return thread_index;
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::set_rasterization_state(const RasterizationStateType &rasterization_state)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		base_rasterization_state = rasterization_state;
	}
	else
	{
		base_rasterization_state = *reinterpret_cast<const vkb::rendering::HPPRasterizationState *>(&rasterization_state);
	}
}

template <vkb::BindingType bindingType>
inline typename GeometrySubpass<bindingType>::PipelineLayoutType &
    GeometrySubpass<bindingType>::prepare_pipeline_layout(vkb::core::CommandBuffer<bindingType> &command_buffer,
                                                          const std::vector<ShaderModuleType *> &shader_modules)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		return prepare_pipeline_layout_impl(command_buffer, shader_modules);
	}
	else
	{
		return reinterpret_cast<vkb::PipelineLayout &>(
		    prepare_pipeline_layout_impl(reinterpret_cast<vkb::core::CommandBufferCpp &>(command_buffer),
		                                 reinterpret_cast<const std::vector<vkb::core::HPPShaderModule *> &>(shader_modules)));
	}
}

template <vkb::BindingType bindingType>
inline vkb::core::HPPPipelineLayout &GeometrySubpass<bindingType>::prepare_pipeline_layout_impl(vkb::core::CommandBufferCpp                     &command_buffer,
                                                                                                const std::vector<vkb::core::HPPShaderModule *> &shader_modules)
{
	// Sets any specified resource modes
	for (auto &shader_module : shader_modules)
	{
		for (auto &resource_mode : this->get_resource_mode_map())
		{
			shader_module->set_resource_mode(resource_mode.first, resource_mode.second);
		}
	}

	return command_buffer.get_device().get_resource_cache().request_pipeline_layout(shader_modules);
}

template <vkb::BindingType bindingType>
inline std::vector<vkb::scene_graph::components::HPPMesh *> const &GeometrySubpass<bindingType>::get_meshes_impl() const
{
	return meshes;
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::draw_submesh_impl(vkb::core::CommandBufferCpp &command_buffer, vkb::scene_graph::components::HPPSubMesh &sub_mesh,
                                                            vk::FrontFace front_face)
{
	vkb::core::HPPScopedDebugLabel submesh_debug_label{command_buffer, sub_mesh.get_name().c_str()};

	if constexpr (bindingType == BindingType::Cpp)
	{
		prepare_pipeline_state(command_buffer, front_face, sub_mesh.get_material()->is_double_sided());
	}
	else
	{
		prepare_pipeline_state(reinterpret_cast<vkb::core::CommandBufferC &>(command_buffer), static_cast<VkFrontFace>(front_face),
		                       sub_mesh.get_material()->is_double_sided());
	}

	vkb::rendering::HPPMultisampleState multisample_state{.rasterization_samples = this->get_sample_count_impl()};
	command_buffer.set_multisample_state(multisample_state);

	auto &resource_cache = command_buffer.get_device().get_resource_cache();
	auto &vert_shader_module =
	    resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, this->get_vertex_shader_impl(), sub_mesh.get_shader_variant());
	auto &frag_shader_module =
	    resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, this->get_fragment_shader_impl(), sub_mesh.get_shader_variant());

	auto &pipeline_layout = reinterpret_cast<vkb::core::HPPPipelineLayout &>(
	    prepare_pipeline_layout(reinterpret_cast<vkb::core::CommandBuffer<bindingType> &>(command_buffer),
	                            {reinterpret_cast<ShaderModuleType *>(&vert_shader_module), reinterpret_cast<ShaderModuleType *>(&frag_shader_module)}));

	command_buffer.bind_pipeline_layout(pipeline_layout);

	if (pipeline_layout.get_push_constant_range_stage(sizeof(PBRMaterialUniform)))
	{
		if constexpr (bindingType == BindingType::Cpp)
		{
			prepare_push_constants(command_buffer, sub_mesh);
		}
		else
		{
			prepare_push_constants(reinterpret_cast<vkb::core::CommandBufferC &>(command_buffer), reinterpret_cast<SubMeshType &>(sub_mesh));
		}
	}

	vkb::core::HPPDescriptorSetLayout const &descriptor_set_layout = pipeline_layout.get_descriptor_set_layout(0);

	for (auto const &texture : sub_mesh.get_material()->get_textures())
	{
		if (auto layout_binding = descriptor_set_layout.get_layout_binding(texture.first))
		{
			command_buffer.bind_image(texture.second->get_image()->get_vk_image_view(), texture.second->get_sampler()->get_core_sampler(), 0,
			                          layout_binding->binding, 0);
		}
	}

	auto vertex_input_resources = pipeline_layout.get_resources(vkb::core::HPPShaderResourceType::Input, vk::ShaderStageFlagBits::eVertex);

	HPPVertexInputState vertex_input_state;

	for (auto &input_resource : vertex_input_resources)
	{
		vkb::scene_graph::components::HPPVertexAttribute attribute;
		if (!sub_mesh.get_attribute(input_resource.name, attribute))
		{
			continue;
		}

		vertex_input_state.attributes.push_back(
		    {.location = input_resource.location, .binding = input_resource.location, .format = attribute.format, .offset = attribute.offset});
		vertex_input_state.bindings.push_back({.binding = input_resource.location, .stride = attribute.stride});
	}

	command_buffer.set_vertex_input_state(vertex_input_state);

	// Find submesh vertex buffers matching the shader input attribute names
	for (auto &input_resource : vertex_input_resources)
	{
		auto const *buffer_ptr = sub_mesh.find_vertex_buffer(input_resource.name);

		if (buffer_ptr)
		{
			std::vector<std::reference_wrapper<const vkb::core::BufferCpp>> buffers;
			buffers.emplace_back(std::ref(*buffer_ptr));

			// Bind vertex buffers only for the attribute locations defined
			command_buffer.bind_vertex_buffers(input_resource.location, std::move(buffers), {0});
		}
	}

	if constexpr (bindingType == BindingType::Cpp)
	{
		draw_submesh_command(command_buffer, sub_mesh);
	}
	else
	{
		draw_submesh_command(reinterpret_cast<vkb::core::CommandBufferC &>(command_buffer), reinterpret_cast<SubMeshType &>(sub_mesh));
	}
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::prepare_pipeline_state(vkb::core::CommandBuffer<bindingType> &command_buffer, FrontFaceType front_face,
                                                                 bool double_sided_material)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		prepare_pipeline_state_impl(command_buffer, front_face, double_sided_material);
	}
	else
	{
		prepare_pipeline_state_impl(reinterpret_cast<vkb::core::CommandBufferCpp &>(command_buffer), static_cast<vk::FrontFace>(front_face),
		                            double_sided_material);
	}
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::prepare_pipeline_state_impl(vkb::core::CommandBufferCpp &command_buffer, vk::FrontFace front_face,
                                                                      bool double_sided_material)
{
	vkb::rendering::HPPRasterizationState rasterization_state = this->base_rasterization_state;
	rasterization_state.front_face                            = front_face;

	if (double_sided_material)
	{
		rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
	}

	command_buffer.set_rasterization_state(rasterization_state);

	vkb::rendering::HPPMultisampleState multisample_state{};
	multisample_state.rasterization_samples = this->get_sample_count_impl();
	command_buffer.set_multisample_state(multisample_state);
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::prepare_push_constants(vkb::core::CommandBuffer<bindingType> &command_buffer, SubMeshType &sub_mesh)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		prepare_push_constants_impl(command_buffer, sub_mesh);
	}
	else
	{
		prepare_push_constants_impl(reinterpret_cast<vkb::core::CommandBufferCpp &>(command_buffer),
		                            reinterpret_cast<vkb::scene_graph::components::HPPSubMesh &>(sub_mesh));
	}
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::prepare_push_constants_impl(vkb::core::CommandBufferCpp              &command_buffer,
                                                                      vkb::scene_graph::components::HPPSubMesh &sub_mesh)
{
	auto pbr_material = reinterpret_cast<const sg::PBRMaterial *>(sub_mesh.get_material());

	PBRMaterialUniform pbr_material_uniform{};
	pbr_material_uniform.base_color_factor = pbr_material->base_color_factor;
	pbr_material_uniform.metallic_factor   = pbr_material->metallic_factor;
	pbr_material_uniform.roughness_factor  = pbr_material->roughness_factor;

	auto data = to_bytes(pbr_material_uniform);

	if (!data.empty())
	{
		command_buffer.push_constants(data);
	}
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::update_uniform(vkb::core::CommandBuffer<bindingType> &command_buffer, vkb::scene_graph::Node<bindingType> &node,
                                                         size_t thread_index)
{
	if constexpr (bindingType == BindingType::Cpp)
	{
		update_uniform_impl(command_buffer, node, thread_index);
	}
	else
	{
		update_uniform_impl(reinterpret_cast<vkb::core::CommandBufferCpp &>(command_buffer), reinterpret_cast<vkb::scene_graph::NodeCpp &>(node), thread_index);
	}
}

template <vkb::BindingType bindingType>
inline void GeometrySubpass<bindingType>::update_uniform_impl(vkb::core::CommandBufferCpp &command_buffer, vkb::scene_graph::NodeCpp &node, size_t thread_index)
{
	GlobalUniform global_uniform;

	global_uniform.camera_view_proj = camera.get_pre_rotation() * vkb::rendering::vulkan_style_projection(camera.get_projection()) * camera.get_view();

	auto &render_frame = this->get_render_context_impl().get_active_frame();

	auto &transform = node.get_transform();

	auto allocation = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(GlobalUniform), thread_index);

	global_uniform.model = transform.get_world_matrix();

	global_uniform.camera_position = glm::vec3(glm::inverse(camera.get_view())[3]);

	allocation.update(global_uniform);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 1, 0);
}

}        // namespace subpasses
}        // namespace rendering
}        // namespace vkb
