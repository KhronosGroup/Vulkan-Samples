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

#include "unix/context.hpp"

namespace vkb
{

UnixPlatformContext::UnixPlatformContext(int argc, char **argv)
{
	_arguments.reserve(argc);
	for (int i = 1; i < argc; ++i)
	{
		_arguments.emplace_back(argv[i]);
	}

	const char *env_temp_dir = std::getenv("TMPDIR");
	_temp_directory          = env_temp_dir ? std::string(env_temp_dir) + "/" : "/tmp/";
}

std::vector<std::string_view> UnixPlatformContext::arguments() const
{
	return _arguments;
}

std::string_view UnixPlatformContext::external_storage_directory() const
{
	return _external_storage_directory;
}

std::string_view UnixPlatformContext::temp_directory() const
{
	return _temp_directory;
}
}        // namespace vkb