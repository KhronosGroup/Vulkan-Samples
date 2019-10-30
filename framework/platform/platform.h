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

#include <memory>
#include <string>
#include <vector>

#include "application.h"
#include "common/utils.h"
#include "common/vk_common.h"
#include "platform/filesystem.h"

namespace vkb
{
class Application;

enum class ExitCode
{
	Success     = 0, /* App prepare succeeded, it ran correctly and exited properly with no errors */
	UnableToRun = 1, /* App prepare failed, could not run */
	FatalError  = 2  /* App encountered an unexpected error */
};

class Platform
{
  public:
	Platform();

	virtual ~Platform() = default;

	/**
	 * @brief Sets up windowing system and logging
	 * @param app The application to prepare after the platform is prepared
	 */
	virtual bool initialize(std::unique_ptr<Application> &&app);

	/**
	 * @brief Prepares the active app supplied in the initialize function
	 */
	virtual bool prepare();

	/**
	 * @brief Gets a handle from the platform's Vulkan surface 
	 * @param instance The Vulkan instance
	 * @returns A VkSurfaceKHR handle, for use by the application
	 */
	virtual VkSurfaceKHR create_surface(VkInstance instance) = 0;

	/**
	 * @brief Handles the main loop of the platform
	 *        This function is responsible for calling run()
	 */
	virtual void main_loop() = 0;

	/**
	 * @brief Handles the running of the app
	 */
	void run();

	/**
	 * @brief Terminates the platform and the application
	 * @param code Determines how the platform should exit
	 */
	virtual void terminate(ExitCode code);

	/**
	 * @brief Requests to close the platform at the next available point
	 */
	virtual void close() const = 0;

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

	Application &get_app() const;

	std::vector<std::string> &get_arguments();

	static void set_arguments(const std::vector<std::string> &args);

	static void set_external_storage_directory(const std::string &dir);

	static void set_temp_directory(const std::string &dir);

  protected:
	std::unique_ptr<Application> active_app;

	bool benchmark_mode{false};

	uint32_t total_benchmark_frames{0};

	uint32_t remaining_benchmark_frames{0};

	Timer timer;

	virtual std::vector<spdlog::sink_ptr> get_platform_sinks();

  private:
	/// Static so can be set via JNI code in android_platform.cpp
	static std::vector<std::string> arguments;

	static std::string external_storage_directory;

	static std::string temp_directory;
};
}        // namespace vkb
