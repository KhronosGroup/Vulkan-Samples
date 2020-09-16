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

#include "platform.h"

#include <ctime>
#include <mutex>
#include <vector>

#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "common/logging.h"
#include "platform/filesystem.h"

namespace vkb
{
std::vector<std::string> Platform::arguments = {};

std::string Platform::external_storage_directory = "";

std::string Platform::temp_directory = "";

// TEMP - Required as the platform configures a window by using the active app is_headless

class DummyApp : public Application
{
  public:
	DummyApp() :
	    Application()
	{}
	virtual ~DummyApp()
	{}

	virtual void update(float delta_time){};
};

// TEMP

bool Platform::initialize()
{
	auto sinks = get_platform_sinks();

	auto logger = std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());

#ifdef VKB_DEBUG
	logger->set_level(spdlog::level::debug);
#else
	logger->set_level(spdlog::level::info);
#endif

	logger->set_pattern(LOGGER_FORMAT);
	spdlog::set_default_logger(logger);

	LOGI("Logger initialized");

	// TEMP - Not needed after later PRs

	active_app = std::make_unique<DummyApp>();
	
	auto app = apps::get_apps()[2];

	request_application(app);

	// TEMP

	return true;
}

bool Platform::prepare()
{
	// width, height can be overriden by the "WindowSize" extension
	create_window();

	if (!window)
	{
		throw std::runtime_error("Window creation failed, make sure platform overrides create_window() and creates a valid window.");
	}

	LOGI("Window created");

	if (!window)
	{
		LOGE("Window creation failed!");
		return false;
	}

	if (!app_requested())
	{
		LOGE("An app was not requested, can not continue");
		return false;
	}

	return true;
}

void Platform::main_loop()
{
	while (!window->should_close())
	{
		// Load or swap running application
		if (app_requested())
		{
			if (!start_app())
			{
				throw std::runtime_error{"Failed to load Application"};
			}
		}

		run();

		window->process_events();
	}
}

void Platform::run()
{
	if (benchmark_mode)
	{
		timer.start();

		if (remaining_benchmark_frames == 0)
		{
			auto time_taken = timer.stop();
			LOGI("Benchmark completed in {} seconds (ran {} frames, averaged {} fps)", time_taken, total_benchmark_frames, total_benchmark_frames / time_taken);
			close();
			return;
		}
	}

	if (active_app->is_focused() || active_app->is_benchmark_mode())
	{
		active_app->step();
		remaining_benchmark_frames--;
	}
}

void Platform::terminate(ExitCode code)
{
	if (active_app)
	{
		active_app->finish();
	}

	active_app.reset();
	window.reset();

	spdlog::drop_all();
}

void Platform::close() const
{
	window->close();
}

const std::string &Platform::get_external_storage_directory()
{
	return external_storage_directory;
}

const std::string &Platform::get_temp_directory()
{
	return temp_directory;
}

float Platform::get_dpi_factor() const
{
	return window->get_dpi_factor();
}

Application &Platform::get_app() const
{
	assert(active_app && "Application is not valid");
	return *active_app;
}

Window &Platform::get_window() const
{
	return *window;
}

std::vector<std::string> &Platform::get_arguments()
{
	return Platform::arguments;
}

void Platform::set_arguments(const std::vector<std::string> &args)
{
	arguments = args;
}

void Platform::set_external_storage_directory(const std::string &dir)
{
	external_storage_directory = dir;
}

void Platform::set_temp_directory(const std::string &dir)
{
	temp_directory = dir;
}

std::vector<spdlog::sink_ptr> Platform::get_platform_sinks()
{
	return {};
}

bool Platform::app_requested()
{
	return requested_app != nullptr;
}

void Platform::request_application(const apps::AppInfo *app)
{
	requested_app = app;
}

bool Platform::start_app()
{
	if (active_app)
	{
		auto execution_time = timer.stop();
		LOGI("Closing App (Runtime: {:.1f})", execution_time);

		auto app_id = active_app->get_name();

		active_app->finish();

		active_app.reset();
	}

	active_app = requested_app->create();

	active_app->set_name(requested_app->id);

	if (!active_app)
	{
		LOGE("Failed to create a valid vulkan app.");
		return false;
	}

	if (!active_app->prepare(*this))
	{
		LOGE("Failed to prepare vulkan app.");
		return false;
	}

	requested_app = nullptr;
	return true;
}

}        // namespace vkb
