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

#include "platform/filesystem.h"

#include "common/warnings.h"

VKBP_DISABLE_WARNINGS()
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
VKBP_ENABLE_WARNINGS()

#include "platform/platform.h"

namespace vkb
{
namespace fs
{
namespace path
{
const std::unordered_map<Type, std::string> relative_paths = {{Type::Assets, "assets/"},
                                                              {Type::Shaders, "shaders/"},
                                                              {Type::Storage, "output/"},
                                                              {Type::Screenshots, "output/images/"},
                                                              {Type::Logs, "output/logs/"},
                                                              {Type::Graphs, "output/graphs/"}};

std::string get(const Type type, const std::string &file)
{
	assert(relative_paths.size() == Type::TotalRelativePathTypes && "Not all paths are defined in filesystem, please check that each enum is specified");

	// Check for special cases first
	if (type == Type::WorkingDir)
	{
		return Platform::get_external_storage_directory();
	}
	else if (type == Type::Temp)
	{
		return Platform::get_temp_directory();
	}

	// Check for relative paths
	auto it = relative_paths.find(type);

	if (relative_paths.size() < Type::TotalRelativePathTypes)
	{
		throw std::runtime_error("Platform hasn't initialized the paths correctly");
	}
	else if (it == relative_paths.end())
	{
		throw std::runtime_error("Path enum doesn't exist, or wasn't specified in the path map");
	}
	else if (it->second.empty())
	{
		throw std::runtime_error("Path was found, but it is empty");
	}

	auto path = Platform::get_external_storage_directory() + it->second;

	if (!is_directory(path))
	{
		create_path(Platform::get_external_storage_directory(), it->second);
	}

	return path + file;
}
}        // namespace path

bool is_directory(const std::string &path)
{
	struct stat info
	{};
	if (stat(path.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR))
	{
		return false;
	}
	else if (info.st_mode & S_IFDIR)
	{
		return true;
	}
	return false;
}

bool is_file(const std::string &filename)
{
	std::ifstream f(filename.c_str());
	return !f.fail();
}

void create_path(const std::string &root, const std::string &path)
{
	for (auto it = path.begin(); it != path.end(); ++it)
	{
		it = std::find(it, path.end(), '/');
		create_directory(root + std::string(path.begin(), it));
	}
}

std::string read_text_file(const std::string &filename)
{
	std::vector<std::string> data;

	std::ifstream file;

	file.open(filename, std::ios::in);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file: " + filename);
	}

	return std::string{(std::istreambuf_iterator<char>(file)),
	                   (std::istreambuf_iterator<char>())};
}

std::vector<uint8_t> read_binary_file(const std::string &filename, const uint32_t count)
{
	std::vector<uint8_t> data;

	std::ifstream file;

	file.open(filename, std::ios::in | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file: " + filename);
	}

	uint64_t read_count = count;
	if (count == 0)
	{
		file.seekg(0, std::ios::end);
		read_count = static_cast<uint64_t>(file.tellg());
		file.seekg(0, std::ios::beg);
	}

	data.resize(static_cast<size_t>(read_count));
	file.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(read_count));
	file.close();

	return data;
}

static void write_binary_file(const std::vector<uint8_t> &data, const std::string &filename, const uint32_t count)
{
	std::ofstream file;

	file.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file: " + filename);
	}

	uint64_t write_count = count;
	if (count == 0)
	{
		write_count = data.size();
	}

	file.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(write_count));
	file.close();
}

std::vector<uint8_t> read_asset(const std::string &filename, const uint32_t count)
{
	return read_binary_file(path::get(path::Type::Assets) + filename, count);
}

std::string read_shader(const std::string &filename)
{
	return read_text_file(path::get(path::Type::Shaders) + filename);
}

std::vector<uint8_t> read_shader_binary(const std::string &filename)
{
	return read_binary_file(path::get(path::Type::Shaders) + filename, 0);
}

std::vector<uint8_t> read_temp(const std::string &filename, const uint32_t count)
{
	return read_binary_file(path::get(path::Type::Temp) + filename, count);
}

void write_temp(const std::vector<uint8_t> &data, const std::string &filename, const uint32_t count)
{
	write_binary_file(data, path::get(path::Type::Temp) + filename, count);
}

void write_image(const uint8_t *data, const std::string &filename, const uint32_t width, const uint32_t height, const uint32_t components, const uint32_t row_stride)
{
	stbi_write_png((path::get(path::Type::Screenshots) + filename + ".png").c_str(), static_cast<int>(width), static_cast<int>(height), components, data, static_cast<int>(row_stride));
}

bool write_json(nlohmann::json &data, const std::string &filename)
{
	std::stringstream json;

	try
	{
		// Whitespace needed as last character is overwritten on android causing the json to be corrupt
		json << data << " ";
	}
	catch (std::exception &e)
	{
		// JSON dump errors
		LOGE(e.what());
		return false;
	}

	if (!nlohmann::json::accept(json.str()))
	{
		LOGE("Invalid JSON string");
		return false;
	}

	std::ofstream out_stream;
	out_stream.open(fs::path::get(vkb::fs::path::Type::Graphs) + filename, std::ios::out | std::ios::trunc);

	if (out_stream.good())
	{
		out_stream << json.str();
	}
	else
	{
		LOGE("Could not load JSON file " + filename);
		return false;
	}

	out_stream.close();
	return true;
}
}        // namespace fs
}        // namespace vkb
