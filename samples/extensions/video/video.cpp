/* Copyright (c) 2023, Holochip Inc.
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

#include "video.h"

#include "gltf_loader.h"
#include "rendering/subpasses/forward_subpass.h"

video::video():
    pipeline(VK_NULL_HANDLE), pipeline_layout(VK_NULL_HANDLE), descriptor_set(VK_NULL_HANDLE), descriptor_set_layout(VK_NULL_HANDLE)
{
	title       = "Compute shader N-body simulation using VK_KHR_synchronization2";
	camera.type = vkb::CameraType::LookAt;

	// Note: Using Reversed depth-buffer for increased precision, so Z-Near and Z-Far are flipped
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 512.0f, 0.1f);
	camera.set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));
	camera.set_translation(glm::vec3(0.0f, 0.0f, -14.0f));
	camera.translation_speed = 2.5f;

	// Vulkan Video required extensions
	add_device_extension(VK_EXT_YCBCR_2PLANE_444_FORMATS_EXTENSION_NAME);
	add_device_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME);
	add_device_extension(VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME);
}

video::~video() {
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
	}
}

void video::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void video::build_command_buffers()
{

}

bool video::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	// Load a scene from the assets folder
	load_scene("scenes/sponza/Sponza01.gltf");

	// Attach a move script to the camera component in the scene
	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	auto camera       = &camera_node.get_component<vkb::sg::Camera>();

	// Example Scene Render Pipeline
	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_sub_pass   = std::make_unique<vkb::ForwardSubpass>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, *camera);
	auto              render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_sub_pass));
	set_render_pipeline(std::move(render_pipeline));

	// Add a GUI with the stats you want to monitor
	stats->request_stats({/*stats you require*/});
	gui = std::make_unique<vkb::Gui>(*this, platform.get_window(), stats.get());

	return true;
}

void video::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
}

std::unique_ptr<vkb::VulkanSample> create_video()
{
	return std::make_unique<video>();
}
