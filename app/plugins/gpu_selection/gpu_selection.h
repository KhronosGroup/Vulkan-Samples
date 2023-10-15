/* Copyright (c) 2023, Sascha Willems
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
class GpuSelection;

using GpuSelectionTags = vkb::PluginBase<GpuSelection, vkb::tags::Passive>;

/**
 * @brief GPU selection options
 *
 * Explicitly select a GPU to run the samples on
 *
 */
class GpuSelection : public GpuSelectionTags
{
  public:
	GpuSelection();

	virtual ~GpuSelection() = default;

	virtual bool is_active(const vkb::CommandParser &parser) override;

	virtual void init(const vkb::CommandParser &options) override;

	vkb::FlagCommand selected_gpu_index = {vkb::FlagType::OneValue, "gpu", "", "If flag is set, hides the user interface at startup"};

	vkb::CommandGroup gpu_selection_options_group = {"GPU selection Options", {&selected_gpu_index}};
};
}        // namespace plugins