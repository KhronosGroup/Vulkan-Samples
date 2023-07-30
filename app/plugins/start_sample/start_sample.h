/* Copyright (c) 2020-2023, Arm Limited and Contributors
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

	virtual bool is_active(const vkb::CommandParser &parser) override;

	virtual void init(const vkb::CommandParser &parser) override;

	vkb::PositionalCommand sample_cmd             = {"sample_id", "ID of the sample to run"};
	vkb::SubCommand        sample_subcmd          = {"sample", "Run a specific sample", {&sample_cmd}};
	vkb::SubCommand        samples_subcmd         = {"samples", "List available samples with descriptions", {}};
	vkb::SubCommand        samples_oneline_subcmd = {"samples-oneline", "List available samples, one per line", {}};
};
}        // namespace plugins
