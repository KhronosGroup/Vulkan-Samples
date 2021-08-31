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

#include "platform.h"

#include <mutex>
#include <vector>

#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "common/logging.h"
#include "platform/filesystem.h"
#include "platform/parsers/CLI11.h"

namespace vkb
{
std::vector<std::string> Platform::arguments = {};

std::string Platform::external_storage_directory = "";

std::string Platform::temp_directory = "";

PositionalCommand Platform::app("sample", "Start a sample with the given id");
SubCommand        Platform::samples("samples", "List all samples", {});
FlagCommand       Platform::sample(FlagType::OneValue, "sample", "s", "Start a sample --sample/--s ID");
FlagCommand       Platform::test(FlagType::OneValue, "test", "t", "Start a test --test/--t ID");
FlagCommand       Platform::benchmark(FlagType::OneValue, "benchmark", "", "Run a benchmark for a set amount of frames");
FlagCommand       Platform::width(FlagType::OneValue, "width", "", "Set the window width --width WIDTH");
FlagCommand       Platform::height(FlagType::OneValue, "height", "", "Set the window height --height HEIGHT");
FlagCommand       Platform::headless(FlagType::FlagOnly, "headless", "", "Run in headless mode --headless");
FlagCommand       Platform::batch_categories(FlagType::ManyValues, "category", "c", "A category to run in batch mode, --category={api,performance,extensions}");
FlagCommand       Platform::batch_tags(FlagType::ManyValues, "tag", "t", "A tag to run in batch mode, --tag={any,Arm}");
SubCommand        Platform::batch("batch", "Run multiple samples", {&Platform::batch_categories, &Platform::batch_tags});

bool Platform::initialize(std::unique_ptr<Application> &&_app)
{
	assert(_app && "Application is not valid");
	active_app = std::move(_app);

	auto sinks = get_platform_sinks();

	auto logger = std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());

#ifdef VKB_DEBUG
	logger->set_level(spdlog::level::debug);
#else
	logger->set_level(spdlog::level::info);
#endif

	logger->set_pattern(LOGGER_FORMAT);
	spdlog::set_default_logger(logger);

	LOGI("Logger initialized")

	auto args = get_arguments();
	args.insert(args.begin(), "vulkan_samples");
	parser = std::make_unique<CLI11CommandParser>("vulkan_samples", "Vulkan Samples\n\nA collection of samples to demonstrate the Vulkan best practice.\n\nUse [SUBCOMMAND] --help for specific subcommand information\n\n", args);

	CommandGroup window_options("Window Options", {&Platform::width, &Platform::height, &Platform::headless});

	if (!parser->parse({&Platform::app, &Platform::sample, &Platform::test, &Platform::batch, &Platform::samples, &Platform::benchmark, &window_options}))
	{
		auto help = parser->help();
		LOGI("")
		for (auto &line : help)
		{
			LOGI(line)
		}
		LOGI("")
		return false;
	}

	// Set the app to execute as a benchmark
	if (parser->contains(&Platform::benchmark))
	{
		benchmark_mode             = true;
		total_benchmark_frames     = parser->as<uint32_t>(&Platform::benchmark);
		remaining_benchmark_frames = total_benchmark_frames;
		active_app->set_benchmark_mode(true);
	}

	// Set the app as headless
	active_app->set_headless(parser->contains(&Platform::headless));

	create_window();

	if (!window)
	{
		throw std::runtime_error("Window creation failed, make sure platform overrides create_window() and creates a valid window.");
	}

	LOGI("Window created")

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

void Platform::main_loop()
{
	while (!window->should_close())
	{
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
			LOGI("Benchmark completed in {} seconds (ran {} frames, averaged {} fps)", time_taken, total_benchmark_frames, total_benchmark_frames / time_taken)
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

std::unique_ptr<RenderContext> Platform::create_render_context(Device &device, VkSurfaceKHR surface, const std::vector<VkSurfaceFormatKHR> &surface_format_priority) const
{
	assert(!surface_format_priority.empty() && "Surface format priority list must contain at least one preferred surface format");

	auto context = std::make_unique<RenderContext>(device, surface, window->get_width(), window->get_height());

	context->set_surface_format_priority(surface_format_priority);

	context->request_image_format(surface_format_priority[0].format);

	context->set_present_mode_priority({
	    VK_PRESENT_MODE_MAILBOX_KHR,
	    VK_PRESENT_MODE_FIFO_KHR,
	    VK_PRESENT_MODE_IMMEDIATE_KHR,
	});

	context->request_present_mode(VK_PRESENT_MODE_MAILBOX_KHR);

	return context;
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
	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	return sinks;
}
}        // namespace vkb
