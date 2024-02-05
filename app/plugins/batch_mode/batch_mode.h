/* Copyright (c) 2020-2024, Arm Limited and Contributors
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
 * Run a subset of samples. The next sample in the set will start after the current sample being executed has finished. Using --wrap-to-start will start again from the first sample after the last sample is executed.
 *
 * Usage: vulkan_samples batch --duration 3 --category performance --tag arm
 *
 */
class BatchMode : public BatchModeTags
{
  public:
	BatchMode();

	virtual ~BatchMode() = default;

	virtual bool is_active(const vkb::CommandParser &parser) override;

	virtual void init(const vkb::CommandParser &parser) override;

	virtual void on_update(float delta_time) override;

	virtual void on_app_error(const std::string &app_id) override;

	// TODO: Could this be replaced by the stop after plugin?
	vkb::FlagCommand duration_flag{vkb::FlagType::OneValue, "duration", "", "The duration which a configuration should run for in seconds"};

	vkb::FlagCommand wrap_flag{vkb::FlagType::FlagOnly, "wrap-to-start", "", "Once all configurations have run wrap to the start"};

	vkb::FlagCommand tags_flag{vkb::FlagType::ManyValues, "tag", "T", "Filter samples by tags"};

	vkb::FlagCommand categories_flag{vkb::FlagType::ManyValues, "category", "C", "Filter samples by categories"};

	vkb::SubCommand batch_cmd{"batch", "Enable batch mode", {&duration_flag, &wrap_flag, &tags_flag, &categories_flag}};

  private:
	/// The list of suitable samples to be run in conjunction with batch mode
	std::vector<apps::AppInfo *> sample_list{};

	/// An iterator to the current batch mode sample info object
	std::vector<apps::AppInfo *>::const_iterator sample_iter;

	/// The amount of time run per configuration for each sample
	std::chrono::duration<float, vkb::Timer::Seconds> sample_run_time_per_configuration{3s};

	float elapsed_time{0.0f};

	bool wrap_to_start = false;

	void request_app();

	void load_next_app();
};
}        // namespace plugins