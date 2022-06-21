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

#include "components/vfs/filesystem.hpp"

namespace components
{
namespace vfs
{
/**
 * @brief Windows File System Impl
 * 
 */
class WindowsFileSystem : public FileSystem
{
  public:
	WindowsFileSystem(const std::string &base_path = "");
	virtual ~WindowsFileSystem() = default;

	virtual bool          folder_exists(const std::string &file_path) override;
	virtual bool          file_exists(const std::string &file_path) override;
	virtual StackErrorPtr read_file(const std::string &file_path, std::shared_ptr<Blob> *blob) override;
	virtual StackErrorPtr read_chunk(const std::string &file_path, const size_t offset, const size_t count, std::shared_ptr<Blob> *blob) override;
	virtual size_t        file_size(const std::string &file_path) override;
	virtual StackErrorPtr write_file(const std::string &file_path, const void *data, size_t size) override;
	virtual StackErrorPtr enumerate_files(const std::string &file_path, std::vector<std::string> *files) override;
	virtual StackErrorPtr enumerate_folders(const std::string &file_path, std::vector<std::string> *folders) override;

  private:
	std::string m_base_path;
};

/**
 * @brief Windows File System Impl from the default Windows temp directory
 * 
 */
class WindowsTempFileSystem final : public WindowsFileSystem
{
  public:
	WindowsTempFileSystem();
	virtual ~WindowsTempFileSystem() = default;
};
}        // namespace vfs
}        // namespace components