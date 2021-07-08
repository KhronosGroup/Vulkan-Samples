/* Copyright (c) 2020-2021, Arm Limited and Contributors
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
using StartAppTags = vkb::PluginBase<vkb::tags::Entrypoint>;
class StartApp : public StartAppTags
{
  public:
	StartApp();

	virtual ~StartApp() = default;

	virtual bool is_active(const vkb::CommandParser &parser) override;

	virtual void init(const vkb::CommandParser &parser) override;

	vkb::PositionalCommand sample_cmd = {"sample_id", "ID of the sample to run"};
	vkb::SubCommand        sample_subcmd = {"sample", "Run a specific sample", {&sample_cmd}};
};
}        // namespace plugins
