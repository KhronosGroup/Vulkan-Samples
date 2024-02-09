/* Copyright (c) 2024, Thomas Atkinson
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
#include <memory>
#include <string>
#include <vector>

#include "core/platform/context.hpp"
#include "core/util/logging.hpp"

namespace vkb
{
namespace filesystem
{
struct FileStat
{
	bool   is_file;
	bool   is_directory;
	size_t size;
};

using Path = std::filesystem::path;

// A thin filesystem wrapper
class FileSystem
{
  public:
	FileSystem()          = default;
	virtual ~FileSystem() = default;

	virtual FileStat             stat_file(const Path &path)                                    = 0;
	virtual bool                 is_file(const Path &path)                                      = 0;
	virtual bool                 is_directory(const Path &path)                                 = 0;
	virtual bool                 exists(const Path &path)                                       = 0;
	virtual bool                 create_directory(const Path &path)                             = 0;
	virtual std::vector<uint8_t> read_chunk(const Path &path, size_t start, size_t offset)      = 0;
	virtual void                 write_file(const Path &path, const std::vector<uint8_t> &data) = 0;
	virtual void                 remove(const Path &path)                                       = 0;

	virtual const Path &external_storage_directory() const = 0;
	virtual const Path &temp_directory() const             = 0;

	void write_file(const Path &path, const std::string &data);

	// Read the entire file into a string
	std::string read_file_string(const Path &path);

	// Read the entire file into a vector of bytes
	std::vector<uint8_t> read_file_binary(const Path &path);
};

using FileSystemPtr = std::shared_ptr<FileSystem>;

void init();

// Initialize the filesystem with the given context
void init_with_context(const PlatformContext &context);

// Get the filesystem instance
FileSystemPtr get();

namespace helpers
{
std::string filename(const std::string &path);
}
}        // namespace filesystem
}        // namespace vkb