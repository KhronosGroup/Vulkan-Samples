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

#include "start_sample.h"

#include "apps.h"

namespace plugins
{
StartSample::StartSample() :
    StartSampleTags("StartSample", "A collection of flags to samples and apps.", {},
                    {{"sample", "Run a specific sample"},
                     {"samples", "List available samples with descriptions"},
                     {"samples-oneline", "List available samples, one per line"}})
{}

void StartSample::launch_sample(apps::SampleInfo const *sample) const
{
	vkb::Window::OptionalProperties properties;
	properties.title = "Vulkan Samples: " + sample->name;
	platform->set_window_properties(properties);
	platform->request_application(sample);
}

void StartSample::list_samples(bool one_per_line) const
{
	auto samples = apps::get_samples();

	LOGI("");
	LOGI("Available Samples");
	LOGI("");

	for (auto *app : samples)
	{
		auto sample = reinterpret_cast<apps::SampleInfo *>(app);
		if (one_per_line)
		{
			LOGI("{}", sample->id.c_str());
		}
		else
		{
			LOGI("{}", sample->name.c_str());
			LOGI("\tid: {}", sample->id.c_str());
			LOGI("\tdescription: {}", sample->description.c_str());
			LOGI("");
		}
	}

	platform->close();
}

bool StartSample::handle_command(std::deque<std::string> &arguments) const
{
	assert(!arguments.empty());
	if (arguments[0] == "sample")
	{
		if (arguments.size() < 2)
		{
			LOGE("Command \"sample\" is missing the actual sample_id to launch!");
			return false;
		}
		auto *sample = apps::get_sample(arguments[1]);
		if (!sample)
		{
			LOGE("Command \"sample\" is called with an unknown sample_id \"{}\"!", arguments[1]);
			return false;
		}
		launch_sample(sample);
		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	if ((arguments[0] == "samples") || (arguments[0] == "samples-oneline"))
	{
		list_samples(arguments[0] == "samples-oneline");
		arguments.pop_front();
		return true;
	}
	return false;
}
}        // namespace plugins