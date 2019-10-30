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

#include "platform.h"

#include <ctime>
#include <mutex>
#include <vector>

#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "application.h"
#include "common/logging.h"
#include "platform/filesystem.h"

namespace vkb
{
std::vector<std::string> Platform::arguments = {};

std::string Platform::external_storage_directory = "";

std::string Platform::temp_directory = "";

Platform::Platform()
{
}

bool Platform::initialize(std::unique_ptr<Application> &&app)
{
	assert(app && "Application is not valid");
	active_app = std::move(app);

	// Override initialize_sinks in the derived platforms
	auto sinks = get_platform_sinks();

	auto logger = std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());
	logger->set_pattern(LOGGER_FORMAT);
	spdlog::set_default_logger(logger);

	LOGI("Logger initialized");

	if (active_app->get_options().contains("--benchmark"))
	{
		benchmark_mode             = true;
		total_benchmark_frames     = active_app->get_options().get_int("--benchmark");
		remaining_benchmark_frames = total_benchmark_frames;
		active_app->set_benchmark_mode(true);
	}

	return true;
}

bool Platform::prepare()
{
	if (active_app)
	{
		return active_app->prepare(*this);
	}
	return false;
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
	spdlog::drop_all();
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
	return 1.0;
}

Application &Platform::get_app() const
{
	assert(active_app && "Application is not valid");
	return *active_app;
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
}        // namespace vkb
