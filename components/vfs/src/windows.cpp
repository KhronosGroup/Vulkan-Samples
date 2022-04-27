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

#include "components/vfs/windows.hpp"

#define NOMINMAX
#include <Windows.h>
#include <direct.h>

#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

namespace vfs
{
RootFileSystem &instance(void *context)
{
	static vfs::RootFileSystem fs;

	static bool first_time = true;
	if (first_time)
	{
		first_time = false;

		char *buffer = _getcwd(NULL, 0);

		if (!buffer)
		{
			return fs;
		}

		std::string cwd{buffer};

		fs.mount("/", std::make_shared<vfs::WindowsFileSystem>(cwd));
		fs.mount("/scenes", std::make_shared<vfs::WindowsFileSystem>(cwd + "/assets/scenes"));
		fs.mount("/textures", std::make_shared<vfs::WindowsFileSystem>(cwd + "/assets/textures"));
		fs.mount("/fonts", std::make_shared<vfs::WindowsFileSystem>(cwd + "/assets/fonts"));
		fs.mount("/temp", std::make_shared<vfs::WindowsTempFileSystem>());

		free(buffer);
	}

	return fs;
}
inline const std::string get_temp_path_from_environment()
{
	TCHAR temp_buffer[MAX_PATH];
	DWORD temp_path_ret = GetTempPath(MAX_PATH, temp_buffer);
	if (temp_path_ret > MAX_PATH || temp_path_ret == 0)
	{
		return "temp";
	}

	std::stringstream os;
	os << temp_buffer;
	return os.str();
}

WindowsFileSystem::WindowsFileSystem(const std::string &base_path) :
    FileSystem{},
    m_base_path{base_path}
{
}

WindowsTempFileSystem::WindowsTempFileSystem() :
    WindowsFileSystem{get_temp_path_from_environment()}
{}

bool WindowsFileSystem::folder_exists(const std::string &file_path)
{
	std::string full_path = m_base_path + file_path;

	struct _stat info;
	if (_stat(full_path.c_str(), &info) != 0)
	{
		return false;
	}
	return (info.st_mode & S_IFDIR) == S_IFDIR;
}

bool WindowsFileSystem::file_exists(const std::string &file_path)
{
	std::string full_path = m_base_path + file_path;

	struct _stat info;
	if (_stat(full_path.c_str(), &info) != 0)
	{
		return false;
	}
	return (info.st_mode & S_IFREG) == S_IFREG;
}

status::status WindowsFileSystem::read_file(const std::string &file_path, std::shared_ptr<Blob> *blob)
{
	std::string full_path = m_base_path + file_path;

	std::fstream stream;
	stream.open(full_path, std::ios::in | std::ios::binary);

	if (!stream.good())
	{
		return status::FailedToRead;
	}

	if (!blob)
	{
		return status::Success;
	}

	stream.ignore(std::numeric_limits<std::streamsize>::max());
	std::streamsize size = stream.gcount();
	stream.clear();
	stream.seekg(0, std::ios_base::beg);

	auto w_blob = std::make_shared<StdBlob>();
	w_blob->buffer.resize(static_cast<size_t>(size), 0);

	if (!stream.read(reinterpret_cast<char *>(w_blob->buffer.data()), size))
	{
		return status::FailedToRead;
	}

	*blob = w_blob;

	return status::Success;
}

status::status WindowsFileSystem::read_chunk(const std::string &file_path, const size_t offset, const size_t count, std::shared_ptr<Blob> *blob)
{
	std::string full_path = m_base_path + file_path;

	std::fstream stream;
	stream.open(full_path, std::ios::in | std::ios::binary);

	if (!stream.good())
	{
		return status::FailedToRead;
	}

	if (!blob)
	{
		return status::Success;
	}

	stream.ignore(std::numeric_limits<std::streamsize>::max());
	std::streamsize size = stream.gcount();
	stream.clear();

	if (offset + count > size)
	{
		return status::MemoryOutOfRange;
	}

	stream.seekg(offset, std::ios_base::beg);

	auto w_blob = std::make_shared<StdBlob>();
	w_blob->buffer.resize(static_cast<size_t>(count), 0);

	if (!stream.read(reinterpret_cast<char *>(w_blob->buffer.data()), count))
	{
		return status::FailedToRead;
	}

	*blob = w_blob;

	return status::Success;
}

size_t WindowsFileSystem::file_size(const std::string &file_path)
{
	std::string full_path = m_base_path + file_path;

	std::fstream stream;
	stream.open(full_path, std::ios::in | std::ios::binary);
	if (!stream.good())
	{
		return 0;
	}

	stream.ignore(std::numeric_limits<std::streamsize>::max());
	return stream.gcount();
}

bool WindowsFileSystem::write_file(const std::string &file_path, const void *data, size_t size)
{
	std::string full_path = m_base_path + file_path;

	std::fstream stream;
	stream.open(full_path, std::ios::out | std::ios::binary);
	if (!stream.good() || !stream.write(reinterpret_cast<const char *>(data), size))
	{
		return false;
	}
	return true;
}

status::status WindowsFileSystem::enumerate_files(const std::string &file_path, std::vector<std::string> *files)
{
	return status::NotImplemented;
}

status::status WindowsFileSystem::enumerate_folders(const std::string &file_path, std::vector<std::string> *folders)
{
	return status::NotImplemented;
}

}        // namespace vfs