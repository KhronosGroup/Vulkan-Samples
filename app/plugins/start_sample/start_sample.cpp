/* Copyright (c) 2020-2021, Arm Limited and Contributors
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
                    {}, {&sample_subcmd})
{
}

bool StartSample::is_active(const vkb::CommandParser &parser)
{
	return parser.contains(&sample_cmd);
}

void StartSample::init(const vkb::CommandParser &parser)
{
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
}        // namespace plugins