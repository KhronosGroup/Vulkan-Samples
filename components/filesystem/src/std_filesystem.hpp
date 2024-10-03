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

#include "filesystem/filesystem.hpp"

#include <filesystem>

namespace vkb
{
namespace filesystem
{
class StdFileSystem final : public FileSystem
{
  public:
	StdFileSystem(Path external_storage_directory = std::filesystem::current_path(), Path temp_directory = std::filesystem::temp_directory_path()) :
	    _external_storage_directory(std::move(external_storage_directory)),
	    _temp_directory(std::move(temp_directory))
	{}

	virtual ~StdFileSystem() = default;

	FileStat stat_file(const Path &path) override;

	bool is_file(const Path &path) override;

	bool is_directory(const Path &path) override;

	bool exists(const Path &path) override;

	bool create_directory(const Path &path) override;

	std::vector<uint8_t> read_chunk(const Path &path, size_t offset, size_t count) override;

	void write_file(const Path &path, const std::vector<uint8_t> &data) override;

	virtual void remove(const Path &path) override;

	virtual void set_external_storage_directory(const std::string &dir) override;

	const Path &external_storage_directory() const override;

	const Path &temp_directory() const override;

  private:
	Path _external_storage_directory;
	Path _temp_directory;
};
}        // namespace filesystem
}        // namespace vkb