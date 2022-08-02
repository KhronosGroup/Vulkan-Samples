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

#include <filesystem>

#include "components/vfs/filesystem.hpp"

namespace components
{
namespace vfs
{
/**
 * @brief Windows File System Impl
 *
 */
class StdFSFileSystem : public FileSystem
{
  public:
	StdFSFileSystem(const std::filesystem::path &base_path = "");
	virtual ~StdFSFileSystem() = default;

	virtual bool                     folder_exists(const std::string &folder_path) const override;
	virtual bool                     file_exists(const std::string &file_path) const override;
	virtual std::vector<uint8_t>     read_chunk(const std::string &file_path, size_t offset, size_t count) const override;
	virtual size_t                   file_size(const std::string &file_path) const override;
	virtual void                     write_file(const std::string &file_path, const void *data, size_t size) override;
	virtual std::vector<std::string> enumerate_files(const std::string &folder_path) const override;
	virtual std::vector<std::string> enumerate_folders(const std::string &folderPath) const override;
	virtual void                     make_directory(const std::string &path) override;
	virtual bool                     remove(const std::string &path) override;

  protected:
	std::filesystem::path m_base_path;
};

/**
 * @brief Windows File System Impl from the default Windows temp directory
 *
 */
class StdFSTempFileSystem final : public StdFSFileSystem
{
  public:
	StdFSTempFileSystem();
	virtual ~StdFSTempFileSystem() = default;
};
}        // namespace vfs
}        // namespace components