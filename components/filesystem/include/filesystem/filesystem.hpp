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

// A thin filesystem wrapper
class FileSystem
{
  public:
	FileSystem()          = default;
	virtual ~FileSystem() = default;

	virtual FileStat             stat_file(const std::string &path)                                    = 0;
	virtual bool                 is_file(const std::string &path)                                      = 0;
	virtual bool                 is_directory(const std::string &path)                                 = 0;
	virtual bool                 exists(const std::string &path)                                       = 0;
	virtual bool                 create_directory(const std::string &path)                             = 0;
	virtual std::vector<uint8_t> read_chunk(const std::string &path, size_t start, size_t offset)      = 0;
	virtual void                 write_file(const std::string &path, const std::vector<uint8_t> &data) = 0;

	virtual const std::string &external_storage_directory() const = 0;
	virtual const std::string &temp_directory() const             = 0;

	void write_file(const std::string &path, const std::string &data);

	// Read the entire file into a string
	std::string read_file(const std::string &path);

	// Read the entire file into a vector of bytes
	template <typename T = uint8_t>
	std::vector<T> read_binary_file(const std::string &path)
	{
		static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");
		auto stat   = stat_file(path);
		auto binary = read_chunk(path, 0, stat.size);
		return std::vector<T>(binary.data(), binary.data() + binary.size() / sizeof(T));
	}
};

using FileSystemPtr = std::shared_ptr<FileSystem>;

// Initialize the filesystem with the given context
void init(const PlatformContext &context);

// Get the filesystem instance
FileSystemPtr get();

namespace helpers
{
std::string filename(const std::string &path);
}
}        // namespace filesystem
}        // namespace vkb