/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "surface_rotation.h"

#include "core/device.h"
#include "core/pipeline_layout.h"
#include "core/shader_module.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "rendering/subpasses/scene_subpass.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/pbr_material.h"
#include "stats.h"

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#	include "platform/android/android_platform.h"
#endif

SurfaceRotation::SurfaceRotation()
{
	auto &config = get_configuration();

	config.insert<vkb::BoolSetting>(0, pre_rotate, false);
	config.insert<vkb::BoolSetting>(1, pre_rotate, true);
}

bool SurfaceRotation::prepare(vkb::Platform &platform)
{
	if (!VulkanSample::prepare(platform))
	{
		return false;
	}

	auto enabled_stats = {vkb::StatIndex::l2_ext_read_stalls, vkb::StatIndex::l2_ext_write_stalls};

	stats = std::make_unique<vkb::Stats>(enabled_stats);

	load_scene("scenes/sponza/Sponza01.gltf");
	auto &camera_node = add_free_camera("main_camera");
	camera            = dynamic_cast<vkb::sg::PerspectiveCamera *>(&camera_node.get_component<vkb::sg::Camera>());

	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_subpass = std::make_unique<vkb::SceneSubpass>(*render_context, std::move(vert_shader), std::move(frag_shader), *scene, *camera);

	auto render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_subpass));

	set_render_pipeline(std::move(render_pipeline));

	gui = std::make_unique<vkb::Gui>(*render_context, platform.get_dpi_factor());

	return true;
}

void SurfaceRotation::update(float delta_time)
{
	// Process GUI input
	if (pre_rotate != last_pre_rotate)
	{
		trigger_swapchain_recreation();
		last_pre_rotate = pre_rotate;
	}

	glm::mat4 pre_rotate_mat = glm::mat4(1.0f);

	// In pre-rotate mode, the application has to handle the rotation
	glm::vec3 rotation_axis = glm::vec3(0.0f, 0.0f, -1.0f);

	const auto &swapchain = render_context->get_swapchain();

	if (swapchain.get_transform() & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR)
	{
		pre_rotate_mat = glm::rotate(pre_rotate_mat, glm::radians(90.0f), rotation_axis);
	}
	else if (swapchain.get_transform() & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR)
	{
		pre_rotate_mat = glm::rotate(pre_rotate_mat, glm::radians(270.0f), rotation_axis);
	}
	else if (swapchain.get_transform() & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR)
	{
		pre_rotate_mat = glm::rotate(pre_rotate_mat, glm::radians(180.0f), rotation_axis);
	}

	// Ensure that the camera uses the swapchain dimensions, since in pre-rotate
	// mode the aspect ratio never changes
	VkExtent2D extent = swapchain.get_extent();
	camera->set_aspect_ratio(static_cast<float>(extent.width) / extent.height);
	camera->set_pre_rotation(pre_rotate_mat);

	VulkanSample::update(delta_time);
}

void SurfaceRotation::draw_gui()
{
	std::string       rotation_by_str = pre_rotate ? "application" : "compositor";
	auto              prerotate_str   = "Pre-rotate (" + rotation_by_str + " rotates)";
	uint32_t          a_width         = render_context->get_swapchain().get_extent().width;
	uint32_t          a_height        = render_context->get_swapchain().get_extent().height;
	float             aspect_ratio    = static_cast<float>(a_width) / static_cast<float>(a_height);
	auto              transform       = vkb::to_string(render_context->get_swapchain().get_transform());
	auto              resolution_str  = "Res: " + std::to_string(a_width) + "x" + std::to_string(a_height);
	std::stringstream fov_stream;
	fov_stream << "FOV: " << std::fixed << std::setprecision(2) << camera->get_field_of_view() * 180.0f / glm::pi<float>();
	auto fov_str = fov_stream.str();

	// If pre-rotate is enabled, the aspect ratio will not change, therefore need to check if the
	// scene has been rotated
	auto rotated = render_context->get_swapchain().get_transform() & (VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR | VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR);
	if (aspect_ratio > 1.0f || (aspect_ratio < 1.0f && rotated))
	{
		// GUI landscape layout
		uint32_t lines = 2;
		gui->show_options_window(
		    /* body = */ [&]() {
			    ImGui::Checkbox(prerotate_str.c_str(), &pre_rotate);
			    ImGui::Text("%s | %s | %s", transform.c_str(), resolution_str.c_str(), fov_str.c_str());
		    },
		    /* lines = */ lines);
	}
	else
	{
		// GUI portrait layout
		uint32_t lines = 3;
		gui->show_options_window(
		    /* body = */ [&]() {
			    ImGui::Checkbox(prerotate_str.c_str(), &pre_rotate);
			    ImGui::Text("%s", transform.c_str());
			    ImGui::Text("%s | %s", resolution_str.c_str(), fov_str.c_str());
		    },
		    /* lines = */ lines);
	}
}

void SurfaceRotation::trigger_swapchain_recreation()
{
	recreate_swapchain();

	if (gui)
	{
		gui->resize(get_render_context().get_surface_extent().width,
		            get_render_context().get_surface_extent().height);
	}
}

std::unique_ptr<vkb::VulkanSample> create_surface_rotation()
{
	return std::make_unique<SurfaceRotation>();
}

void SurfaceRotation::recreate_swapchain()
{
	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(get_device().get_physical_device(),
	                                                   get_surface(),
	                                                   &surface_properties));

	auto width  = surface_properties.currentExtent.width;
	auto height = surface_properties.currentExtent.height;

	VkSurfaceTransformFlagBitsKHR pre_transform;

	if (pre_rotate)
	{
		// Best practice: adjust the preTransform attribute in the swapchain properties
		pre_transform = surface_properties.currentTransform;

		// Always use native orientation i.e. if rotated, use width and height of identity transform
		if (pre_transform == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR || pre_transform == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR)
		{
			std::swap(width, height);
		}
	}
	else
	{
		// Bad practice: keep preTransform as identity
		pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}

	get_device().wait_idle();

	get_render_context().update_swapchain(VkExtent2D{width, height}, pre_transform);
}

void SurfaceRotation::handle_surface_changes()
{
	auto surface_extent = get_render_context().get_surface_extent();

	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(get_device().get_physical_device(),
	                                                   get_surface(),
	                                                   &surface_properties));

	if (surface_properties.currentExtent.width != surface_extent.width ||
	    surface_properties.currentExtent.height != surface_extent.height ||
	    (pre_rotate && surface_properties.currentTransform != get_render_context().get_swapchain().get_transform()))
	{
		recreate_swapchain();

		surface_extent = surface_properties.currentExtent;
	}
}
