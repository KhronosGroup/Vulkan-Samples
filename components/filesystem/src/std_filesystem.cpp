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
FileStat StdFileSystem::stat_file(const Path &path)
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

bool StdFileSystem::is_file(const Path &path)
{
	auto stat = stat_file(path);
	return stat.is_file;
}

bool StdFileSystem::is_directory(const Path &path)
{
	auto stat = stat_file(path);
	return stat.is_directory;
}

bool StdFileSystem::exists(const Path &path)
{
	auto stat = stat_file(path);
	return stat.is_file || stat.is_directory;
}

bool StdFileSystem::create_directory(const Path &path)
{
	std::error_code ec;

	std::filesystem::create_directory(path, ec);

	if (ec)
	{
		throw std::runtime_error("Failed to create directory at path: " + path.string());
	}

	return !ec;
}

std::vector<uint8_t> StdFileSystem::read_chunk(const Path &path, size_t offset, size_t count)
{
	std::ifstream file{path, std::ios::binary | std::ios::ate};

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file for reading at path: " + path.string());
	}

	auto size = stat_file(path).size;

	if (offset + count > size)
	{
		return {};
	}

	// read file contents
	file.seekg(offset, std::ios::beg);
	std::vector<uint8_t> data(count);
	file.read(reinterpret_cast<char *>(data.data()), count);

	return data;
}

void StdFileSystem::write_file(const Path &path, const std::vector<uint8_t> &data)
{
	// create directory if it doesn't exist
	auto parent = path.parent_path();
	if (!std::filesystem::exists(parent))
	{
		create_directory(parent);
	}

	std::ofstream file{path, std::ios::binary | std::ios::trunc};

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file for writing at path: " + path.string());
	}

	file.write(reinterpret_cast<const char *>(data.data()), data.size());
}

void StdFileSystem::remove(const Path &path)
{
	std::error_code ec;

	std::filesystem::remove(path, ec);

	if (ec)
	{
		throw std::runtime_error("Failed to remove file at path:" + path.string());
	}
}

void StdFileSystem::set_external_storage_directory(const std::string &dir)
{
	_external_storage_directory = dir;
}

const Path &StdFileSystem::external_storage_directory() const
{
	return _external_storage_directory;
}

const Path &StdFileSystem::temp_directory() const
{
	return _temp_directory;
}

}        // namespace filesystem
}        // namespace vkb