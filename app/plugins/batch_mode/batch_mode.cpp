/* Copyright (c) 2020-2024, Arm Limited and Contributors
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

#include "platform/parser.h"

namespace plugins
{
using BatchModeSampleIter = std::vector<apps::AppInfo *>::const_iterator;

BatchMode::BatchMode() :
    BatchModeTags("Batch Mode",
                  "Run a collection of samples in sequence.",
                  {
                      vkb::Hook::OnUpdate,
                      vkb::Hook::OnAppError,
                  },
                  {&batch_cmd})
{
}

bool BatchMode::is_active(const vkb::CommandParser &parser)
{
	return parser.contains(&batch_cmd);
}

void BatchMode::init(const vkb::CommandParser &parser)
{
	if (parser.contains(&duration_flag))
	{
		sample_run_time_per_configuration = std::chrono::duration<float, vkb::Timer::Seconds>{parser.as<float>(&duration_flag)};
	}

	if (parser.contains(&wrap_flag))
	{
		wrap_to_start = true;
	}

	std::vector<std::string> tags;
	if (parser.contains(&tags_flag))
	{
		tags = parser.as<std::vector<std::string>>(&tags_flag);
	}

	std::vector<std::string> categories;
	if (parser.contains(&categories_flag))
	{
		categories = parser.as<std::vector<std::string>>(&categories_flag);
	}

	std::unordered_set<std::string> skips;
	if (parser.contains(&skip_flag))
	{
		skips = parser.as<std::unordered_set<std::string>>(&skip_flag);
	}

	sample_list = apps::get_samples(categories, tags);
	if (!skips.empty())
	{
		std::vector<apps::AppInfo *> filtered_list;
		filtered_list.reserve(sample_list.size());

		std::copy_if(
		    sample_list.begin(), sample_list.end(),
		    std::back_inserter(filtered_list),
		    [&](const apps::AppInfo *app) {
			    return skips.find(app->id) == skips.end();
		    });

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
	if (elapsed_time >= sample_run_time_per_configuration.count())
	{
		elapsed_time = 0.0f;

		// Only check and advance the config if the application is a vulkan sample
		if (auto *vulkan_app = dynamic_cast<vkb::VulkanSample<vkb::BindingType::C> *>(&platform->get_app()))
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
