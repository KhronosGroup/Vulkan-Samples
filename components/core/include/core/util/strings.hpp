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
}        // namespace vkb