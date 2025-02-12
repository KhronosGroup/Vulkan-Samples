/* Copyright (c) 2020-2025, Arm Limited and Contributors
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

#include "platform/platform.h"
#include "platform/plugins/plugin_base.h"

namespace plugins
{
using StartSampleTags = vkb::PluginBase<vkb::tags::Entrypoint>;

/**
 * @brief Start App
 *
 * Loads a given sample
 *
 * Usage: vulkan_sample sample afbc
 *
 * TODO: Could this be extended to allow configuring a sample from the command line? Currently options are set explicitly by the UI
 */
class StartSample : public StartSampleTags
{
  public:
	StartSample();

	virtual ~StartSample() = default;

	bool handle_command(std::deque<std::string> &arguments) const override;

  private:
	void launch_sample(apps::SampleInfo const *sample) const;
	void list_samples(bool one_per_line) const;
};
}        // namespace plugins
