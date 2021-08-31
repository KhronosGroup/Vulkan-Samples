/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

#include "common/warnings.h"

VKBP_DISABLE_WARNINGS()
#include <android_native_app_glue.h>
VKBP_ENABLE_WARNINGS()

#include "platform/platform.h"

namespace vkb
{
class AndroidPlatform : public Platform
{
  public:
	AndroidPlatform(android_app *app);

	virtual ~AndroidPlatform() = default;

	virtual bool initialize(std::unique_ptr<Application> &&app) override;

	virtual void create_window() override;

	virtual void main_loop() override;

	virtual void terminate(ExitCode code) override;

	virtual const char *get_surface_extension() override;

	/**
	 * @brief Sends a notification in the task bar
	 * @param message The message to display
	 */
	void send_notification(const std::string &message);

	/**
	 * @brief Sends an error notification in the task bar
	 * @param message The message to display
	 */
	void send_error_notification(const std::string &message);

	android_app *get_android_app();

	ANativeActivity *get_activity();

	virtual std::unique_ptr<RenderContext> create_render_context(Device &device, VkSurfaceKHR surface, const std::vector<VkSurfaceFormatKHR> &surface_format_priority) const override;

	void set_surface_ready();

  private:
	void poll_events();

	android_app *app{nullptr};

	std::string log_output;

	virtual std::vector<spdlog::sink_ptr> get_platform_sinks() override;

	bool surface_ready{false};
};
}        // namespace vkb
