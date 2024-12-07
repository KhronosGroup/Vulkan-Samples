/* Copyright (c) 2024, Sascha Willems
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

#include "platform/plugins/plugin_base.h"

namespace plugins
{
class ShadingLanguageSelection;

using ShadingLanguageSelectionTags = vkb::PluginBase<ShadingLanguageSelection, vkb::tags::Passive>;

/**
 * @brief Shading language selection options
 *
 * Select what shading language to run the samples with (glsl, hlsl)
 *
 */
class ShadingLanguageSelection : public ShadingLanguageSelectionTags
{
  public:
	ShadingLanguageSelection();

	virtual ~ShadingLanguageSelection() = default;

	virtual bool is_active(const vkb::CommandParser &parser) override;

	virtual void init(const vkb::CommandParser &options) override;

	vkb::FlagCommand selected_shading_language = {vkb::FlagType::OneValue, "shading-language", "", "Shading language to use (glsl or hlsl)"};

	vkb::CommandGroup shading_language_selection_options_group = {"Shading langauge selection Options", {&selected_shading_language}};
};
}        // namespace plugins