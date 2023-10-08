/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include "oit_linked_lists.h"

OITLinkedLists::OITLinkedLists()
{
}

OITLinkedLists::~OITLinkedLists()
{
	if (!device)
	{
		return;
	}

	object_desc_.reset();
	scene_constants_.reset();
}

bool OITLinkedLists::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}
	
	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.0f, 0.0f, -4.0f});
	camera.set_rotation({0.0f, 180.0f, 0.0f});
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	prepare_buffers();
	update_scene_constants();

	return true;
}

void OITLinkedLists::prepare_buffers()
{
	scene_constants_ = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(SceneConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	object_desc_ = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ObjectDesc) * kObjectCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void OITLinkedLists::request_gpu_features(vkb::PhysicalDevice &gpu)
{
}

void OITLinkedLists::build_command_buffers()
{
}

void OITLinkedLists::update_scene_constants()
{
	SceneConstants constants;
	constants.projection       = camera.matrices.perspective;
	constants.view             = camera.matrices.view * glm::mat4(1.f);
	scene_constants_->convert_and_update(constants);
}

void OITLinkedLists::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void OITLinkedLists::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (camera.updated)
	{
		update_scene_constants();
	}
}

std::unique_ptr<vkb::VulkanSample> create_oit_linked_lists()
{
	return std::make_unique<OITLinkedLists>();
}
