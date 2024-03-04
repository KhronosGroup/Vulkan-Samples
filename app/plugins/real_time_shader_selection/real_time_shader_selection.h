/* Copyright (c) 2024, Mobica Limited
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

#include "common/vk_common.h"
#include "platform/plugins/plugin_base.h"
#include <map>
#include <vector>

namespace plugins
{
class RealTimeShaderSelection;

// Passive behaviour
using RealTimeShaderSelectionTags = vkb::PluginBase<RealTimeShaderSelection, vkb::tags::Passive>;

/**
 * @brief Real Time Shader Selection
 *
 * When this option is enabled, the samples get the ability to dynamically choose which shaders are available for a given sample.
 *
 * Usage: vulkan_samples sample afbc --realtimeshaderselection
 *
 */
class RealTimeShaderSelection : public RealTimeShaderSelectionTags
{
  public:
	RealTimeShaderSelection();

	virtual ~RealTimeShaderSelection() = default;

	virtual bool is_active(const vkb::CommandParser &parser) override;

	virtual void init(const vkb::CommandParser &parser) override;

	virtual void on_app_start(const std::string &app_info) override;

	virtual void on_update_ui_overlay(vkb::Drawer &drawer) override;

	vkb::FlagCommand realtimeshaderselection_flag = {vkb::FlagType::FlagOnly, "realtimeshaderselection", "", "Enable dynamic shader selection"};

  private:
	std::vector<std::string> language_names;
	int                      active_shader;
	const int                min_size_for_shaders;
};
}        // namespace plugins