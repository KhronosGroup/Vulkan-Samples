/* Copyright (c) 2020-2025, Arm Limited and Contributors
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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

#include "batch_mode.h"
#include "vulkan_sample.h"

namespace plugins
{
BatchMode::BatchMode() :
    BatchModeTags("Batch Mode",
                  "Run a collection of samples in sequence.",
                  {
                      vkb::Hook::OnUpdate,
                      vkb::Hook::OnAppError,
                  },
                  {{"batch", "Enable batch mode"}},
                  {{"category", "Filter samples by categories"},
                   {"duration", "The duration which a configuration should run for in seconds"},
                   {"skip", "Skip a sample by id"},
                   {"tag", "Filter samples by tags"},
                   {"wrap-to-start", "Once all configurations have run wrap to the start"}})
{
}

bool BatchMode::handle_command(std::deque<std::string> &arguments) const
{
	assert(!arguments.empty());
	if (arguments[0] == "batch")
	{
		arguments.pop_front();
		return true;
	}
	return false;
}

bool BatchMode::handle_option(std::deque<std::string> &arguments)
{
	assert(!arguments.empty() && (arguments[0].substr(0, 2) == "--"));
	std::string option = arguments[0].substr(2);
	if (option == "category")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"category\" is missing the actual category!");
			return false;
		}
		std::string category = arguments[1];
		if (std::ranges::any_of(categories, [&category](auto const &c) { return c == category; }))
		{
			LOGW("Option \"category\" lists category \"{}\" multiple times!", category)
		}
		else
		{
			categories.push_back(category);
		}

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	else if (option == "duration")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"duration\" is missing the actual duration!");
			return false;
		}
		duration = std::chrono::duration<float, vkb::Timer::Seconds>{std::stof(arguments[1])};

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	else if (option == "skip")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"skip\" is missing the sample_id to skip!");
			return false;
		}
		std::string sample_id = arguments[1];
		if (!skips.insert(sample_id).second)
		{
			LOGW("Option \"skip\" lists sample_id \"{}\" multiple times!", sample_id)
		}

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	else if (option == "tag")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"tag\" is missing the actual to tag!");
			return false;
		}
		std::string tag = arguments[1];
		if (std::ranges::any_of(tags, [&tag](auto const &t) { return t == tag; }))
		{
			LOGW("Option \"tag\" lists tag \"{}\" multiple times!", tag)
		}
		else
		{
			tags.push_back(tag);
		}

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	else if (option == "wrap-to-start")
	{
		wrap_to_start = true;

		arguments.pop_front();
		return true;
	}
	return false;
}

void BatchMode::trigger_command()
{
	sample_list = apps::get_samples(categories, tags);

	if (!skips.empty())
	{
		std::vector<apps::AppInfo *> filtered_list;
		filtered_list.reserve(sample_list.size() - skips.size());

		std::copy_if(
		    sample_list.begin(), sample_list.end(), std::back_inserter(filtered_list), [&](const apps::AppInfo *app) { return !skips.count(app->id); });

		if (filtered_list.size() != sample_list.size())
		{
			sample_list.swap(filtered_list);
		}
	}

	if (sample_list.empty())
	{
		LOGE("No samples found")
		throw std::runtime_error{"Can not continue"};
	}

	sample_iter = sample_list.begin();

	vkb::Window::OptionalProperties properties;
	properties.resizable = false;
	platform->set_window_properties(properties);
	platform->disable_input_processing();
	platform->force_render(true);
	request_app();
}

void BatchMode::on_update(float delta_time)
{
	elapsed_time += delta_time;

	// When the runtime for the current configuration is reached, advance to the next config or next sample
	if (elapsed_time >= duration.count())
	{
		elapsed_time = 0.0f;

		// Only check and advance the config if the application is a vulkan sample
		if (auto *vulkan_app = dynamic_cast<vkb::VulkanSampleC *>(&platform->get_app()))
		{
			auto &configuration = vulkan_app->get_configuration();

			if (configuration.next())
			{
				configuration.set();
				return;
			}
		}

		// Cycled through all configs, load next app
		load_next_app();
	}
}

void BatchMode::on_app_error(const std::string &app_id)
{
	// App failed, load next app
	load_next_app();
}

void BatchMode::request_app()
{
	LOGI("===========================================");
	LOGI("Running {}", (*sample_iter)->id);
	LOGI("===========================================");

	platform->request_application((*sample_iter));
}

void BatchMode::load_next_app()
{
	// Wrap it around to the start
	++sample_iter;
	if (sample_iter == sample_list.end())
	{
		if (wrap_to_start)
		{
			sample_iter = sample_list.begin();
		}
		else
		{
			platform->close();
			return;
		}
	}

	// App will be started before the next update loop
	request_app();
}
}        // namespace plugins
