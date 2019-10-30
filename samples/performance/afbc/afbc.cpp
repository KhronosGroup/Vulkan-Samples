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

#include "afbc.h"

#include "common/vk_common.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "rendering/subpasses/scene_subpass.h"
#include "stats.h"

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#	include "platform/android/android_platform.h"
#endif

AFBCSample::AFBCSample()
{
	auto &config = get_configuration();

	config.insert<vkb::BoolSetting>(0, afbc_enabled, false);
	config.insert<vkb::BoolSetting>(1, afbc_enabled, true);
}

bool AFBCSample::prepare(vkb::Platform &platform)
{
	if (!VulkanSample::prepare(platform))
	{
		return false;
	}

	// We want AFBC disabled by default, hence we create swapchain with 'VK_IMAGE_USAGE_STORAGE_BIT'
	std::set<VkImageUsageFlagBits> image_usage = {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_STORAGE_BIT};
	get_render_context().update_swapchain(image_usage);

	load_scene("scenes/sponza/Sponza01.gltf");
	auto &camera_node = add_free_camera("main_camera");
	camera            = &camera_node.get_component<vkb::sg::Camera>();

	vkb::ShaderSource vert_shader("base.vert");
	vkb::ShaderSource frag_shader("base.frag");
	auto              scene_subpass = std::make_unique<vkb::SceneSubpass>(*render_context, std::move(vert_shader), std::move(frag_shader), *scene, *camera);

	auto render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_subpass));

	set_render_pipeline(std::move(render_pipeline));

	stats = std::make_unique<vkb::Stats>(std::set<vkb::StatIndex>{vkb::StatIndex::l2_ext_write_bytes});
	gui   = std::make_unique<vkb::Gui>(*render_context, platform.get_dpi_factor());

	return true;
}

void AFBCSample::update(float delta_time)
{
	if (afbc_enabled != afbc_enabled_last_value)
	{
		std::set<VkImageUsageFlagBits> image_usage_flags = {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};

		if (!afbc_enabled)
		{
			image_usage_flags.insert(VK_IMAGE_USAGE_STORAGE_BIT);
		}

		get_device().wait_idle();

		get_render_context().update_swapchain(image_usage_flags);

		afbc_enabled_last_value = afbc_enabled;
	}

	VulkanSample::update(delta_time);
}

void AFBCSample::draw_gui()
{
	gui->show_options_window(
	    /* body = */ [this]() {
		    ImGui::Checkbox("AFBC", &afbc_enabled);
	    },
	    /* lines = */ 1);
}

std::unique_ptr<vkb::VulkanSample> create_afbc()
{
	return std::make_unique<AFBCSample>();
}
