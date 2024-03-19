/* Copyright (c) 2024, Thomas Atkinson
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

#include "core/util/logging.hpp"

#include "spdlog/cfg/env.h"

#ifdef PLATFORM__ANDROID
#	include "spdlog/sinks/android_sink.h"
#else
#	include "spdlog/sinks/stdout_color_sinks.h"
#endif

namespace vkb
{
namespace logging
{
void init()
{
	// Taken from "spdlog/cfg/env.h" and renamed SPDLOG_LEVEL to VKB_LOG_LEVEL
	auto env_val = spdlog::details::os::getenv("VKB_LOG_LEVEL");
	if (!env_val.empty())
	{
		spdlog::cfg::helpers::load_levels(env_val);
	}

#ifdef PLATFORM__ANDROID
	auto logger = spdlog::android_logger_mt("vkb", "VulkanSamples");
#else
	auto logger = spdlog::stdout_color_mt("vkb");
#endif

	logger->set_pattern(LOGGER_FORMAT);
	logger->set_level(spdlog::level::trace);
	spdlog::set_default_logger(logger);
}
}        // namespace logging
}        // namespace vkb