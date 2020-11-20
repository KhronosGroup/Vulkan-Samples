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

#include <algorithm>
#include <ctime>
#include <mutex>
#include <vector>

#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "common/logging.h"
#include "platform/extensions/extension.h"
#include "platform/filesystem.h"

namespace vkb
{
std::vector<std::string> Platform::arguments = {};

std::string Platform::external_storage_directory = "";

std::string Platform::temp_directory = "";

bool Platform::initialize(const std::vector<Extension *> &extensions = {})
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

	parser = std::make_unique<Parser>(extensions);

	// Process command line arguments
	if (!parser->parse(arguments))
	{
		return false;
	}

	// Subscribe extensions to requested hooks and store activated extensions
	for (auto &extension : extensions)
	{
		if (extension->activate_extension(*this, *parser.get()))
		{
			auto &extension_hooks = extension->get_hooks();
			for (auto hook : extension_hooks)
			{
				auto it = hooks.find(hook);

				if (it == hooks.end())
				{
					auto r = hooks.emplace(hook, std::vector<Extension *>{});

					if (r.second)
					{
						it = r.first;
					}
				}

				it->second.emplace_back(extension);
			}

			active_extensions.emplace_back(extension);
		}
	}

	return true;
}

bool Platform::prepare()
{
	// width, height can be overriden by the "WindowSize" extension
	create_window();

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

		update();

		window->process_events();
	}
}

void Platform::update()
{
	auto delta_time = static_cast<float>(timer.tick<Timer::Seconds>());

	if (app_focused)
	{
		call_hook(Hook::OnUpdate, [&delta_time](Extension *extension) { extension->on_update(delta_time); });

		if (active_app->is_benchmark_mode())
		{
			// Benchmark Mode to make the run as reproducible as possible fix the delta time
			// Skip first frame delta time so that there is not a large jump in delta time
			delta_time = 0.01667f;
		}

		active_app->update(delta_time);
	}
}

void Platform::terminate(ExitCode code)
{
	if (code == ExitCode::UnableToRun)
	{
		parser->print_help();
	}

	if (active_app)
	{
		std::string id = active_app->get_name();

		call_hook(Hook::OnAppClose, [&id](Extension *extension) { extension->on_app_close(id); });

		active_app->finish();
	}

	active_app.reset();
	window.reset();

	spdlog::drop_all();
}

void Platform::close() const
{
	if (window)
	{
		window->close();
	}

	call_hook(Hook::OnPlatformClose, [](Extension *extension) { extension->on_platform_close(); });
}

void Platform::call_hook(const Hook &hook, std::function<void(Extension *)> fn) const
{
	auto res = hooks.find(hook);
	if (res != hooks.end())
	{
		for (auto extension : res->second)
		{
			fn(extension);
		}
	}
}

void Platform::set_focused(bool focused)
{
	app_focused = focused;
}

void Platform::set_window(std::unique_ptr<Window> &&window)
{
	this->window = std::move(window);
}

const std::string &
    Platform::get_external_storage_directory()
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

Application &Platform::get_app()
{
	assert(active_app && "Application is not valid");
	return *active_app;
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

void Platform::input_event(const InputEvent &input_event)
{
	// TODO: Move to desktop platform
	if (input_event.get_source() == EventSource::Keyboard)
	{
		const auto &key_event = static_cast<const KeyInputEvent &>(input_event);

		if (key_event.get_code() == KeyCode::Back ||
		    key_event.get_code() == KeyCode::Escape)
		{
			close();
		}
	}

	if (pass_input_to_app && active_app)
	{
		active_app->input_event(input_event);
	}
}

void Platform::set_width(uint32_t width)
{
	this->width = std::max<uint32_t>(width, 420);
}

void Platform::set_height(uint32_t height)
{
	this->height = std::max<uint32_t>(height, 320);
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
