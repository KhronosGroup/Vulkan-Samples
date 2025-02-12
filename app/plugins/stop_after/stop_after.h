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

#include "platform/plugins/plugin_base.h"

namespace plugins
{
using StopAfterTags = vkb::PluginBase<vkb::tags::Stopping>;

/**
 * @brief Stop After
 *
 * Stop the execution of the app after a specific frame.
 *
 * Usage: vulkan_sample sample afbc --stop-after-frame 100
 *
 * TODO: Add stop after duration
 *
 */
class StopAfter : public StopAfterTags
{
  public:
	StopAfter();

	virtual ~StopAfter() = default;

	void on_update(float delta_time) override;

	bool handle_option(std::deque<std::string> &arguments) override;

  private:
	uint32_t remaining_frames{0};
};
}        // namespace plugins