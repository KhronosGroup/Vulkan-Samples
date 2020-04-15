/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "rendering/subpasses/geometry_subpass.h"
#include "common/utils.h"
#include "common/vk_common.h"
#include "rendering/render_context.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/node.h"
#include "scene_graph/scene.h"

namespace vkb
{
GeometrySubpass::GeometrySubpass(RenderContext &render_context, ShaderSource &&vertex_source, ShaderSource &&fragment_source, sg::Scene &scene_, sg::Camera &camera) :
    Subpass{render_context, std::move(vertex_source), std::move(fragment_source)},
    meshes{scene_.get_components<sg::Mesh>()},
    camera{camera},
    scene{scene_}
{
}

void GeometrySubpass::prepare()
{
	// Build all shader variance upfront
	auto &device = render_context.get_device();
	for (auto &mesh : meshes)
	{
		for (auto &sub_mesh : mesh->get_submeshes())
		{
			auto &variant     = sub_mesh->get_shader_variant();
			auto &vert_module = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader(), variant);
			auto &frag_module = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader(), variant);
		}
	}
}

void GeometrySubpass::get_sorted_nodes(std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> &opaque_nodes, std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> &transparent_nodes)
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
				if (sub_mesh->get_material()->alpha_mode == sg::AlphaMode::Blend)
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

void GeometrySubpass::draw(CommandBuffer &command_buffer)
{
	std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> opaque_nodes;
	std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> transparent_nodes;

	get_sorted_nodes(opaque_nodes, transparent_nodes);

	// Draw opaque objects in front-to-back order
	for (auto node_it = opaque_nodes.begin(); node_it != opaque_nodes.end(); node_it++)
	{
		update_uniform(command_buffer, *node_it->second.first);

		// Invert the front face if the mesh was flipped
		const auto &scale      = node_it->second.first->get_transform().get_scale();
		bool        flipped    = scale.x * scale.y * scale.z < 0;
		VkFrontFace front_face = flipped ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;

		draw_submesh(command_buffer, *node_it->second.second, front_face);
	}

	// Enable alpha blending
	ColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.blend_enable           = VK_TRUE;
	color_blend_attachment.src_color_blend_factor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dst_color_blend_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.src_alpha_blend_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

	ColorBlendState color_blend_state{};
	color_blend_state.attachments.resize(get_output_attachments().size());
	color_blend_state.attachments[0] = color_blend_attachment;
	command_buffer.set_color_blend_state(color_blend_state);

	command_buffer.set_depth_stencil_state(get_depth_stencil_state());

	// Draw transparent objects in back-to-front order
	for (auto node_it = transparent_nodes.rbegin(); node_it != transparent_nodes.rend(); node_it++)
	{
		update_uniform(command_buffer, *node_it->second.first);

		draw_submesh(command_buffer, *node_it->second.second);
	}
}

void GeometrySubpass::update_uniform(CommandBuffer &command_buffer, sg::Node &node, size_t thread_index)
{
	GlobalUniform global_uniform;

	global_uniform.camera_view_proj = camera.get_pre_rotation() * vkb::vulkan_style_projection(camera.get_projection()) * camera.get_view();

	auto &render_frame = get_render_context().get_active_frame();

	auto &transform = node.get_transform();

	auto allocation = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(GlobalUniform), thread_index);

	global_uniform.model = transform.get_world_matrix();

	global_uniform.camera_position = glm::vec3(glm::inverse(camera.get_view())[3]);

	allocation.update(global_uniform);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 1, 0);
}

void GeometrySubpass::draw_submesh(CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, VkFrontFace front_face)
{
	auto &device = command_buffer.get_device();

	RasterizationState rasterization_state{};
	rasterization_state.front_face = front_face;

	if (sub_mesh.get_material()->double_sided)
	{
		rasterization_state.cull_mode = VK_CULL_MODE_NONE;
	}

	command_buffer.set_rasterization_state(rasterization_state);

	MultisampleState multisample_state{};
	multisample_state.rasterization_samples = sample_count;
	command_buffer.set_multisample_state(multisample_state);

	auto &vert_shader_module = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader(), sub_mesh.get_shader_variant());
	auto &frag_shader_module = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader(), sub_mesh.get_shader_variant());

	std::vector<ShaderModule *> shader_modules{&vert_shader_module, &frag_shader_module};

	auto &pipeline_layout = device.get_resource_cache().request_pipeline_layout(shader_modules);

	command_buffer.bind_pipeline_layout(pipeline_layout);

	auto pbr_material = dynamic_cast<const sg::PBRMaterial *>(sub_mesh.get_material());

	PBRMaterialUniform pbr_material_uniform{};
	pbr_material_uniform.base_color_factor = pbr_material->base_color_factor;
	pbr_material_uniform.metallic_factor   = pbr_material->metallic_factor;
	pbr_material_uniform.roughness_factor  = pbr_material->roughness_factor;

	command_buffer.push_constants_accumulated(pbr_material_uniform);

	auto &descriptor_set_layout = pipeline_layout.get_descriptor_set_layout(0);

	for (auto &texture : sub_mesh.get_material()->textures)
	{
		if (auto layout_binding = descriptor_set_layout.get_layout_binding(texture.first))
		{
			command_buffer.bind_image(texture.second->get_image()->get_vk_image_view(),
			                          texture.second->get_sampler()->vk_sampler,
			                          0, layout_binding->binding, 0);
		}
	}

	auto vertex_input_resources = pipeline_layout.get_resources(ShaderResourceType::Input, VK_SHADER_STAGE_VERTEX_BIT);

	VertexInputState vertex_input_state;

	for (auto &input_resource : vertex_input_resources)
	{
		sg::VertexAttribute attribute;

		if (!sub_mesh.get_attribute(input_resource.name, attribute))
		{
			continue;
		}

		VkVertexInputAttributeDescription vertex_attribute{};
		vertex_attribute.binding  = input_resource.location;
		vertex_attribute.format   = attribute.format;
		vertex_attribute.location = input_resource.location;
		vertex_attribute.offset   = attribute.offset;

		vertex_input_state.attributes.push_back(vertex_attribute);

		VkVertexInputBindingDescription vertex_binding{};
		vertex_binding.binding = input_resource.location;
		vertex_binding.stride  = attribute.stride;

		vertex_input_state.bindings.push_back(vertex_binding);
	}

	command_buffer.set_vertex_input_state(vertex_input_state);

	// Find submesh vertex buffers matching the shader input attribute names
	for (auto &input_resource : vertex_input_resources)
	{
		const auto &buffer_iter = sub_mesh.vertex_buffers.find(input_resource.name);

		if (buffer_iter != sub_mesh.vertex_buffers.end())
		{
			std::vector<std::reference_wrapper<const core::Buffer>> buffers;
			buffers.emplace_back(std::ref(buffer_iter->second));

			// Bind vertex buffers only for the attribute locations defined
			command_buffer.bind_vertex_buffers(input_resource.location, std::move(buffers), {0});
		}
	}

	draw_submesh_command(command_buffer, sub_mesh);
}

void GeometrySubpass::draw_submesh_command(CommandBuffer &command_buffer, sg::SubMesh &sub_mesh)
{
	// Draw submesh indexed if indices exists
	if (sub_mesh.vertex_indices != 0)
	{
		// Bind index buffer of submesh
		command_buffer.bind_index_buffer(*sub_mesh.index_buffer, sub_mesh.index_offset, sub_mesh.index_type);

		// Draw submesh using indexed data
		command_buffer.draw_indexed(sub_mesh.vertex_indices, 1, 0, 0, 0);
	}
	else
	{
		// Draw submesh using vertices only
		command_buffer.draw(sub_mesh.vertices_count, 1, 0, 0);
	}
}
}        // namespace vkb
