/* Copyright (c) 2021-2025, Arm Limited and Contributors
 * Copyright (c) 2021-2025, Sascha Willems
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

#include "file_logger.h"

#include "apps.h"

#include <fmt/format.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

namespace plugins
{
FileLogger::FileLogger() : FileLoggerTags("File Logger", "Enable log output to a file.", {}, {}, {{"log-file", "Write log messages to the given file name"}})
{}

bool FileLogger::handle_option(std::deque<std::string> &arguments)
{
	assert(!arguments.empty() && (arguments[0].substr(0, 2) == "--"));
	std::string option = arguments[0].substr(2);
	if (option == "log-file")
	{
		if (arguments.size() < 2)
		{
			LOGE("Option \"log-file\" is missing the actual log file name!");
			return false;
		}
		std::string log_file = arguments[1];

		spdlog::default_logger()->sinks().push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true));

		arguments.pop_front();
		arguments.pop_front();
		return true;
	}
	return false;
}
}        // namespace plugins