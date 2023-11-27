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

#include "platform.h"

#include <algorithm>
#include <ctime>
#include <mutex>
#include <vector>

#include <fmt/format.h>
#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "force_close/force_close.h"
#include "hpp_vulkan_sample.h"
#include "platform/filesystem.h"
#include "platform/parsers/CLI11.h"
#include "platform/plugins/plugin.h"
#include "vulkan_sample.h"

#include <core/util/logging.hpp>

namespace plugins
{
class BenchmarkMode;
}

namespace vkb
{
const uint32_t Platform::MIN_WINDOW_WIDTH  = 420;
const uint32_t Platform::MIN_WINDOW_HEIGHT = 320;

std::string Platform::external_storage_directory = "";

std::string Platform::temp_directory = "";

Platform::Platform(const PlatformContext &context)
{
	arguments = context.arguments();

	external_storage_directory = context.external_storage_directory();
	temp_directory             = context.temp_directory();
}

ExitCode Platform::initialize(const std::vector<Plugin *> &plugins)
{
	vkb::logging::init();

	LOGI("Logger initialized");

	parser = std::make_unique<CLI11CommandParser>("vulkan_samples", "\n\tVulkan Samples\n\n\t\tA collection of samples to demonstrate the Vulkan best practice.\n", arguments);

	// Process command line arguments
	if (!parser->parse(associate_plugins(plugins)))
	{
		return ExitCode::Help;
	}

	// Subscribe plugins to requested hooks and store activated plugins
	for (auto *plugin : plugins)
	{
		if (plugin->activate_plugin(this, *parser.get()))
		{
			auto &plugin_hooks = plugin->get_hooks();
			for (auto hook : plugin_hooks)
			{
				auto it = hooks.find(hook);

				if (it == hooks.end())
				{
					auto r = hooks.emplace(hook, std::vector<Plugin *>{});

					if (r.second)
					{
						it = r.first;
					}
				}

				it->second.emplace_back(plugin);
			}

			active_plugins.emplace_back(plugin);
		}
	}

	// Platform has been closed by a plugins initialization phase
	if (close_requested)
	{
		return ExitCode::Close;
	}

	create_window(window_properties);

	if (!window)
	{
		LOGE("Window creation failed!");
		return ExitCode::FatalError;
	}

	return ExitCode::Success;
}

ExitCode Platform::main_loop()
{
	if (!app_requested())
	{
		LOGI("An app was not requested, can not continue");
		return ExitCode::Close;
	}

	while (!window->should_close() && !close_requested)
	{
		try
		{
			// Load the requested app
			if (app_requested())
			{
				if (!start_app())
				{
					LOGE("Failed to load requested application");
					return ExitCode::FatalError;
				}

				// Compensate for load times of the app by rendering the first frame pre-emptively
				timer.tick<Timer::Seconds>();
				active_app->update(0.01667f);
			}

			update();

			if (active_app && active_app->should_close())
			{
				std::string id = active_app->get_name();
				on_app_close(id);
				active_app->finish();
			}

			window->process_events();
		}
		catch (std::exception &e)
		{
			LOGE("Error Message: {}", e.what());
			LOGE("Failed when running application {}", active_app->get_name());

			on_app_error(active_app->get_name());

			if (app_requested())
			{
				LOGI("Attempting to load next application");
			}
			else
			{
				return ExitCode::FatalError;
			}
		}
	}

	return ExitCode::Success;
}

void Platform::update()
{
	auto delta_time = static_cast<float>(timer.tick<Timer::Seconds>());

	if (focused)
	{
		on_update(delta_time);

		if (fixed_simulation_fps)
		{
			delta_time = simulation_frame_time;
		}

		active_app->update(delta_time);

		if (auto *app = dynamic_cast<VulkanSample *>(active_app.get()))
		{
			if (app->has_render_context())
			{
				on_post_draw(app->get_render_context());
			}
		}
		else if (auto *app = dynamic_cast<HPPVulkanSample *>(active_app.get()))
		{
			if (app->has_render_context())
			{
				on_post_draw(reinterpret_cast<vkb::RenderContext &>(app->get_render_context()));
			}
		}
	}
}

void Platform::terminate(ExitCode code)
{
	if (code == ExitCode::Help)
	{
		auto help = parser->help();
		for (auto &line : help)
		{
			LOGI(line);
		}
	}

	if (active_app)
	{
		std::string id = active_app->get_name();
		on_app_close(id);
		active_app->finish();
	}

	active_app.reset();
	window.reset();

	spdlog::drop_all();

	on_platform_close();

	// Halt on all unsuccessful exit codes unless ForceClose is in use
	if (code != ExitCode::Success && !using_plugin<::plugins::ForceClose>())
	{
#ifndef ANDROID
		std::cout << "Press return to continue";
		std::cin.get();
#endif
	}
}

void Platform::close()
{
	if (window)
	{
		window->close();
	}

	// Fallback incase a window is not yet in use
	close_requested = true;
}

void Platform::force_simulation_fps(float fps)
{
	fixed_simulation_fps  = true;
	simulation_frame_time = 1 / fps;
}

void Platform::disable_input_processing()
{
	process_input_events = false;
}

void Platform::set_focus(bool _focused)
{
	focused = _focused;
}

void Platform::set_window_properties(const Window::OptionalProperties &properties)
{
	window_properties.title         = properties.title.has_value() ? properties.title.value() : window_properties.title;
	window_properties.mode          = properties.mode.has_value() ? properties.mode.value() : window_properties.mode;
	window_properties.resizable     = properties.resizable.has_value() ? properties.resizable.value() : window_properties.resizable;
	window_properties.vsync         = properties.vsync.has_value() ? properties.vsync.value() : window_properties.vsync;
	window_properties.extent.width  = properties.extent.width.has_value() ? properties.extent.width.value() : window_properties.extent.width;
	window_properties.extent.height = properties.extent.height.has_value() ? properties.extent.height.value() : window_properties.extent.height;
}

const std::string &Platform::get_external_storage_directory()
{
	return external_storage_directory;
}

const std::string &Platform::get_temp_directory()
{
	return temp_directory;
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

Window &Platform::get_window()
{
	return *window;
}

void Platform::set_external_storage_directory(const std::string &dir)
{
	external_storage_directory = dir;
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
	auto *requested_app_info = requested_app;
	// Reset early incase error in preparation stage
	requested_app = nullptr;

	if (active_app)
	{
		auto execution_time = timer.stop();
		LOGI("Closing App (Runtime: {:.1f})", execution_time);

		auto app_id = active_app->get_name();

		active_app->finish();
	}

	active_app = requested_app_info->create();

	active_app->set_name(requested_app_info->id);

	if (!active_app)
	{
		LOGE("Failed to create a valid vulkan app.");
		return false;
	}

	if (!active_app->prepare({false, window.get()}))
	{
		LOGE("Failed to prepare vulkan app.");
		return false;
	}

	on_app_start(requested_app_info->id);

	return true;
}

void Platform::input_event(const InputEvent &input_event)
{
	if (process_input_events && active_app)
	{
		active_app->input_event(input_event);
	}

	if (input_event.get_source() == EventSource::Keyboard)
	{
		const auto &key_event = static_cast<const KeyInputEvent &>(input_event);

		if (key_event.get_code() == KeyCode::Back ||
		    key_event.get_code() == KeyCode::Escape)
		{
			close();
		}
	}
}

void Platform::resize(uint32_t width, uint32_t height)
{
	auto extent = Window::Extent{std::max<uint32_t>(width, MIN_WINDOW_WIDTH), std::max<uint32_t>(height, MIN_WINDOW_HEIGHT)};
	if ((window) && (width > 0) && (height > 0))
	{
		auto actual_extent = window->resize(extent);

		if (active_app)
		{
			active_app->resize(actual_extent.width, actual_extent.height);
		}
	}
}

#define HOOK(enum, func)                \
	static auto res = hooks.find(enum); \
	if (res != hooks.end())             \
	{                                   \
		for (auto plugin : res->second) \
		{                               \
			plugin->func;               \
		}                               \
	}

void Platform::on_post_draw(RenderContext &context)
{
	HOOK(Hook::PostDraw, on_post_draw(context));
}

void Platform::on_app_error(const std::string &app_id)
{
	HOOK(Hook::OnAppError, on_app_error(app_id));
}

void Platform::on_update(float delta_time)
{
	HOOK(Hook::OnUpdate, on_update(delta_time));
}

void Platform::on_app_start(const std::string &app_id)
{
	HOOK(Hook::OnAppStart, on_app_start(app_id));
}

void Platform::on_app_close(const std::string &app_id)
{
	HOOK(Hook::OnAppClose, on_app_close(app_id));
}

void Platform::on_platform_close()
{
	HOOK(Hook::OnPlatformClose, on_platform_close());
}

#undef HOOK

}        // namespace vkb
