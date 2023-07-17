/* Copyright (c) 2020-2023, Arm Limited and Contributors
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
    StartSampleTags("Apps",
                    "A collection of flags to samples and apps.",
                    {}, {&sample_subcmd, &samples_subcmd, &samples_oneline_subcmd})
{
}

bool StartSample::is_active(const vkb::CommandParser &parser)
{
	return parser.contains(&sample_cmd) ||
	       parser.contains(&samples_subcmd) ||
	       parser.contains(&samples_oneline_subcmd);
}

void StartSample::init(const vkb::CommandParser &parser)
{
	if (parser.contains(&sample_cmd))
	{
		// Launch Sample
		auto *sample = apps::get_sample(parser.as<std::string>(&sample_cmd));
		if (sample != nullptr)
		{
			vkb::Window::OptionalProperties properties;
			std::string                     title = "Vulkan Samples: " + sample->name;
			properties.title                      = title;
			platform->set_window_properties(properties);
			platform->request_application(sample);
		}
	}
	else if (parser.contains(&samples_subcmd) || parser.contains(&samples_oneline_subcmd))
	{
		// List samples

		auto samples = apps::get_samples();

		LOGI("");
		LOGI("Available Samples");
		LOGI("");

		for (auto *app : samples)
		{
			auto sample = reinterpret_cast<apps::SampleInfo *>(app);
			if (parser.contains(&samples_subcmd))
			{
				LOGI("{}", sample->name.c_str());
				LOGI("\tid: {}", sample->id.c_str());
				LOGI("\tdescription: {}", sample->description.c_str());
				LOGI("");
			}
			else
			{
				LOGI("{}", sample->id.c_str());
			}
		}

		platform->close();
	}
}
}        // namespace plugins