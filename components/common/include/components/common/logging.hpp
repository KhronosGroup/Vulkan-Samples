/* Copyright (c) 2018-2022, Arm Limited and Contributors
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

#pragma once

#include <string_view>

#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <fmt/format.h>
#include <spdlog/spdlog.h>
VKBP_ENABLE_WARNINGS()

#define PROJECT_NAME "VulkanSamples"

#define LOGT(...) spdlog::trace(__VA_ARGS__)
#define LOGI(...) spdlog::info(__VA_ARGS__)
#define LOGW(...) spdlog::warn(__VA_ARGS__)
#define LOGE(...) spdlog::error(__VA_ARGS__)
#define LOGD(...) spdlog::debug(__VA_ARGS__)

namespace components
{
namespace logging
{
namespace colors
{
// Formatting codes
inline const std::string_view reset      = "\033[m";
inline const std::string_view bold       = "\033[1m";
inline const std::string_view dark       = "\033[2m";
inline const std::string_view underline  = "\033[4m";
inline const std::string_view blink      = "\033[5m";
inline const std::string_view reverse    = "\033[7m";
inline const std::string_view concealed  = "\033[8m";
inline const std::string_view clear_line = "\033[K";

// Foreground colors
inline const std::string_view black   = "\033[30m";
inline const std::string_view red     = "\033[31m";
inline const std::string_view green   = "\033[32m";
inline const std::string_view yellow  = "\033[33m";
inline const std::string_view blue    = "\033[34m";
inline const std::string_view magenta = "\033[35m";
inline const std::string_view cyan    = "\033[36m";
inline const std::string_view white   = "\033[37m";

/// Background colors
inline const std::string_view on_black   = "\033[40m";
inline const std::string_view on_red     = "\033[41m";
inline const std::string_view on_green   = "\033[42m";
inline const std::string_view on_yellow  = "\033[43m";
inline const std::string_view on_blue    = "\033[44m";
inline const std::string_view on_magenta = "\033[45m";
inline const std::string_view on_cyan    = "\033[46m";
inline const std::string_view on_white   = "\033[47m";

/// Bold colors
inline const std::string_view yellow_bold = "\033[33m\033[1m";
inline const std::string_view red_bold    = "\033[31m\033[1m";
inline const std::string_view bold_on_red = "\033[1m\033[41m";
}        // namespace colors

void init_default_logger();
}        // namespace logging
}        // namespace components