/* Copyright (c) 2018-2019, Arm Limited and Contributors
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

#include "utils/graphs.h"

#include <iostream>
#include <vector>

#include "common/logging.h"
#include "scene_graph/node.h"
#include "utils/graph/graph.h"
#include "utils/graph/nodes/framework.h"
#include "utils/graph/nodes/scene.h"

namespace vkb
{
namespace utils
{
void get_node(Graph &graph, const std::vector<sg::Node *> &children, size_t owner)
{
	for (const auto &child : children)
	{
		size_t child_id = graph.create_node<SceneNode>(*child);
		graph.add_edge(owner, child_id);

		if (child->has_component<sg::Transform>())
		{
			auto & i            = child->get_component<sg::Transform>();
			size_t component_id = graph.create_node<SceneNode>(i);
			graph.add_edge(child_id, component_id);
		}
		if (child->has_component<sg::Mesh>())
		{
			auto & mesh    = child->get_component<sg::Mesh>();
			size_t mesh_id = graph.create_node<SceneNode>(mesh);
			graph.add_edge(child_id, mesh_id);

			for (const auto &sub_mesh : mesh.get_submeshes())
			{
				size_t sub_mesh_id = graph.create_node<SceneNode>(*sub_mesh);
				graph.add_edge(mesh_id, sub_mesh_id);

				const auto &material    = sub_mesh->get_material();
				size_t      material_id = graph.create_node<SceneNode>(*material);
				graph.add_edge(sub_mesh_id, material_id);

				auto it = material->textures.begin();
				while (it != material->textures.end())
				{
					size_t texture_id = graph.create_node<SceneNode>(*it->second, it->first);
					graph.add_edge(material_id, texture_id);
					it++;
				}
			}
		}

		get_node(graph, child->get_children(), child_id);
	}
}

bool debug_graphs(RenderContext &context, sg::Scene &scene)
{
	Graph  framework_graph("Framework");
	size_t device_id = framework_graph.create_node<FrameworkNode>(context.get_device());

	//Device
	auto &device = context.get_device();

	auto & resource_cache    = device.get_resource_cache();
	size_t resource_cache_id = framework_graph.create_node<FrameworkNode>(resource_cache);
	framework_graph.add_edge(device_id, resource_cache_id);

	const auto &resource_cache_state = resource_cache.get_internal_state();

	auto it_pipeline_layouts = resource_cache_state.pipeline_layouts.begin();
	while (it_pipeline_layouts != resource_cache_state.pipeline_layouts.end())
	{
		size_t pipeline_layouts_id = framework_graph.create_node<FrameworkNode>(it_pipeline_layouts->second, it_pipeline_layouts->first);
		framework_graph.add_edge(resource_cache_id, pipeline_layouts_id);

		auto &stages = it_pipeline_layouts->second.get_stages();
		for (const auto *shader_module : stages)
		{
			size_t shader_modules_id = framework_graph.create_node<FrameworkNode>(*shader_module);
			framework_graph.add_edge(pipeline_layouts_id, shader_modules_id);

			const auto &resources = shader_module->get_resources();
			for (const auto &resource : resources)
			{
				size_t resource_id = framework_graph.create_node<FrameworkNode>(resource);
				framework_graph.add_edge(shader_modules_id, resource_id);
			}
		}

		it_pipeline_layouts++;
	}

	auto it_descriptor_set_layouts = resource_cache_state.descriptor_set_layouts.begin();
	while (it_descriptor_set_layouts != resource_cache_state.descriptor_set_layouts.end())
	{
		size_t descriptor_set_layouts_id = framework_graph.create_node<FrameworkNode>(it_descriptor_set_layouts->second, it_descriptor_set_layouts->first);
		framework_graph.add_edge(resource_cache_id, descriptor_set_layouts_id);
		it_descriptor_set_layouts++;
	}
	auto it_render_passes = resource_cache_state.render_passes.begin();
	while (it_render_passes != resource_cache_state.render_passes.end())
	{
		size_t render_passes_id = framework_graph.create_node<FrameworkNode>(it_render_passes->second, it_render_passes->first);
		it_render_passes++;
	}
	auto it_graphics_pipelines = resource_cache_state.graphics_pipelines.begin();
	while (it_graphics_pipelines != resource_cache_state.graphics_pipelines.end())
	{
		size_t pipeline_layout = framework_graph.create_node<FrameworkNode>(it_graphics_pipelines->second.get_state().get_pipeline_layout());
		framework_graph.add_edge(resource_cache_id, pipeline_layout);

		size_t graphics_pipelines_id = framework_graph.create_node<FrameworkNode>(it_graphics_pipelines->second, it_graphics_pipelines->first);
		framework_graph.add_edge(pipeline_layout, graphics_pipelines_id);

		size_t graphics_pipelines_state_id = framework_graph.create_node<FrameworkNode>(it_graphics_pipelines->second.get_state());
		framework_graph.add_edge(graphics_pipelines_id, graphics_pipelines_state_id);

		size_t render_pass = framework_graph.create_node<FrameworkNode>(*it_graphics_pipelines->second.get_state().get_render_pass());
		framework_graph.add_edge(graphics_pipelines_state_id, render_pass);

		size_t specialization_constant_state = framework_graph.create_node<FrameworkNode>(it_graphics_pipelines->second.get_state().get_specialization_constant_state());
		framework_graph.add_edge(graphics_pipelines_state_id, specialization_constant_state);

		size_t vertex_input_state = framework_graph.create_node<FrameworkNode>(it_graphics_pipelines->second.get_state().get_vertex_input_state());
		framework_graph.add_edge(graphics_pipelines_state_id, vertex_input_state);

		size_t input_assembly_state = framework_graph.create_node<FrameworkNode>(it_graphics_pipelines->second.get_state().get_input_assembly_state());
		framework_graph.add_edge(graphics_pipelines_state_id, input_assembly_state);

		size_t rasterization_state = framework_graph.create_node<FrameworkNode>(it_graphics_pipelines->second.get_state().get_rasterization_state());
		framework_graph.add_edge(graphics_pipelines_state_id, rasterization_state);

		size_t viewport_state = framework_graph.create_node<FrameworkNode>(it_graphics_pipelines->second.get_state().get_viewport_state());
		framework_graph.add_edge(graphics_pipelines_state_id, viewport_state);

		size_t multisample_state = framework_graph.create_node<FrameworkNode>(it_graphics_pipelines->second.get_state().get_multisample_state());
		framework_graph.add_edge(graphics_pipelines_state_id, multisample_state);

		size_t depth_stencil_state = framework_graph.create_node<FrameworkNode>(it_graphics_pipelines->second.get_state().get_depth_stencil_state());
		framework_graph.add_edge(graphics_pipelines_state_id, depth_stencil_state);

		size_t color_blend_state = framework_graph.create_node<FrameworkNode>(it_graphics_pipelines->second.get_state().get_color_blend_state());
		framework_graph.add_edge(graphics_pipelines_state_id, color_blend_state);
		it_graphics_pipelines++;
	}
	auto it_compute_pipelines = resource_cache_state.compute_pipelines.begin();
	while (it_compute_pipelines != resource_cache_state.compute_pipelines.end())
	{
		size_t compute_pipelines_id = framework_graph.create_node<FrameworkNode>(it_compute_pipelines->second, it_compute_pipelines->first);
		framework_graph.add_edge(resource_cache_id, compute_pipelines_id);
		it_compute_pipelines++;
	}

	auto it_framebuffers = resource_cache_state.framebuffers.begin();
	while (it_framebuffers != resource_cache_state.framebuffers.end())
	{
		size_t framebuffers_id = framework_graph.create_node<FrameworkNode>(it_framebuffers->second, it_framebuffers->first);
		framework_graph.add_edge(resource_cache_id, framebuffers_id);
		it_framebuffers++;
	}

	size_t render_context_id = framework_graph.create_node<FrameworkNode>(context);
	framework_graph.add_edge(device_id, render_context_id);
	size_t swapchain_id = framework_graph.create_node<FrameworkNode>(context.get_swapchain());

	for (auto image : context.get_swapchain().get_images())
	{
		size_t vkimage_id = framework_graph.create_vk_image(image);
		framework_graph.add_edge(vkimage_id, swapchain_id);
	}

	size_t last_frame_id = framework_graph.create_node<FrameworkNode>(context.get_last_rendered_frame(), "Last Rendered Frame");
	framework_graph.add_edge(render_context_id, last_frame_id);

	// Render Frames - Fill out
	const auto &frames   = context.get_render_frames();
	auto        frame_it = frames.begin();
	while (frame_it != frames.end())
	{
		size_t frame_id = framework_graph.create_node<FrameworkNode>(*frame_it, "Render Frame");
		framework_graph.add_edge(render_context_id, frame_id);

		size_t semaphore_pool_id = framework_graph.create_node<FrameworkNode>(frame_it->get_semaphore_pool());
		size_t fence_pool_id     = framework_graph.create_node<FrameworkNode>(frame_it->get_fence_pool());
		size_t render_target_id  = framework_graph.create_node<FrameworkNode>(frame_it->get_render_target_const());
		framework_graph.add_edge(frame_id, semaphore_pool_id);
		framework_graph.add_edge(frame_id, fence_pool_id);
		framework_graph.add_edge(frame_id, render_target_id);

		for (const auto &view : frame_it->get_render_target_const().get_views())
		{
			size_t      image_view_id = framework_graph.create_node<FrameworkNode>(view);
			const auto &image         = view.get_image();
			size_t      image_id      = framework_graph.create_node<FrameworkNode>(image);

			framework_graph.add_edge(render_target_id, image_view_id);
			framework_graph.add_edge(image_view_id, image_id);

			size_t vkimage_id = framework_graph.create_vk_image(image.get_handle());
			framework_graph.add_edge(image_id, vkimage_id);

			size_t vkimageview_id = framework_graph.create_vk_image_view(view.get_handle());
			framework_graph.add_edge(image_view_id, vkimageview_id);
		}
		frame_it++;
	}

	Graph scene_graph("Scene");

	size_t scene_id = scene_graph.create_node<SceneNode>(scene);

	get_node(scene_graph, scene.get_root_node().get_children(), scene_id);

	return framework_graph.dump_to_file("framework.json") && scene_graph.dump_to_file("scene.json");
}
}        // namespace utils
}        // namespace vkb