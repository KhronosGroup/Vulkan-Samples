/* Copyright (c) 2023-2025, Sascha Willems
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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

	bool handle_option(std::deque<std::string> &arguments) override;
};
}        // namespace plugins