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

#include "components/vfs/filesystem.hpp"

#include <algorithm>
#include <queue>
#include <set>

#include "components/vfs/helpers.hpp"

namespace components
{
namespace vfs
{
size_t StdBlob::size() const
{
	return buffer.size();
}

const void *StdBlob::data() const
{
	if (buffer.empty())
	{
		return nullptr;
	}

	return static_cast<const void *>(&buffer.at(0));
}

void FileSystem::make_directory_recursive(const std::string &path)
{
	auto parts = helpers::get_directory_parts(path);

	for (auto &path : parts)
	{
		if (!folder_exists(path))
		{
			make_directory(path);
		}
	}
}

RootFileSystem::RootFileSystem(const std::string &base_path) :
    FileSystem{}, m_root_path{base_path}
{
}

bool RootFileSystem::folder_exists(const std::string &file_path)
{
	std::string adjusted_path;
	auto        fs = find_file_system(file_path, &adjusted_path);
	if (!fs)
	{
		return false;
	}

	return fs->folder_exists(adjusted_path);
}

bool RootFileSystem::file_exists(const std::string &file_path)
{
	std::string adjusted_path;
	auto        fs = find_file_system(file_path, &adjusted_path);
	if (!fs)
	{
		return false;
	}
	return fs->file_exists(adjusted_path);
}

StackErrorPtr RootFileSystem::read_file(const std::string &file_path, std::shared_ptr<Blob> *blob)
{
	std::string adjusted_path;
	auto        fs = find_file_system(file_path, &adjusted_path);
	if (!fs)
	{
		return StackError::unique("failed to select appropriate file system for path: " + file_path, "vfs/root_file_system.cpp", __LINE__);
	}

	return fs->read_file(adjusted_path, blob);
}

StackErrorPtr RootFileSystem::read_chunk(const std::string &file_path, const size_t offset, const size_t count, std::shared_ptr<Blob> *blob)
{
	std::string adjusted_path;
	auto        fs = find_file_system(file_path, &adjusted_path);
	if (!fs)
	{
		return StackError::unique("failed to select appropriate file system for path: " + file_path, "vfs/root_file_system.cpp", __LINE__);
	}

	return fs->read_chunk(adjusted_path, offset, count, blob);
}

size_t RootFileSystem::file_size(const std::string &file_path)
{
	std::string adjusted_path;
	auto        fs = find_file_system(file_path, &adjusted_path);
	if (!fs)
	{
		return 0;
	}

	return fs->file_size(adjusted_path);
}

StackErrorPtr RootFileSystem::write_file(const std::string &file_path, const void *data, size_t size)
{
	std::string adjusted_path;
	auto        fs = find_file_system(file_path, &adjusted_path);
	if (!fs)
	{
		return StackError::unique("failed to select appropriate file system for path: " + file_path, "vfs/root_file_system.cpp", __LINE__);
	}

	fs->make_directory_recursive(adjusted_path);

	return fs->write_file(adjusted_path, data, size);
}

StackErrorPtr RootFileSystem::enumerate_files(const std::string &file_path, std::vector<std::string> *files)
{
	std::string adjusted_path;
	auto        fs = find_file_system(file_path, &adjusted_path);
	if (!fs)
	{
		return StackError::unique("failed to select appropriate file system for path: " + file_path, "vfs/root_file_system.cpp", __LINE__);
	}

	if (!files)
	{
		return nullptr;
	}

	return fs->enumerate_files(adjusted_path, files);
}

StackErrorPtr RootFileSystem::enumerate_folders(const std::string &file_path, std::vector<std::string> *folders)
{
	std::string adjusted_path;
	auto        fs = find_file_system(file_path, &adjusted_path);
	if (!fs)
	{
		return StackError::unique("failed to select appropriate file system for path: " + file_path, "vfs/root_file_system.cpp", __LINE__);
	}

	if (!folders)
	{
		return nullptr;
	}

	return fs->enumerate_folders(adjusted_path, folders);
}

StackErrorPtr RootFileSystem::enumerate_files(const std::string &file_path, const std::string &extension, std::vector<std::string> *files)
{
	std::vector<std::string> all_files;

	auto res = enumerate_files(file_path, &all_files);
	if (res != nullptr)
	{
		return res;
	}

	std::vector<std::string> files_with_extension;
	files_with_extension.reserve(all_files.size());

	if (extension.empty())
	{
		*files = all_files;
		return nullptr;
	}

	for (auto file : all_files)
	{
		if (file.size() >= extension.size())
		{
			if (file.substr(file.size() - extension.size(), extension.size()) == extension)
			{
				// match
				files_with_extension.push_back(file);
			}
		}
	}

	*files = files_with_extension;

	return nullptr;
}

StackErrorPtr RootFileSystem::enumerate_files_recursive(const std::string &file_path, const std::string &extension, std::vector<std::string> *files)
{
	std::vector<std::string> folders;
	auto                     res = enumerate_folders_recursive(file_path, &folders);
	if (res != nullptr)
	{
		return res;
	}

	std::vector<std::string> all_files;

	for (auto &folder : folders)
	{
		std::vector<std::string> folder_files;
		auto                     res = enumerate_files(folder, extension, &folder_files);
		if (res != nullptr)
		{
			return res;
		}

		all_files.reserve(folder_files.size());
		for (auto &file : folder_files)
		{
			all_files.push_back(folder + file);
		}
	}

	*files = all_files;

	return nullptr;
}

StackErrorPtr RootFileSystem::enumerate_folders_recursive(const std::string &file_path, std::vector<std::string> *folders)
{
	std::set<std::string> all_dirs;
	all_dirs.emplace(file_path);

	std::queue<std::string> dirs_to_visit;
	dirs_to_visit.emplace(file_path);

	while (!dirs_to_visit.empty())
	{
		std::string front = dirs_to_visit.front();
		dirs_to_visit.pop();

		std::vector<std::string> dirs;

		auto res = enumerate_folders(front, &dirs);
		if (res != nullptr)
		{
			return res;
		}

		for (std::string &dir : dirs)
		{
			std::string full_path = front + dir;
			auto        it        = all_dirs.find(full_path);
			if (it == all_dirs.end())
			{
				all_dirs.emplace(full_path);
				dirs_to_visit.emplace(full_path);
			}
		}
	}

	*folders = std::vector<std::string>{all_dirs.begin(), all_dirs.end()};

	return nullptr;
}

void RootFileSystem::make_directory(const std::string &file_path)
{
	std::string adjusted_path;
	auto        fs = find_file_system(file_path, &adjusted_path);
	if (!fs)
	{
		return;
	}

	fs->make_directory(file_path);
}

void RootFileSystem::mount(const std::string &file_path, std::shared_ptr<FileSystem> file_system)
{
	for (auto &mount : m_mounts)
	{
		if (mount.first == file_path)
		{
			mount.second = file_system;
			return;
		}
	}

	m_mounts.push_back({file_path, file_system});
}

void RootFileSystem::unmount(const std::string &file_path)
{
	auto it = m_mounts.begin();
	while (it != m_mounts.end())
	{
		if (it->first == file_path)
		{
			m_mounts.erase(it);
			return;
		}
		it++;
	}
}

std::shared_ptr<FileSystem> RootFileSystem::find_file_system(const std::string &file_path, std::string *trimmed_path)
{
	if (file_path.empty())
	{
		return nullptr;
	}

	size_t                                               best_score = 0;
	std::pair<std::string, std::shared_ptr<FileSystem>> *pair{nullptr};

	for (auto &mount : m_mounts)
	{
		if (file_path.substr(0, mount.first.size()) == mount.first && mount.first.size() > best_score)
		{
			best_score = mount.first.size();
			pair       = &mount;
		}
	}

	if (!pair)
	{
		return nullptr;
	}

	auto trimmed = file_path.substr(best_score, file_path.size() - best_score);

	if (trimmed[0] != '/')
	{
		trimmed = "/" + trimmed;
	}

	*trimmed_path = trimmed;

	return pair->second;
}
}        // namespace vfs
}        // namespace components