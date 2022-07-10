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

#include "components/vfs/helpers.hpp"

#include <cassert>

#include <re2/re2.h>

#include <components/common/strings.hpp>

namespace components
{
namespace vfs
{
namespace helpers
{
std::string get_file_extension(const std::string &uri)
{
	auto dot_pos = uri.find_last_of('.');
	if (dot_pos == std::string::npos)
	{
		return "";
	}

	return uri.substr(dot_pos + 1);
}

std::string get_directory(const std::string &path)
{
	auto sanitized_path = sanitize(path);
	return sanitized_path.substr(0, sanitized_path.rfind("/"));
}

std::vector<std::string> get_directory_parts(const std::string &path)
{
	auto dir_path = get_directory(path);

	std::string accum;
	accum.reserve(dir_path.size());

	std::vector<std::string> dirs;

	size_t max_length = 0;
	for (char c : dir_path)
	{
		if (c == '/' && accum.size() > 0)
		{
			dirs.emplace_back(accum);
			max_length = accum.size();
		}

		accum += c;
	}

	if (max_length != accum.size())
	{
		dirs.emplace_back(accum);
	}

	return dirs;
}

std::vector<std::string> tokenize_path(const std::string &path)
{
	auto dir_path = get_directory(path);

	std::string accum;
	accum.reserve(dir_path.size());

	std::vector<std::string> dirs;

	for (char c : dir_path)
	{
		if (c == '/' && accum.size() > 0)
		{
			if (accum.size() > 0)
			{
				dirs.emplace_back(accum);
				accum.clear();
			}
		}

		if (c != '/')
		{
			accum += c;
		}
	}

	if (accum.size() > 0)
	{
		dirs.emplace_back(accum);
	}

	return dirs;
}

std::string get_file_name(const std::string &path)
{
	return path.substr(path.rfind("/") + 1, path.size());
}

std::string sanitize(const std::string &path)
{
	if (path.empty())
	{
		return "/";
	}

	std::string sanitized = path;

	// handle windows drive mounts
	std::string windows_prefix;
	RE2         re("^([a-zA-Z]+:).*");
	assert(re.ok());
	if (RE2::FullMatch(sanitized, re, &windows_prefix))
	{
		sanitized = sanitized.substr(windows_prefix.size());
	}

	// standardize file path
	sanitized = strings::replace_all(sanitized, "\\", "/");
	sanitized = strings::replace_all(sanitized, "//", "/");
	sanitized = strings::replace_all(sanitized, "/./", "/");

	// prefix with /
	if (sanitized[0] != '/' && windows_prefix.empty())
	{
		sanitized = "/" + sanitized;
	}

	// remove trailing /
	if (sanitized.size() > 1 && sanitized[sanitized.size() - 1] == '/')
	{
		sanitized = sanitized.substr(0, sanitized.size() - 1);
	}

	return windows_prefix + sanitized;
}

std::string join(const std::vector<std::string> &paths)
{
	std::string accum;

	for (auto &path : paths)
	{
		accum += sanitize(path);
	}

	return sanitize(accum);
}
}        // namespace helpers
}        // namespace vfs
}        // namespace components