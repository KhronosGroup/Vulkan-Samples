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

#include <android_native_app_glue.h>

#include "components/vfs/filesystem.hpp"
#include "components/vfs/std_filesystem.hpp"

namespace components
{
namespace vfs
{
/**
 * @brief Detect the Android Temporary File dir through JNI
 * @param sub_path a subpath into the temporary file dir
 */
class AndroidTempFileSystem final : public StdFSFileSystem
{
  public:
	AndroidTempFileSystem(android_app *app, const std::string &sub_path = "");
	virtual ~AndroidTempFileSystem() = default;
};

/**
 * @brief Detect the Android External File dir through JNI
 * @param sub_path a subpath into the external file dir
 */
class AndroidExternalFileSystem final : public StdFSFileSystem
{
  public:
	AndroidExternalFileSystem(android_app *app, const std::string &sub_path = "");
	virtual ~AndroidExternalFileSystem() = default;
};

/**
 * @brief Load files from the android AAssetManager
 * @param sub_path a subpath into the AAsset archive
 */
class AndroidAAssetManager : public FileSystem
{
  public:
	AndroidAAssetManager(android_app *app, const std::string &sub_path = "");
	virtual ~AndroidAAssetManager() = default;

	virtual bool          folder_exists(const std::string &file_path) override;
	virtual bool          file_exists(const std::string &file_path) override;
	virtual StackErrorPtr read_chunk(const std::string &file_path, const size_t offset, const size_t count, std::shared_ptr<Blob> *blob) override;
	virtual size_t        file_size(const std::string &file_path) override;
	virtual StackErrorPtr write_file(const std::string &file_path, const void *data, size_t size) override;
	virtual StackErrorPtr enumerate_files(const std::string &file_path, std::vector<std::string> *files) override;
	virtual StackErrorPtr enumerate_folders(const std::string &file_path, std::vector<std::string> *folders) override;
	virtual void          make_directory(const std::string &path) override;
	virtual bool          remove(const std::string &path) override;

  private:
	std::string get_path(const std::string &path);

	std::string    m_base_path;
	AAssetManager *asset_manager;
};
}        // namespace vfs
}        // namespace components