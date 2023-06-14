/* Copyright (c) 2019-2023, Arm Limited and Contributors
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
#include "timer.h"

namespace vkb
{
class Window;

struct ApplicationOptions
{
	bool    benchmark_enabled{false};
	Window *window{nullptr};
};

class Application
{
  public:
	Application();

	virtual ~Application() = default;

	/**
	 * @brief Prepares the application for execution
	 */
	virtual bool prepare(const ApplicationOptions &options);

	/**
	 * @brief Updates the application
	 * @param delta_time The time since the last update
	 */
	virtual void update(float delta_time);

	/**
	 * @brief Handles cleaning up the application
	 */
	virtual void finish();

	/**
	 * @brief Handles resizing of the window
	 * @param width New width of the window
	 * @param height New height of the window
	 */
	virtual bool resize(const uint32_t width, const uint32_t height);

	/**
	 * @brief Handles input events of the window
	 * @param input_event The input event object
	 */
	virtual void input_event(const InputEvent &input_event);

	const std::string &get_name() const;

	void set_name(const std::string &name);

	DebugInfo &get_debug_info();

	inline bool should_close() const
	{
		return requested_close;
	}

	// request the app to close
	// does not guarantee that the app will close immediately
	inline void close()
	{
		requested_close = true;
	}

  protected:
	float fps{0.0f};

	float frame_time{0.0f};        // In ms

	uint32_t frame_count{0};

	uint32_t last_frame_count{0};

	bool lock_simulation_speed{false};

	Window *window{nullptr};

  private:
	std::string name{};

	// The debug info of the app
	DebugInfo debug_info{};

	bool requested_close{false};
};
}        // namespace vkb
