/* Copyright (c) 2019, Arm Limited and Contributors
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

#include <string>

#include "debug_info.h"
#include "platform/configuration.h"
#include "platform/input_events.h"
#include "platform/options.h"
#include "timer.h"

namespace vkb
{
class Platform;

class Application
{
  public:
	Application();

	virtual ~Application() = default;

	/**
	 * @brief Advances the application forward one frame, calculating the delta time between steps
	 *        and then calling the update method accordingly
	 */
	void step();

	/**
	 * @brief Prepares the application for execution
	 * @param platform The platform the application is being run on
	 */
	virtual bool prepare(Platform &platform);

	/**
	 * @brief Updates the application
	 * @param delta_time The time since the last update
	 */
	virtual void update(float delta_time) = 0;

	/**
	 * @brief Handles cleaning up the application
	 */
	virtual void finish();

	/**
	 * @brief Handles resizing of the window
	 * @param width New width of the window
	 * @param height New height of the window
	 */
	virtual void resize(const uint32_t width, const uint32_t height);

	/**
	 * @brief Handles input events of the window
	 * @param input_event The input event object
	 */
	virtual void input_event(const InputEvent &input_event);

	/**
	 * @brief Parses the arguments against Application::usage
	 * @param args The argument list
	 */
	void parse_options(const std::vector<std::string> &args);

	const std::string &get_name() const;

	void set_name(const std::string &name);

	bool is_benchmark_mode() const;

	void set_benchmark_mode(bool benchmark_mode);

	bool is_headless() const;

	void set_headless(bool headless);

	bool is_focused() const;

	void set_focus(bool flag);

	DebugInfo &get_debug_info();

	const Options &get_options();

  protected:
	float fps{0.0f};

	float frame_time{0.0f};        // In ms

	uint32_t frame_count{0};

	uint32_t last_frame_count{0};

	static std::string usage;

	Options options{};

	static void set_usage(const std::string &usage);

  private:
	std::string name{};

	bool focus{true};

	Timer timer;

	bool benchmark_mode{false};

	bool headless{false};

	// The debug info of the app
	DebugInfo debug_info{};
};
}        // namespace vkb
