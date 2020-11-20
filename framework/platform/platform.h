/* Copyright (c) 2019-2020, Arm Limited and Contributors
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
#include <string>
#include <vector>

#include "apps.h"
#include "common/utils.h"
#include "common/vk_common.h"
#include "platform/application.h"
#include "platform/extensions/extension.h"
#include "platform/extensions/parser.h"
#include "platform/filesystem.h"
#include "platform/window.h"
#include "timer.h"

namespace vkb
{
enum class ExitCode
{
	Success     = 0, /* App prepare succeeded, it ran correctly and exited properly with no errors */
	UnableToRun = 1, /* App prepare failed, could not run */
	FatalError  = 2  /* App encountered an unexpected error */
};

class Platform
{
  public:
	Platform() = default;

	virtual ~Platform() = default;

	/**
	 * @brief Sets up the window and logger
	 * @param app_id The application to prepare after the platform is prepared
	 */
	virtual bool initialize(const std::vector<Extension *> &extensions);

	/**
	 * @brief Prepares the active app supplied in the initialize function
	 */
	virtual bool prepare();

	/**
	 * @brief Handles the main loop of the platform
	 * This should be overriden if a platform requires a specific main loop setup.
	 */
	virtual void main_loop();

	/**
	 * @brief Runs the application for one frame
	 */
	void update();

	/**
	 * @brief Terminates the platform and the application
	 * @param code Determines how the platform should exit
	 */
	virtual void terminate(ExitCode code);

	/**
	 * @brief Requests to close the platform at the next available point
	 */
	virtual void close() const;

	/**
	 * @brief Returns the working directory of the application set by the platform
	 * @returns The path to the working directory
	 */
	static const std::string &get_external_storage_directory();

	/**
	 * @brief Returns the suitable directory for temporary files from the environment variables set in the system
	 * @returns The path to the temp folder on the system
	 */
	static const std::string &get_temp_directory();

	/**
	 * @return The dot-per-inch scale factor
	 */
	virtual float get_dpi_factor() const;

	/**
	 * @return The VkInstance extension name for the platform
	 */
	virtual const char *get_surface_extension() = 0;

	void input_event(const InputEvent &input_event);

	Window &get_window() const;

	void set_window(std::unique_ptr<Window> &&window);

	Application &get_app() const;

	Application &get_app();

	std::vector<std::string> &get_arguments();

	static void set_arguments(const std::vector<std::string> &args);

	static void set_external_storage_directory(const std::string &dir);

	static void set_temp_directory(const std::string &dir);

	void set_width(uint32_t width);

	void set_height(uint32_t height);

	void set_focused(bool focused);

	/**
	 * @brief Handles the creation of the window
	 */
	virtual void create_window() = 0;

	void lock_app_input();

	template <class T>
	bool using_extension() const;

	void request_application(const apps::AppInfo *app);

	bool app_requested();

	bool start_app();

  protected:
	std::vector<Extension *> active_extensions;

	std::unordered_map<Hook, std::vector<Extension *>> hooks;

	std::unique_ptr<Window> window{nullptr};

	std::unique_ptr<Application> active_app{nullptr};

	virtual std::vector<spdlog::sink_ptr> get_platform_sinks();

	uint32_t width{1280};

	uint32_t height{720};

  private:
	Timer timer;

	bool app_focused{true};

	bool pass_input_to_app{true};

	const apps::AppInfo *requested_app{nullptr};

	std::unique_ptr<Parser> parser;

	/// Static so can be set via JNI code in android_platform.cpp
	static std::vector<std::string> arguments;

	static std::string external_storage_directory;

	static std::string temp_directory;

	void call_hook(const Hook &hook, std::function<void(Extension *)> fn) const;
};

template <class T>
bool Platform::using_extension() const
{
	return !extensions::with_tags<T>(active_extensions).empty();
}
}        // namespace vkb
