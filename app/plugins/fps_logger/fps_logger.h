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
using FpsLoggerTags = vkb::PluginBase<vkb::tags::Passive>;

/**
 * @brief FPS Logger
 *
 * Control when FPS should be logged. Declutters the log output by removing FPS logs when not enabled
 *
 * Usage: vulkan_sample sample afbc --log-fps
 *
 */
class FpsLogger : public FpsLoggerTags
{
  public:
	FpsLogger();

	virtual ~FpsLogger() = default;

	void on_update(float delta_time) override;

	bool handle_option(std::deque<std::string> &arguments) override;

  private:
	size_t     frame_count      = 0;
	size_t     last_frame_count = 0;
	vkb::Timer timer;
};
}        // namespace plugins