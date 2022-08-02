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
#include <cassert>
#include <deque>
#include <set>

#include "components/vfs/helpers.hpp"

namespace components
{
namespace vfs
{
std::vector<uint8_t> FileSystem::read_file(const std::string &file_path) const
{
	return read_chunk(file_path, 0, file_size(file_path));
}

void FileSystem::make_directory_recursive(const std::string &path)
{
	auto parts = helpers::get_directory_parts(path);

	for (auto &sub_path : parts)
	{
		if (!folder_exists(sub_path))
		{
			make_directory(sub_path);
		}
	}
}

std::vector<std::string> FileSystem::enumerate_files(const std::string &folder_path, const std::string &extension) const
{
	std::vector<std::string> all_files = enumerate_files(folder_path);

	if (extension.empty())
	{
		return all_files;
	}

	std::vector<std::string> files_with_extension;
	files_with_extension.reserve(all_files.size());

	for (auto const &file : all_files)
	{
		if (helpers::get_file_extension(file) == extension)
		{
			files_with_extension.push_back(file);
		}
	}

	return files_with_extension;
}

std::vector<std::string> FileSystem::enumerate_files_recursive(const std::string &folder_path, const std::string &extension) const
{
	std::vector<std::string> folders = enumerate_folders_recursive(folder_path);

	std::vector<std::string> all_files;

	for (auto const &folder : folders)
	{
		std::vector<std::string> folder_files = enumerate_files(folder, extension);

		all_files.reserve(all_files.size() + folder_files.size());
		all_files.insert(all_files.end(), folder_files.begin(), folder_files.end());
	}
	return all_files;
}

std::vector<std::string> FileSystem::enumerate_folders_recursive(const std::string &folder_path) const
{
	std::vector<std::string> all_folders;

	std::deque<std::string> folders_to_visit;
	folders_to_visit.push_back(folder_path);

	while (!folders_to_visit.empty())
	{
		std::string folder = folders_to_visit.front();
		folders_to_visit.pop_front();

		std::vector<std::string> folders = enumerate_folders(folder);

		folders_to_visit.insert(folders_to_visit.end(), folders.begin(), folders.end());
		all_folders.insert(all_folders.end(), folders.begin(), folders.end());
	}

	return all_folders;
}

RootFileSystem::RootFileSystem(const std::string &base_path) :
    FileSystem{}, m_root_path{base_path}
{
}

bool RootFileSystem::folder_exists(const std::string &folder_path) const
{
	std::string adjusted_path;
	auto        fs = find_file_system(folder_path, &adjusted_path);
	if (!fs)
	{
		return false;
	}

	return fs->folder_exists(adjusted_path);
}

bool RootFileSystem::file_exists(const std::string &file_path) const
{
	std::string adjusted_path;
	auto        fs = find_file_system(file_path, &adjusted_path);
	if (!fs)
	{
		return false;
	}
	return fs->file_exists(adjusted_path);
}

std::vector<uint8_t> RootFileSystem::read_chunk(const std::string &file_path, size_t offset, size_t count) const
{
	std::string adjusted_path;
	auto        fs = find_file_system(file_path, &adjusted_path);
	if (!fs)
	{
		throw std::runtime_error("vfs/root_file_system.cpp line" + std::to_string(__LINE__) + ": failed to select appropriate file system for path: " + file_path);
	}

	return fs->read_chunk(adjusted_path, offset, count);
}

size_t RootFileSystem::file_size(const std::string &file_path) const
{
	std::string adjusted_path;
	auto        fs = find_file_system(file_path, &adjusted_path);
	if (!fs)
	{
		return 0;
	}

	return fs->file_size(adjusted_path);
}

void RootFileSystem::write_file(const std::string &file_path, const void *data, size_t size)
{
	assert(data);

	std::string adjusted_path;
	auto        fs = find_file_system(file_path, &adjusted_path);
	if (!fs)
	{
		throw std::runtime_error("vfs/root_file_system.cpp line" + std::to_string(__LINE__) + ": failed to select appropriate file system for path: " + file_path);
	}

	fs->make_directory_recursive(adjusted_path);

	return fs->write_file(adjusted_path, data, size);
}

std::vector<std::string> RootFileSystem::enumerate_files(const std::string &folder_path) const
{
	std::string adjusted_path;
	auto        fs = find_file_system(folder_path, &adjusted_path);
	if (!fs)
	{
		throw std::runtime_error("vfs/root_file_system.cpp line" + std::to_string(__LINE__) + ": failed to select appropriate file system for path: " + folder_path);
	}

	return fs->enumerate_files(adjusted_path);
}

std::vector<std::string> RootFileSystem::enumerate_folders(const std::string &folder_path) const
{
	std::string adjusted_path;
	auto        fs = find_file_system(folder_path, &adjusted_path);
	if (!fs)
	{
		throw std::runtime_error("vfs/root_file_system.cpp line" + std::to_string(__LINE__) + ": failed to select appropriate file system for path: " + folder_path);
	}

	return fs->enumerate_folders(adjusted_path);
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

bool RootFileSystem::remove(const std::string &path)
{
	std::string adjusted_path;
	auto        fs = find_file_system(path, &adjusted_path);
	if (!fs)
	{
		return false;
	}

	return fs->remove(adjusted_path);
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

std::shared_ptr<FileSystem> RootFileSystem::find_file_system(const std::string &file_path, std::string *trimmed_path) const
{
	if (file_path.empty())
	{
		return nullptr;
	}

	size_t                                                     best_score = 0;
	std::pair<std::string, std::shared_ptr<FileSystem>> const *pair{nullptr};

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