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

#include "core/platform/context.hpp"
#include "core/util/error.hpp"

#include "std_filesystem.hpp"

namespace vkb
{
namespace filesystem
{
static FileSystemPtr fs = nullptr;

void init()
{
	fs = std::make_shared<StdFileSystem>();
}

void init_with_context(const PlatformContext &context)
{
	fs = std::make_shared<StdFileSystem>(
	    context.external_storage_directory(),
	    context.temp_directory());
}

FileSystemPtr get()
{
	assert(fs && "Filesystem not initialized");
	return fs;
}

void FileSystem::write_file(const Path &path, const std::string &data)
{
	write_file(path, std::vector<uint8_t>(data.begin(), data.end()));
}

std::string FileSystem::read_file(const Path &path)
{
	auto bin = read_binary_file(path);
	return {bin.begin(), bin.end()};
}

}        // namespace filesystem
}        // namespace vkb