/* Copyright (c) 2018-2019, Arm Limited and Contributors
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

#include <memory>

#include "platform/application.h"
#include "samples.h"
#include "tests.h"
#include "vulkan_sample.h"

namespace vkb
{
using CreateAppFunc = std::function<std::unique_ptr<vkb::Application>()>;

class VulkanSamples : public Application
{
  public:
	VulkanSamples();

	virtual ~VulkanSamples() = default;

	virtual bool prepare(Platform &platform) override;

	virtual void update(float delta_time) override;

	virtual void finish() override;

	virtual void resize(const uint32_t width, const uint32_t height) override;

	virtual void input_event(const InputEvent &input_event) override;

	/**
	 * @brief Prepares a sample or a test to be run under certain conditions
	 * @param run_info A struct containing the information needed to run
	 * @returns true if the preparation was a success, false if there was a failure
	 */
	bool prepare_active_app(CreateAppFunc create_app_func, const std::string &name, bool test, bool batch);

  private:
	/// Platform pointer
	Platform *platform;

	/// The actual sample that the vulkan samples controls
	std::unique_ptr<Application> active_app{nullptr};

	/// The list of suitable samples to be run in conjunction with batch mode
	std::vector<SampleInfo> batch_mode_sample_list{};

	/// An iterator to the current batch mode sample info object
	std::vector<SampleInfo>::const_iterator batch_mode_sample_iter;

	/// If batch mode is enabled
	bool batch_mode{false};

	/// The first frame is skipped as we don't want to include the prepare time
	bool skipped_first_frame{false};

	/// The amount of time run per configuration for each sample
	float sample_run_time_per_configuration{3.0f};

	/// Used to calculate when the sample has exceeded the sample_run_time_per_configuration
	float elapsed_time{0.0f};
};

}        // namespace vkb

std::unique_ptr<vkb::Application> create_vulkan_samples();
