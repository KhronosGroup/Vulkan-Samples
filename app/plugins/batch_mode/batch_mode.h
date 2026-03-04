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

#include <chrono>
#include <vector>

#include "apps.h"
#include "platform/plugins/plugin_base.h"
#include "timer.h"

using namespace std::chrono_literals;

namespace plugins
{
using BatchModeTags = vkb::PluginBase<vkb::tags::Entrypoint, vkb::tags::FullControl>;

/**
 * @brief Batch Mode
 *
 * Run a subset of samples. The next sample in the set will start after the current sample being executed has finished. Using --wrap-to-start will start again
 * from the first sample after the last sample is executed.
 *
 * Usage: vulkan_samples batch --duration 3 --category performance --tag arm
 *
 */
class BatchMode : public BatchModeTags
{
  public:
	BatchMode();

	virtual ~BatchMode() = default;

	void on_update(float delta_time) override;
	void on_app_error(const std::string &app_id) override;

	bool handle_command(std::deque<std::string> &arguments) const override;
	bool handle_option(std::deque<std::string> &arguments) override;
	void trigger_command() override;

  private:
	void request_app();
	void load_next_app();

  private:
	std::vector<std::string>                          categories;
	std::chrono::duration<float, vkb::Timer::Seconds> duration     = 3s;
	float                                             elapsed_time = 0.0f;
	std::set<std::string>                             skips;
	std::vector<apps::AppInfo *>::const_iterator      sample_iter;        // An iterator to the current batch mode sample info object
	std::vector<apps::AppInfo *>                      sample_list;        // The list of suitable samples to be run in conjunction with batch mode
	std::vector<std::string>                          tags;
	bool                                              wrap_to_start = false;
};
}        // namespace plugins