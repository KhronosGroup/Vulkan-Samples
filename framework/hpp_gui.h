/* Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
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

#pragma once

#include <gui.h>

#include <hpp_vulkan_sample.h>

namespace vkb
{
class HPPDrawer : private vkb::Drawer
{
  public:
	using vkb::Drawer::checkbox;
	using vkb::Drawer::clear;
	using vkb::Drawer::combo_box;
	using vkb::Drawer::header;
	using vkb::Drawer::input_float;
	using vkb::Drawer::is_dirty;
	using vkb::Drawer::slider_float;
	using vkb::Drawer::text;
};

/**
 * @brief facade class around vkb::Gui, providing a vulkan.hpp-based interface
 *
 * See vkb::Gui for documentation
 */
class HPPGui : private vkb::Gui
{
  public:
	using vkb::Gui::get_drawer;
	using vkb::Gui::input_event;
	using vkb::Gui::is_debug_view_active;
	using vkb::Gui::new_frame;
	using vkb::Gui::resize;
	using vkb::Gui::show_options_window;
	using vkb::Gui::show_simple_window;
	using vkb::Gui::show_top_window;
	using vkb::Gui::update;

	HPPGui(vkb::HPPVulkanSample &          sample,
	       const vkb::platform::HPPWindow &window,
	       const vkb::stats::HPPStats *    stats           = nullptr,
	       const float                     font_size       = 21.0f,
	       bool                            explicit_update = false) :
	    Gui(reinterpret_cast<vkb::VulkanSample &>(sample), reinterpret_cast<vkb::Window const &>(window), reinterpret_cast<vkb::Stats const *>(stats), font_size, explicit_update)
	{}

	void draw(vk::CommandBuffer command_buffer)
	{
		Gui::draw(command_buffer);
	}

	void draw(vkb::core::HPPCommandBuffer &command_buffer)
	{
		Gui::draw(reinterpret_cast<vkb::CommandBuffer &>(command_buffer));
	}

	vkb::HPPDrawer &get_drawer()
	{
		return reinterpret_cast<vkb::HPPDrawer &>(Gui::get_drawer());
	}

	void prepare(const vk::PipelineCache pipeline_cache, const vk::RenderPass render_pass, const std::vector<vk::PipelineShaderStageCreateInfo> &shader_stages)
	{
		std::vector<VkPipelineShaderStageCreateInfo> pssci;
		pssci.reserve(shader_stages.size());
		for (auto const &ci : shader_stages)
		{
			pssci.push_back(*reinterpret_cast<VkPipelineShaderStageCreateInfo const *>(&ci));
		}
		Gui::prepare(pipeline_cache, render_pass, pssci);
	}

	void show_top_window(const std::string &app_name, const vkb::stats::HPPStats *stats = nullptr, DebugInfo *debug_info = nullptr)
	{
		Gui::show_top_window(app_name, reinterpret_cast<vkb::Stats const *>(stats), debug_info);
	}

	bool update_buffers()
	{
		return Gui::update_buffers();
	}
};
}        // namespace vkb
