/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>

#include "common/warnings.h"

VKBP_DISABLE_WARNINGS()
#include <json.hpp>
VKBP_ENABLE_WARNINGS()

namespace vkb
{
namespace fs
{
namespace path
{
enum Type
{
	//Relative paths
	Assets,
	Shaders,
	Storage,
	Screenshots,
	Logs,
	Graphs,
	/* NewFolder */
	TotalRelativePathTypes,

	//Special paths
	ExternalStorage,
	WorkingDir = ExternalStorage,
	Temp
};

extern const std::unordered_map<Type, std::string> relative_paths;

/**
 * @brief Gets the absolute path of a given type or a specific file
 * @param type The type of file path
 * @param file (Optional) The filename
 * @throws runtime_error if the platform didn't initialize each path properly, path wasn't found or the path was found but is empty
 * @return Path to the directory of a certain type
 */
std::string get(const Type type, const std::string &file = "");
}        // namespace path

/**
 * @brief Helper to tell if a given path is a directory
 * @param path A path to a directory
 * @return True if the path points to a valid directory, false if not
 */
bool is_directory(const std::string &path);

/** 
 * @brief Checks if a file exists
 * @param filename The filename to check
 * @return True if the path points to a valid file, false if not
 */
bool is_file(const std::string &filename);

/**
 * @brief Platform specific implementation to create a directory
 * @param path A path to a directory
 */
void create_directory(const std::string &path);

/**
 * @brief Recursively creates a directory
 * @param root The root directory that the path is relative to
 * @param path A path in the format 'this/is/an/example/path/'
 */
void create_path(const std::string &root, const std::string &path);

/**
 * @brief Helper to read an asset file into a byte-array
 *
 * @param filename The path to the file (relative to the assets directory)
 * @param count (optional) How many bytes to read. If 0 or not specified, the size
 * of the file will be used.
 * @return A vector filled with data read from the file
 */
std::vector<uint8_t> read_asset(const std::string &filename, const uint32_t count = 0);

/**
 * @brief Helper to read a shader file into a single string
 *
 * @param filename The path to the file (relative to the assets directory)
 * @return A string of the text in the shader file
 */
std::string read_shader(const std::string &filename);

/**
 * @brief Helper to read a shader file into a byte-array
 *
 * @param filename The path to the file (relative to the assets directory)
 * @return A vector filled with data read from the file
 */
std::vector<uint8_t> read_shader_binary(const std::string &filename);

/**
 * @brief Helper to read a temporary file into a byte-array
 *
 * @param filename The path to the file (relative to the temporary storage directory)
 * @param count (optional) How many bytes to read. If 0 or not specified, the size
 * of the file will be used.
 * @return A vector filled with data read from the file
 */
std::vector<uint8_t> read_temp(const std::string &filename, const uint32_t count = 0);

/**
 * @brief Helper to write to a file in temporary storage
 *
 * @param data A vector filled with data to write
 * @param filename The path to the file (relative to the temporary storage directory)
 * @param count (optional) How many bytes to write. If 0 or not specified, the size
 * of data will be used.
 */
void write_temp(const std::vector<uint8_t> &data, const std::string &filename, const uint32_t count = 0);

/**
 * @brief Helper to write to a png image in permanent storage
 *
 * @param data       A vector filled with pixel data to write in (R, G, B, A) format
 * @param filename   The name of the image file without an extension
 * @param width      The width of the image
 * @param height     The height of the image
 * @param components The number of bytes per element
 * @param row_stride The stride in bytes of a row of pixels
 */
void write_image(const uint8_t *data, const std::string &filename, const uint32_t width, const uint32_t height, const uint32_t components, const uint32_t row_stride);

/**
 * @brief Helper to output a json graph
 * 
 * @param data A json object
 * @param filename The name of the file
 */
bool write_json(nlohmann::json &data, const std::string &filename);
}        // namespace fs
}        // namespace vkb
