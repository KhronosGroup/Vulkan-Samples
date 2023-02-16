/* Copyright (c) 2022, Arm Limited and Contributors
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
#include <string_view>
#include <vector>

namespace components
{
namespace strings
{
//	Replace all occurrences of a substring with another substring
inline std::string replace_all(std::string str, const std::string &from, const std::string &to)
{
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length() - 1;
	}
	return str;
}

// Trim all occurrences of a substring from the left
inline std::string trim_right(const std::string &str, const std::string &chars = "")
{
	std::string result = str;
	result.erase(str.find_last_not_of(chars) + 1);
	return result;
}

// Trim all occurrences of a substring from the right
inline std::string trim_left(const std::string &str, const std::string &chars = "")
{
	std::string result = str;
	result.erase(0, str.find_first_not_of(chars));
	return result;
}

// Trim all occurrences of a substring from both sides
inline std::string trim(const std::string &str, const std::string &chars = "")
{
	return trim_left(trim_right(str, chars), chars);
}

// Convert a vector of string_view to a vector of C strings
inline std::vector<const char *> to_cstr(const std::vector<std::string_view> &strs)
{
	std::vector<const char *> result;
	result.reserve(strs.size());
	for (const auto &str : strs)
	{
		result.push_back(str.data());
	}
	return result;
}
}        // namespace strings
}        // namespace components