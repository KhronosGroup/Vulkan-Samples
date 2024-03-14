/* Copyright (c) 2018-2020, Arm Limited and Contributors
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

#include "rendering/subpasses/forward_subpass.h"

#include "common/utils.h"
#include "common/vk_common.h"
#include "rendering/render_context.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/sub_mesh.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/node.h"
#include "scene_graph/scene.h"

namespace vkb
{
ForwardSubpass::ForwardSubpass(RenderContext &render_context, ShaderSource &&vertex_source, ShaderSource &&fragment_source, sg::Scene &scene_, sg::Camera &camera) :
    GeometrySubpass{render_context, std::move(vertex_source), std::move(fragment_source), scene_, camera}
{
}

void ForwardSubpass::prepare()
{
	auto &device = render_context.get_device();
	for (auto &mesh : meshes)
	{
		for (auto &sub_mesh : mesh->get_submeshes())
		{
			auto &variant = sub_mesh->get_mut_shader_variant();

			// Same as Geometry except adds lighting definitions to sub mesh variants.
			variant.add_definitions({"MAX_LIGHT_COUNT " + std::to_string(MAX_FORWARD_LIGHT_COUNT)});

			variant.add_definitions(light_type_definitions);

			auto &vert_module = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader(), variant);
			auto &frag_module = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader(), variant);
		}
	}
}

void ForwardSubpass::draw(vkb::core::CommandBuffer<vkb::BindingType::C> &command_buffer)
{
	allocate_lights<ForwardLights>(scene.get_components<sg::Light>(), MAX_FORWARD_LIGHT_COUNT);
	command_buffer.bind_lighting(get_lighting_state(), 0, 4);

	GeometrySubpass::draw(command_buffer);
}
}        // namespace vkb
