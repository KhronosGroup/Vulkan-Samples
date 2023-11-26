/* Copyright (c) 2021-2023, Arm Limited and Contributors
 * Copyright (c) 2021-2023, Sascha Willems
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

VKBP_DISABLE_WARNINGS()
#include <fmt/format.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
VKBP_ENABLE_WARNINGS()

namespace plugins
{
FileLogger::FileLogger() :
    FileLoggerTags("File Logger",
                   "Enable log output to a file.",
                   {}, {&log_file_flag})
{
}

bool FileLogger::is_active(const vkb::CommandParser &parser)
{
	return parser.contains(&log_file_flag);
}

void FileLogger::init(const vkb::CommandParser &parser)
{
	if (parser.contains(&log_file_flag))
	{
		if (spdlog::default_logger())
		{
			std::string log_file_name = parser.as<std::string>(&log_file_flag);
			spdlog::default_logger()->sinks().push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file_name, true));
		}
	}
}
}        // namespace plugins