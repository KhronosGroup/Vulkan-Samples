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

bool StdFSFileSystem::folder_exists(const std::string &file_path)
{
	return std::filesystem::is_directory(m_base_path += file_path);
}

bool StdFSFileSystem::file_exists(const std::string &file_path)
{
	return std::filesystem::is_regular_file(m_base_path += file_path);
}

StackErrorPtr StdFSFileSystem::read_chunk(const std::string &file_path, const size_t offset, const size_t count, std::shared_ptr<Blob> *blob)
{
	if (!blob)
	{
		return nullptr;
	}

	std::filesystem::path full_path = get_full_path(m_base_path, file_path);

	if (!std::filesystem::exists(full_path))
	{
		return StackError::unique("file does not exist: " + full_path.string(), "vfs/std_filesystem.cpp", __LINE__);
	}

	size_t size = file_size(file_path);

	if (offset + count > static_cast<size_t>(size))
	{
		return StackError::unique("chunk out of file bounds: " + full_path.string(), "vfs/std_filesystem.cpp", __LINE__);
	}

	std::fstream stream;
	stream.open(full_path, std::ios::in | std::ios::binary);

	if (!stream.good())
	{
		return StackError::unique("failed to open file: " + full_path.string(), "vfs/std_filesystem.cpp", __LINE__);
	}

	stream.seekg(offset, std::ios_base::beg);

	auto w_blob = std::make_shared<StdBlob>();
	w_blob->buffer.resize(static_cast<size_t>(count), 0);

	if (!stream.read(reinterpret_cast<char *>(w_blob->buffer.data()), count))
	{
		return StackError::unique("failed to read chunk from file: " + full_path.string(), "vfs/std_filesystem.cpp", __LINE__);
	}

	*blob = std::move(w_blob);

	return nullptr;
}

size_t StdFSFileSystem::file_size(const std::string &file_path)
{
	std::filesystem::path full_path = get_full_path(m_base_path, file_path);

	if (!std::filesystem::exists(full_path))
	{
		return 0;
	}

	return static_cast<size_t>(std::filesystem::file_size(full_path));
}

StackErrorPtr StdFSFileSystem::write_file(const std::string &file_path, const void *data, size_t size)
{
	if (!data)
	{
		return nullptr;
	}

	auto full_path = get_full_path(m_base_path, file_path);

	std::fstream stream;
	stream.open(full_path, std::ios::out | std::ios::binary);
	if (!stream.good())
	{
		return StackError::unique("failed to open file: " + full_path.string(), "vfs/std_filesystem.cpp", __LINE__);
	}

	if (!stream.write(reinterpret_cast<const char *>(data), size))
	{
		return StackError::unique("failed to write to file: " + full_path.string(), "vfs/std_filesystem.cpp", __LINE__);
	}
	return nullptr;
}

StackErrorPtr StdFSFileSystem::enumerate_files(const std::string &dir, std::vector<std::string> *files)
{
	if (!files)
	{
		return nullptr;
	}

	auto full_path = get_full_path(m_base_path, dir);

	if (!std::filesystem::exists(full_path))
	{
		return StackError::unique("path does not exist: " + full_path.string(), "vfs/std_filesystem.cpp", __LINE__);
	}

	if (!std::filesystem::is_directory(full_path))
	{
		return StackError::unique("path is not a directory: " + full_path.string(), "vfs/std_filesystem.cpp", __LINE__);
	}

	std::vector<std::string> temp_files{};

	auto base_path_size = m_base_path.string().size();

	for (const auto &entry : std::filesystem::directory_iterator(full_path))
	{
		if (std::filesystem::is_regular_file(entry.status()))
		{
			// TODO: switching between std::path and std::string is inefficient
			auto file = entry.path().string().substr(base_path_size);
			temp_files.push_back(helpers::sanitize(file));
		}
	}

	*files = std::move(temp_files);

	return nullptr;
}

StackErrorPtr StdFSFileSystem::enumerate_folders(const std::string &dir, std::vector<std::string> *folders)
{
	if (!folders)
	{
		return nullptr;
	}

	auto full_path = get_full_path(m_base_path, dir);

	if (!std::filesystem::exists(full_path))
	{
		return StackError::unique("path does not exist: " + full_path.string(), "vfs/std_filesystem.cpp", __LINE__);
	}

	if (!std::filesystem::is_directory(full_path))
	{
		return StackError::unique("path is not a directory: " + full_path.string(), "vfs/std_filesystem.cpp", __LINE__);
	}

	std::vector<std::string> temp_folders{};

	for (const auto &entry : std::filesystem::directory_iterator(full_path))
	{
		if (std::filesystem::is_directory(entry.status()))
		{
			// TODO: switching between std::path and std::string is inefficient
			auto directory = entry.path().string();
			directory      = directory.substr(m_base_path.string().size());
			temp_folders.push_back(helpers::sanitize(directory));
		}
	}

	*folders = std::move(temp_folders);

	return nullptr;
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