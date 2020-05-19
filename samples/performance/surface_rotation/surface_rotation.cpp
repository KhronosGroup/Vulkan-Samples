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

#include "surface_rotation.h"

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
#include <glm/gtc/matrix_transform.hpp>
VKBP_ENABLE_WARNINGS()

#include "core/device.h"
#include "core/pipeline_layout.h"
#include "core/shader_module.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/pbr_material.h"
#include "stats/stats.h"

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

	if (get_surface() == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Requires a surface to run sample");
	}

	stats->request_stats({vkb::StatIndex::gpu_ext_read_stalls,
	                      vkb::StatIndex::gpu_ext_write_stalls});

	load_scene("scenes/sponza/Sponza01.gltf");

	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = dynamic_cast<vkb::sg::PerspectiveCamera *>(&camera_node.get_component<vkb::sg::Camera>());

	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_subpass = std::make_unique<vkb::ForwardSubpass>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, *camera);

	auto render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_subpass));

	set_render_pipeline(std::move(render_pipeline));

	gui = std::make_unique<vkb::Gui>(*this, platform.get_window(), stats.get());

	return true;
}

void SurfaceRotation::update(float delta_time)
{
	// Process GUI input, recreating the swapchain if pre-rotate mode was
	// enabled/disabled by the user. Otherwise it might still be recreated if
	// a 180 degree change in orientation is detected
	if (pre_rotate != last_pre_rotate)
	{
		recreate_swapchain();

		last_pre_rotate = pre_rotate;
	}
	else
	{
		handle_no_resize_rotations();
	}

	// In pre-rotate mode, the application has to handle the rotation.
	// The swapchain preTransform attribute will now be something other than
	// identity only if pre-rotate mode is enabled
	glm::mat4   pre_rotate_mat = glm::mat4(1.0f);
	glm::vec3   rotation_axis  = glm::vec3(0.0f, 0.0f, 1.0f);
	const auto &swapchain      = get_render_context().get_swapchain();

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

	camera->set_pre_rotation(pre_rotate_mat);

	VulkanSample::update(delta_time);
}

void SurfaceRotation::draw_gui()
{
	auto        extent          = get_render_context().get_swapchain().get_extent();
	std::string rotation_by_str = pre_rotate ? "application" : "compositor";
	auto        prerotate_str   = "Pre-rotate (" + rotation_by_str + " rotates)";
	auto        transform       = vkb::to_string(get_render_context().get_swapchain().get_transform());
	auto        resolution_str  = "Res: " + std::to_string(extent.width) + "x" + std::to_string(extent.height);

	// If pre-rotate is enabled, the aspect ratio will not change, therefore need to check if the
	// scene has been rotated using the swapchain preTransform attribute
	auto  rotated      = get_render_context().get_swapchain().get_transform() & (VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR | VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR);
	float aspect_ratio = static_cast<float>(extent.width) / extent.height;
	if (aspect_ratio > 1.0f || (aspect_ratio < 1.0f && rotated))
	{
		// GUI landscape layout
		uint32_t lines = 2;
		gui->show_options_window(
		    /* body = */ [&]() {
			    ImGui::Checkbox(prerotate_str.c_str(), &pre_rotate);
			    ImGui::Text("%s | %s", transform.c_str(), resolution_str.c_str());
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
			    ImGui::Text("%s", resolution_str.c_str());
		    },
		    /* lines = */ lines);
	}
}

VkSurfaceTransformFlagBitsKHR SurfaceRotation::select_pre_transform()
{
	VkSurfaceTransformFlagBitsKHR pre_transform;

	if (pre_rotate)
	{
		VkSurfaceCapabilitiesKHR surface_properties;
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(get_device().get_gpu().get_handle(),
		                                                   get_surface(),
		                                                   &surface_properties));

		// Best practice: adjust the preTransform attribute in the swapchain properties
		// so that it matches the value in the surface properties. This is to
		// communicate to the presentation engine that the application is pre-rotating
		pre_transform = surface_properties.currentTransform;
	}
	else
	{
		// Bad practice: keep preTransform as identity
		pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}

	return pre_transform;
}

void SurfaceRotation::handle_no_resize_rotations()
{
	// The render context recreates the swapchain if it detects a change of dimensions or an
	// out-of-date swapchain. However 180 degree rotations do not currently trigger a resize.
	// If pre-rotate mode is enabled, the sample will detect a 180 degree change in orientation
	// and re-create the swapchain
	VkSurfaceCapabilitiesKHR surface_properties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(get_device().get_gpu().get_handle(),
	                                                   get_surface(),
	                                                   &surface_properties));

	if (surface_properties.currentExtent.width == get_render_context().get_surface_extent().width &&
	    surface_properties.currentExtent.height == get_render_context().get_surface_extent().height &&
	    (pre_rotate && surface_properties.currentTransform != get_render_context().get_swapchain().get_transform()))
	{
		recreate_swapchain();
	}
}

void SurfaceRotation::recreate_swapchain()
{
	get_device().wait_idle();

	auto surface_extent = get_render_context().get_surface_extent();

	get_render_context().update_swapchain(surface_extent, select_pre_transform());

	if (gui)
	{
		gui->resize(surface_extent.width, surface_extent.height);
	}
}

std::unique_ptr<vkb::VulkanSample> create_surface_rotation()
{
	return std::make_unique<SurfaceRotation>();
}
