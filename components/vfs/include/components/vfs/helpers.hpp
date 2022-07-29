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

#include <string>
#include <vector>

namespace components
{
namespace vfs
{
namespace helpers
{
/**
 * @brief Extracts the extension from an uri
 * @param uri An uniform Resource Identifier
 * @return The extension
 */
std::string get_file_extension(const std::string &uri);

/**
 * @brief Get the parent directory from a given path
 *
 * @param path a path
 * @return std::string The parent directory or an empty string
 */
std::string get_directory(const std::string &path);

/**
 * @brief Get a paths directory and then split its path into an array of parts
 *        e.g /temp/folder/file.txt -> ["/", "/temp", "/temp/folder"]
 *
 * @param path A path
 * @return std::vector<std::string> A vector of topological directory parts
 */
std::vector<std::string> get_directory_parts(const std::string &path);

/**
 * @brief Get a paths directory and then split its path into an array of individual parts
 *        e.g /temp/folder/file.txt -> ["temp", "folder"]
 *
 * @param path A path
 * @return std::vector<std::string> A vector of topological directory parts
 */
std::vector<std::string> tokenize_path(const std::string &path);

/**
 * @brief Get a files name from a path
 *        e.g /temp/file.txt -> "file.txt"
 *
 * @param path a path
 * @return std::string the file name
 */
std::string get_file_name(const std::string &path);

/**
 * @brief sanitize a path
 *
 * @param path a path
 * @return std::string a sanitized path
 */
std::string sanitize(const std::string &path);

/**
 * @brief Join multiple paths together
 *
 * @param paths parts of a path
 * @return std::string a sanitized path
 */
std::string join(const std::vector<std::string> &paths);
}        // namespace helpers
}        // namespace vfs
}        // namespace components