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

#pragma once

#include "rendering/render_pipeline.h"
#include "scene_graph/components/perspective_camera.h"
#include "vulkan_sample.h"

class DescriptorManagement : public vkb::VulkanSample
{
  public:
	DescriptorManagement();

	bool prepare(vkb::Platform &platform) override;

	~DescriptorManagement() override = default;

	void update(float delta_time) override;

  private:
	/**
	  * @brief Struct that contains radio button labeling and the value
	  *        which is selected
	  */
	struct RadioButtonGroup
	{
		const char *              description;
		std::vector<const char *> options;
		int                       value;
	};

	RadioButtonGroup descriptor_caching{
	    "Descriptor set caching",
	    {"Disabled", "Enabled"},
	    0};

	RadioButtonGroup buffer_allocation{
	    "Single large VkBuffer",
	    {"Disabled", "Enabled"},
	    0};

	std::vector<RadioButtonGroup *> radio_buttons = {&descriptor_caching, &buffer_allocation};

	vkb::sg::PerspectiveCamera *camera{nullptr};

	void draw_gui() override;
};

std::unique_ptr<vkb::VulkanSample> create_descriptor_management();
