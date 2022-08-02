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

#include "components/vfs/std_filesystem.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

#include "components/vfs/helpers.hpp"

namespace components
{
namespace vfs
{
inline std::filesystem::path get_full_path(const std::filesystem::path &base, const std::string &path)
{
	auto parts = helpers::tokenize_path(path);

	auto full_path = base;

	for (auto &part : parts)
	{
		full_path /= part;
	}

	full_path /= helpers::get_file_name(path);

	return full_path;
}

StdFSFileSystem::StdFSFileSystem(const std::filesystem::path &base_path) :
    FileSystem{},
    m_base_path{base_path}
{
}

StdFSTempFileSystem::StdFSTempFileSystem() :
    StdFSFileSystem{}
{
	std::error_code error;

	m_base_path = std::filesystem::temp_directory_path(error);

	if (error)
	{
		throw std::runtime_error{"failed to initialize temporary file directory path"};
	}
}

bool StdFSFileSystem::folder_exists(const std::string &folder_path) const
{
	return std::filesystem::is_directory(m_base_path / folder_path);
}

bool StdFSFileSystem::file_exists(const std::string &file_path) const
{
	return std::filesystem::is_regular_file(m_base_path / file_path);
}

std::vector<uint8_t> StdFSFileSystem::read_chunk(const std::string &file_path, size_t offset, size_t count) const
{
	std::filesystem::path full_path = get_full_path(m_base_path, file_path);

	if (!std::filesystem::exists(full_path))
	{
		throw std::runtime_error("vfs/std_filesystem.cpp line " + std::to_string(__LINE__) + ": file does not exist: " + full_path.string());
	}

	size_t size = file_size(file_path);

	if (offset + count > static_cast<size_t>(size))
	{
		throw std::runtime_error("vfs/std_filesystem.cpp line " + std::to_string(__LINE__) + "chunk out of file bounds: " + full_path.string());
	}

	std::fstream stream;
	stream.open(full_path, std::ios::in | std::ios::binary);

	if (!stream.good())
	{
		throw std::runtime_error("vfs/std_filesystem.cpp line " + std::to_string(__LINE__) + "failed to open file: " + full_path.string());
	}

	stream.seekg(offset, std::ios_base::beg);

	std::vector<uint8_t> blob(count, 0);

	if (!stream.read(reinterpret_cast<char *>(blob.data()), count))
	{
		throw std::runtime_error("vfs/std_filesystem.cpp line " + std::to_string(__LINE__) + "failed to read chunk from file: " + full_path.string());
	}

	return blob;
}

size_t StdFSFileSystem::file_size(const std::string &file_path) const
{
	std::filesystem::path full_path = get_full_path(m_base_path, file_path);

	if (!std::filesystem::exists(full_path))
	{
		return 0;
	}

	return static_cast<size_t>(std::filesystem::file_size(full_path));
}

void StdFSFileSystem::write_file(const std::string &file_path, const void *data, size_t size)
{
	if (!data)
	{
		return;
	}

	auto full_path = get_full_path(m_base_path, file_path);

	std::fstream stream;
	stream.open(full_path, std::ios::out | std::ios::binary);
	if (!stream.good())
	{
		throw std::runtime_error("vfs/std_filesystem.cpp line " + std::to_string(__LINE__) + "failed to open file: " + full_path.string());
	}

	stream.write(reinterpret_cast<const char *>(data), size);
	if (!stream.good())
	{
		throw std::runtime_error("vfs/std_filesystem.cpp line " + std::to_string(__LINE__) + "failed to write to file: " + full_path.string());
	}
}

std::vector<std::string> StdFSFileSystem::enumerate_files(const std::string &dir) const
{
	auto full_path = get_full_path(m_base_path, dir);

	if (!std::filesystem::exists(full_path))
	{
		throw std::runtime_error("vfs/std_filesystem.cpp line " + std::to_string(__LINE__) + "path does not exist: " + full_path.string());
	}

	if (!std::filesystem::is_directory(full_path))
	{
		throw std::runtime_error("vfs/std_filesystem.cpp line " + std::to_string(__LINE__) + "path is not a directory: " + full_path.string());
	}

	std::vector<std::string> files;

	auto base_path_size = m_base_path.string().size();

	for (const auto &entry : std::filesystem::directory_iterator(full_path))
	{
		if (std::filesystem::is_regular_file(entry.status()))
		{
			// TODO: switching between std::path and std::string is inefficient
			auto file = entry.path().string().substr(base_path_size);
			files.push_back(helpers::sanitize(file));
		}
	}

	return files;
}

std::vector<std::string> StdFSFileSystem::enumerate_folders(const std::string &dir) const
{
	auto full_path = get_full_path(m_base_path, dir);

	if (!std::filesystem::exists(full_path))
	{
		throw std::runtime_error("vfs/std_filesystem.cpp line " + std::to_string(__LINE__) + "path does not exist: " + full_path.string());
	}

	if (!std::filesystem::is_directory(full_path))
	{
		throw std::runtime_error("vfs/std_filesystem.cpp line " + std::to_string(__LINE__) + "path is not a directory: " + full_path.string());
	}

	std::vector<std::string> folders;

	for (const auto &entry : std::filesystem::directory_iterator(full_path))
	{
		if (std::filesystem::is_directory(entry.status()))
		{
			// TODO: switching between std::path and std::string is inefficient
			auto directory = entry.path().string();
			directory      = directory.substr(m_base_path.string().size());
			folders.push_back(helpers::sanitize(directory));
		}
	}
	return folders;
}

void StdFSFileSystem::make_directory(const std::string &path)
{
	if (!std::filesystem::exists(path))
	{
		std::filesystem::create_directory(path);
	}
}

bool StdFSFileSystem::remove(const std::string &path)
{
	std::error_code error;

	if (!std::filesystem::remove(get_full_path(m_base_path, path), error))
	{
		// file does not exist and so was not removed
		return true;
	}

	return !error;
}

}        // namespace vfs
}        // namespace components