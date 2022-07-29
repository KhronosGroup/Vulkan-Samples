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

#include <components/common/stack_error.hpp>

namespace components
{
namespace vfs
{
/**
 * @brief Binary Blob container
 */
class Blob
{
  public:
	Blob()          = default;
	virtual ~Blob() = default;

	/**
	 * @brief Query size of blob
	 *
	 * @return size_t size of blob in bytes
	 */
	virtual size_t size() const = 0;

	/**
	 * @brief Ptr to blob data
	 *
	 * @return const void* ptr to the start of the blob data
	 */
	virtual const void *data() const = 0;

	/**
	 * @brief Check if the blob contains data or not
	 */
	inline bool empty() const
	{
		return size() == 0 || data() == nullptr;
	}

	/**
	 * @brief Convert blob to std vector of uint8_t
	 *
	 * @return std::vector<uint8_t> std vector of data
	 */
	inline std::vector<uint8_t> binary() const
	{
		if (empty())
		{
			return {};
		}

		const uint8_t *bin_ptr = static_cast<const uint8_t *>(data());
		return {bin_ptr, bin_ptr + size()};
	}

	/**
	 * @brief Convert blob to ascii string
	 *
	 * @return std::string ascii string
	 */
	inline std::string ascii() const
	{
		auto bin = binary();
		return std::string{bin.begin(), bin.end()};
	}
};

/**
 * @brief Blob implementation using std::vector<uint8_t>
 *
 */
class StdBlob final : public Blob
{
  public:
	StdBlob()          = default;
	virtual ~StdBlob() = default;

	virtual size_t      size() const override;
	virtual const void *data() const override;

	std::vector<uint8_t> buffer;
};

/**
 * @brief FileSystem interface
 *
 */
class FileSystem
{
  public:
	FileSystem()          = default;
	virtual ~FileSystem() = default;

	void make_directory_recursive(const std::string &path);

	virtual bool          folder_exists(const std::string &file_path)                                                                    = 0;
	virtual bool          file_exists(const std::string &file_path)                                                                      = 0;
	virtual StackErrorPtr read_chunk(const std::string &file_path, const size_t offset, const size_t count, std::shared_ptr<Blob> *blob) = 0;
	virtual size_t        file_size(const std::string &file_path)                                                                        = 0;
	virtual StackErrorPtr write_file(const std::string &file_path, const void *data, size_t size)                                        = 0;
	virtual void          make_directory(const std::string &path)                                                                        = 0;
	virtual bool          remove(const std::string &path)                                                                                = 0;

	virtual StackErrorPtr read_file(const std::string &file_path, std::shared_ptr<Blob> *blob);

	virtual StackErrorPtr enumerate_files(const std::string &file_path, std::vector<std::string> *files)     = 0;
	virtual StackErrorPtr enumerate_folders(const std::string &file_path, std::vector<std::string> *folders) = 0;
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

	virtual bool          folder_exists(const std::string &file_path) override;
	virtual bool          file_exists(const std::string &file_path) override;
	virtual StackErrorPtr read_chunk(const std::string &file_path, const size_t offset, const size_t count, std::shared_ptr<Blob> *blob) override;
	virtual size_t        file_size(const std::string &file_path) override;
	virtual StackErrorPtr write_file(const std::string &file_path, const void *data, size_t size) override;
	virtual StackErrorPtr enumerate_files(const std::string &file_path, std::vector<std::string> *files) override;
	virtual StackErrorPtr enumerate_folders(const std::string &file_path, std::vector<std::string> *folders) override;
	virtual void          make_directory(const std::string &path) override;
	virtual bool          remove(const std::string &path) override;

	virtual StackErrorPtr read_file(const std::string &file_path, std::shared_ptr<Blob> *blob) override;

	StackErrorPtr enumerate_files(const std::string &file_path, const std::string &extension, std::vector<std::string> *files);
	StackErrorPtr enumerate_files_recursive(const std::string &file_path, const std::string &extension, std::vector<std::string> *files);
	StackErrorPtr enumerate_folders_recursive(const std::string &folder_path, std::vector<std::string> *folders);

	void mount(const std::string &file_path, std::shared_ptr<FileSystem> file_system);
	void unmount(const std::string &file_path);

  private:
	std::shared_ptr<FileSystem> find_file_system(const std::string &file_path, std::string *mount_adjusted_path);

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
extern RootFileSystem &_default(void *context = nullptr);
}        // namespace vfs
}        // namespace components