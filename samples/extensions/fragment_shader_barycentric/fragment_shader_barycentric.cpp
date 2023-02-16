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

#include "fragment_shader_barycentric.h"

#include "common/vk_common.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "rendering/subpasses/forward_subpass.h"
#include "stats/stats.h"

FragmentShaderBarycentric::FragmentShaderBarycentric()
{
	title = "Fragment shader barycentric";

	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);

}

bool FragmentShaderBarycentric::prepare(vkb::Platform &platform)
{
if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	/* Set up camera properties */
	camera.type = vkb::CameraType::LookAt;
	camera.set_position({2.0f, -4.0f, -10.0f});
	camera.set_rotation({-15.0f, 190.0f, 0.0f});
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	// Load a scene from the assets folder
	build_command_buffers();

	return true;
}

void FragmentShaderBarycentric::build_command_buffers()
{
	//TODO
}

void FragmentShaderBarycentric::render(float delta_time)
{
	//TODO
}

std::unique_ptr<vkb::VulkanSample> create_fragment_shader_barycentric()
{
	return std::make_unique<FragmentShaderBarycentric>();
}