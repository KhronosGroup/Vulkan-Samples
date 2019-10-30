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

#include "gltf_loader_test.h"

#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "rendering/subpasses/scene_subpass.h"
#include "stats.h"
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#	include "platform/android/android_platform.h"
#endif

namespace vkbtest
{
GLTFLoaderTest::GLTFLoaderTest(const std::string &scene_path) :
    scene_path{scene_path}
{
}

bool GLTFLoaderTest::prepare(vkb::Platform &platform)
{
	if (!VulkanTest::prepare(platform))
	{
		return false;
	}

	load_scene(scene_path);

	auto camera_node = scene->find_node("main_camera");

	if (!camera_node)
	{
		LOGW("Camera node not found. Looking for `default_camera` node.");

		camera_node = scene->find_node("default_camera");
	}

	auto &camera = camera_node->get_component<vkb::sg::Camera>();

	vkb::ShaderSource vert_shader(vkb::fs::read_shader("base.vert"));
	vkb::ShaderSource frag_shader(vkb::fs::read_shader("base.frag"));

	auto scene_subpass = std::make_unique<vkb::SceneSubpass>(*render_context, std::move(vert_shader), std::move(frag_shader), *scene, camera);

	auto render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_subpass));

	VulkanSample::set_render_pipeline(std::move(render_pipeline));

	return true;
}

}        // namespace vkbtest
