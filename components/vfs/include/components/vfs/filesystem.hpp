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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <components/platform/platform.hpp>

namespace components
{
namespace vfs
{
/**
 * @brief FileSystem interface
 *
 */
class FileSystem
{
  public:
	FileSystem()          = default;
	virtual ~FileSystem() = default;

	void                 make_directory_recursive(const std::string &path);
	std::vector<uint8_t> read_file(const std::string &file_path) const;

	virtual bool                 folder_exists(const std::string &folder_path) const                         = 0;
	virtual bool                 file_exists(const std::string &file_path) const                             = 0;
	virtual std::vector<uint8_t> read_chunk(const std::string &file_path, size_t offset, size_t count) const = 0;
	virtual size_t               file_size(const std::string &file_path) const                               = 0;
	virtual void                 write_file(const std::string &file_path, const void *data, size_t size)     = 0;
	virtual void                 make_directory(const std::string &path)                                     = 0;
	virtual bool                 remove(const std::string &path)                                             = 0;

	virtual std::vector<std::string> enumerate_files(const std::string &folder_path) const  = 0;
	virtual std::vector<std::string> enumerate_folders(const std::string &folderPath) const = 0;

	std::vector<std::string> enumerate_files(const std::string &folder_path, const std::string &extension) const;
	std::vector<std::string> enumerate_files_recursive(const std::string &folder_path, const std::string &extension) const;
	std::vector<std::string> enumerate_folders_recursive(const std::string &folder_path) const;
};

/**
 * @brief Root File System
 *
 * 		  A file system consists of a RootFileSystem with a combination of mounted virtual file systems.
 *        Different OS and runtime configurations can determine how the RootFS is initialised.
 */
class RootFileSystem : public FileSystem
{
  public:
	RootFileSystem(const std::string &base_path = "");
	virtual ~RootFileSystem() = default;

	virtual bool                     folder_exists(const std::string &folder_path) const override;
	virtual bool                     file_exists(const std::string &file_path) const override;
	virtual std::vector<uint8_t>     read_chunk(const std::string &file_path, size_t offset, size_t count) const override;
	virtual size_t                   file_size(const std::string &file_path) const override;
	virtual void                     write_file(const std::string &file_path, const void *data, size_t size) override;
	virtual std::vector<std::string> enumerate_files(const std::string &folder_path) const override;
	virtual std::vector<std::string> enumerate_folders(const std::string &folderPath) const override;
	virtual void                     make_directory(const std::string &path) override;
	virtual bool                     remove(const std::string &path) override;

	void mount(const std::string &file_path, std::shared_ptr<FileSystem> file_system);
	void unmount(const std::string &file_path);

  private:
	std::shared_ptr<FileSystem> find_file_system(const std::string &file_path, std::string *mount_adjusted_path) const;

	std::string                                                      m_root_path;
	std::vector<std::pair<std::string, std::shared_ptr<FileSystem>>> m_mounts;
};

/**
 * @brief Create a default RootFileSystem singleton taking into account the OS or compile time flags
 *
 *        If an OS relies on a context then instance should be called with a context only once and before the filesystem is used for any useful task.
 *
 * @param context An OS specific context ptr
 * @return RootFileSystem& A root file system reference
 */
extern RootFileSystem &_default(const PlatformContext *context = nullptr);
}        // namespace vfs
}        // namespace components