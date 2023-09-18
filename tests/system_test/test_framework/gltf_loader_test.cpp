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

#include "gltf_loader_test.h"

#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "rendering/subpasses/forward_subpass.h"
#include "stats/stats.h"
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#	include "platform/android/android_platform.h"
#endif

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
#include <glm/gtx/quaternion.hpp>
VKBP_ENABLE_WARNINGS()

namespace vkbtest
{
GLTFLoaderTest::GLTFLoaderTest(const std::string &scene_path) :
    scene_path{scene_path}
{
}

bool GLTFLoaderTest::prepare(const vkb::ApplicationOptions &options)
{
	if (!VulkanTest::prepare(options))
	{
		return false;
	}

	load_scene(scene_path);

	scene->clear_components<vkb::sg::Light>();

	vkb::add_directional_light(get_scene(), glm::quat({glm::radians(-90.0f), 0.0f, glm::radians(30.0f)}));

	auto camera_node = scene->find_node("main_camera");

	if (!camera_node)
	{
		LOGW("Camera node not found. Looking for `default_camera` node.");

		camera_node = scene->find_node("default_camera");
	}

	auto &camera = camera_node->get_component<vkb::sg::Camera>();

	vkb::ShaderSource vert_shader("base.vert.glsl");
	vkb::ShaderSource frag_shader("base.frag.glsl");

	auto scene_subpass = std::make_unique<vkb::ForwardSubpass>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, camera);

	auto render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_subpass));

	VulkanSample::set_render_pipeline(std::move(render_pipeline));

	return true;
}

}        // namespace vkbtest
