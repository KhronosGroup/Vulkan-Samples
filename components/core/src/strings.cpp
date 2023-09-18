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

#include <core/util/strings.hpp>

#include <algorithm>
#include <cctype>
#include <sstream>

namespace vkb
{
std::string replace_all(std::string str, const std::string &from, const std::string &to)
{
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length() - 1;
	}
	return str;
}

std::string trim_right(const std::string &str, const std::string &chars)
{
	std::string result = str;
	result.erase(str.find_last_not_of(chars) + 1);
	return result;
}

std::string trim_left(const std::string &str, const std::string &chars)
{
	std::string result = str;
	result.erase(0, str.find_first_not_of(chars));
	return result;
}

std::vector<std::string> split(const std::string &str, const std::string &delim)
{
	std::vector<std::string> result;
	size_t                   start = 0;
	size_t                   end   = 0;

	while ((end = str.find(delim, start)) != std::string::npos)
	{
		result.push_back(str.substr(start, end - start));
		start = end + delim.length();
	}

	result.push_back(str.substr(start));

	return result;
}

std::string to_snake_case(const std::string &text)
{
	std::stringstream result;

	for (auto i = 0; i < text.size(); i++)
	{
		const auto ch = text[i];

		if (!std::isalpha(ch))
		{
			result << ch;
			continue;
		}

		if (std::isspace(ch))
		{
			result << "_";
			continue;
		}

		if (std::isupper(ch) && i > 0)
		{
			bool lower_before = std::islower(text[i - 1]);
			bool lower_after  = i < text.size() ? std::islower(text[i + 1]) : false;
			if (lower_before || lower_after)
			{
				result << "_";
			}
		}

		result << static_cast<char>(std::tolower(ch));
	}

	return result.str();
}

std::string to_upper_case(const std::string &text)
{
	std::string result = text;
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);
	return result;
}

bool ends_with(const std::string &str, const std::string &suffix, bool case_sensitive)
{
	if (str.length() < suffix.length())
	{
		return false;
	}

	if (case_sensitive)
	{
		return (0 == str.compare(str.length() - suffix.length(), suffix.length(), suffix));
	}

	std::string str_upper    = to_upper_case(str);
	std::string suffix_upper = to_upper_case(suffix);
	return (0 == str_upper.compare(str_upper.length() - suffix_upper.length(), suffix_upper.length(), suffix_upper));
}
}        // namespace vkb