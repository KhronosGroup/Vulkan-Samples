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

#include "std_filesystem.hpp"

#include <core/util/logging.hpp>

#include <filesystem>
#include <fstream>

namespace vkb
{
namespace filesystem
{
FileStat StdFileSystem::stat_file(const std::string &path)
{
	std::error_code ec;

	auto fs_stat = std::filesystem::status(path, ec);
	if (ec)
	{
		return FileStat{
		    false,
		    false,
		    0,
		};
	}

	auto size = std::filesystem::file_size(path, ec);
	if (ec)
	{
		size = 0;
	}

	return FileStat{
	    fs_stat.type() == std::filesystem::file_type::regular,
	    fs_stat.type() == std::filesystem::file_type::directory,
	    size,
	};
}

bool StdFileSystem::is_file(const std::string &path)
{
	auto stat = stat_file(path);
	return stat.is_file;
}

bool StdFileSystem::is_directory(const std::string &path)
{
	auto stat = stat_file(path);
	return stat.is_directory;
}

bool StdFileSystem::exists(const std::string &path)
{
	auto stat = stat_file(path);
	return stat.is_file || stat.is_directory;
}

bool StdFileSystem::create_directory(const std::string &path)
{
	std::error_code ec;

	std::filesystem::create_directory(path, ec);

	if (ec)
	{
		LOGE("Failed to create directory: {}", path);
	}

	return !ec;
}

std::vector<uint8_t> StdFileSystem::read_chunk(const std::string &path, size_t start, size_t offset)
{
	std::ifstream file{path, std::ios::binary | std::ios::ate};

	if (!file.is_open())
	{
		return {};
	}

	auto size = stat_file(path).size;

	if (start + offset > size)
	{
		return {};
	}

	// read file contents
	file.seekg(start, std::ios::beg);
	std::vector<uint8_t> data(offset);
	file.read(reinterpret_cast<char *>(data.data()), offset);

	return data;
}

void StdFileSystem::write_file(const std::string &path, const std::vector<uint8_t> &data)
{
	std::ofstream file{path, std::ios::binary | std::ios::trunc};

	if (!file.is_open())
	{
		return;
	}

	file.write(reinterpret_cast<const char *>(data.data()), data.size());
}

const std::string &StdFileSystem::external_storage_directory() const
{
	return _external_storage_directory;
}

const std::string &StdFileSystem::temp_directory() const
{
	return _temp_directory;
}

}        // namespace filesystem
}        // namespace vkb