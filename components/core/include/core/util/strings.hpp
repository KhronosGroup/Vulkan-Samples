/* Copyright (c) 2023, Thomas Atkinson
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

#include <string>
#include <vector>

namespace vkb
{
/**
 * @brief Replaces all occurrences of a substring with another substring.
 */
std::string replace_all(std::string str, const std::string &from, const std::string &to);

/**
 * @brief Removes all occurrences of a set of characters from the end of a string.
 */
std::string trim_right(const std::string &str, const std::string &chars = " ");

/**
 * @brief Removes all occurrences of a set of characters from the beginning of a string.
 */
std::string trim_left(const std::string &str, const std::string &chars = " ");

/**
 * @brief Split a string into a vector of strings.
 */
std::vector<std::string> split(const std::string &str, const std::string &delim = " ");

/**
 * @brief Converts a string to snake_case
 */
std::string to_snake_case(const std::string &text);

/**
 * @brief Convert a string to upper case
 */
std::string to_upper_case(const std::string &text);

/**
 * @brief Check that the string ends with the given suffix
 */
bool ends_with(const std::string &str, const std::string &suffix, bool case_sensitive = true);
}        // namespace vkb