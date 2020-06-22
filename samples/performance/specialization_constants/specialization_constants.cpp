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

#include "specialization_constants.h"

#include <string>
#include <vector>

#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "stats/stats.h"

SpecializationConstants::SpecializationConstants()
{
	auto &config = get_configuration();

	config.insert<vkb::IntSetting>(0, specialization_constants_enabled, 0);
	config.insert<vkb::IntSetting>(1, specialization_constants_enabled, 1);
}

SpecializationConstants::ForwardSubpassCustomLights::ForwardSubpassCustomLights(vkb::RenderContext &render_context,
                                                                                vkb::ShaderSource &&vertex_shader, vkb::ShaderSource &&fragment_shader, vkb::sg::Scene &scene_, vkb::sg::Camera &camera) :
    vkb::ForwardSubpass{render_context, std::move(vertex_shader), std::move(fragment_shader), scene_, camera}
{
}

bool SpecializationConstants::prepare(vkb::Platform &platform)
{
	if (!VulkanSample::prepare(platform))
	{
		return false;
	}

	load_scene("scenes/sponza/Sponza01.gltf");
	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = dynamic_cast<vkb::sg::PerspectiveCamera *>(&camera_node.get_component<vkb::sg::Camera>());

	// Create two pipelines, one with specialization constants the other with UBOs
	specialization_constants_pipeline = create_specialization_renderpass();
	standard_pipeline                 = create_standard_renderpass();

	stats->request_stats({vkb::StatIndex::gpu_fragment_cycles});

	gui = std::make_unique<vkb::Gui>(*this, platform.get_window(), stats.get());

	return true;
}

void SpecializationConstants::ForwardSubpassCustomLights::prepare()
{
	auto &device = render_context.get_device();
	for (auto &mesh : meshes)
	{
		for (auto &sub_mesh : mesh->get_submeshes())
		{
			auto &variant = sub_mesh->get_mut_shader_variant();

			// Same as Geometry except adds lighting definitions to sub mesh variants.
			variant.add_definitions({"MAX_FORWARD_LIGHT_COUNT " + std::to_string(LIGHT_COUNT)});
			variant.add_definitions(vkb::light_type_definitions);

			auto &vert_module = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader(), variant);
			auto &frag_module = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader(), variant);
		}
	}
}

void SpecializationConstants::render(vkb::CommandBuffer &command_buffer)
{
	// POI
	//
	// If specialization constants is enabled, pass the light count with specialization constants and use the specialization
	// constants render pipeline (using the "specialization_constants/specialization_constants.frag" fragment shader)
	//
	// Otherwise, pass the light count with uniform buffer objects constants and use the standard render pipeline
	// (using the "base.frag" fragment shader)

	if (specialization_constants_enabled)
	{
		command_buffer.set_specialization_constant(0, LIGHT_COUNT);
		specialization_constants_pipeline->draw(command_buffer, get_render_context().get_active_frame().get_render_target());
	}
	else
	{
		standard_pipeline->draw(command_buffer, get_render_context().get_active_frame().get_render_target());
	}
}

std::unique_ptr<vkb::RenderPipeline> SpecializationConstants::create_specialization_renderpass()
{
	// Scene subpass
	vkb::ShaderSource vert_shader(vkb::fs::read_shader("base.vert"));
	vkb::ShaderSource frag_shader(vkb::fs::read_shader("specialization_constants/specialization_constants.frag"));
	auto              scene_subpass = std::make_unique<ForwardSubpassCustomLights>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, *camera);

	// Create specialization constants pipeline
	std::vector<std::unique_ptr<vkb::Subpass>> scene_subpasses{};
	scene_subpasses.push_back(std::move(scene_subpass));

	auto specialization_constants_pipeline = std::make_unique<vkb::RenderPipeline>(std::move(scene_subpasses));

	return specialization_constants_pipeline;
}

std::unique_ptr<vkb::RenderPipeline> SpecializationConstants::create_standard_renderpass()
{
	// Scene subpass
	vkb::ShaderSource vert_shader(vkb::fs::read_shader("base.vert"));
	vkb::ShaderSource frag_shader(vkb::fs::read_shader("specialization_constants/UBOs.frag"));
	auto              scene_subpass = std::make_unique<ForwardSubpassCustomLights>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, *camera);

	// Create base pipeline
	std::vector<std::unique_ptr<vkb::Subpass>> scene_subpasses{};
	scene_subpasses.push_back(std::move(scene_subpass));

	auto standard_pipeline = std::make_unique<vkb::RenderPipeline>(std::move(scene_subpasses));

	return standard_pipeline;
}

void SpecializationConstants::ForwardSubpassCustomLights::draw(vkb::CommandBuffer &command_buffer)
{
	// Override forward light subpass draw function to provide a custom number of lights
	auto lights_buffer = allocate_set_num_lights<CustomForwardLights>(scene.get_components<vkb::sg::Light>(), static_cast<size_t>(LIGHT_COUNT));
	command_buffer.bind_buffer(lights_buffer.get_buffer(), lights_buffer.get_offset(), lights_buffer.get_size(), 0, 4, 0);

	vkb::GeometrySubpass::draw(command_buffer);
}

void SpecializationConstants::draw_gui()
{
	bool     landscape = camera->get_aspect_ratio() > 1.0f;
	uint32_t lines     = landscape ? 1 : 2;

	gui->show_options_window(
	    /* body = */ [&]() {
		    ImGui::RadioButton("Uniform Buffer Objects", &specialization_constants_enabled, 0);
		    if (landscape)
		    {
			    ImGui::SameLine();
		    }
		    ImGui::RadioButton("Specialization Constants", &specialization_constants_enabled, 1);
	    },
	    /* lines = */ lines);
}

std::unique_ptr<vkb::VulkanSample> create_specialization_constants()
{
	return std::make_unique<SpecializationConstants>();
}
