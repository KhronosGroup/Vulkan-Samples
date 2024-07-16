/* Copyright (c) 2018-2024, Arm Limited and Contributors
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

#include <iomanip>        // setprecision
#include <sstream>        // stringstream

#include "scene_graph/components/perspective_camera.h"
#include "vulkan_sample.h"

/**
 * @brief Appropriate use of render pass attachment operations
 */
class RenderPassesSample : public vkb::VulkanSampleC
{
  public:
	RenderPassesSample();

	bool prepare(const vkb::ApplicationOptions &options) override;

	void draw_gui() override;

	/**
	 * @brief Struct that contains radio button labeling
	 * and the value which one is selected
	 */
	struct RadioButtonGroup
	{
		const char *description;

		std::vector<const char *> options;

		int value;
	};

	struct CheckBox
	{
		const char *description;

		bool value;
	};

  private:
	void reset_stats_view() override;

	void draw_renderpass(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target) override;

	vkb::sg::PerspectiveCamera *camera{nullptr};

	// Whether to use vkCmdClear or not
	bool cmd_clear = false;

	RadioButtonGroup load{
	    "Color attachment load operation",
	    {"Load", "Clear", "Don't care"},
	    0};

	RadioButtonGroup store{
	    "Depth attachment store operation",
	    {"Store", "Don't care"},
	    1};

	std::vector<RadioButtonGroup *> radio_buttons = {&load, &store};

	float frame_rate;
};

std::unique_ptr<vkb::VulkanSampleC> create_render_passes();
