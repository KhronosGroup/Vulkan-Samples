/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "filesystem/legacy.h"

#include "core/util/error.hpp"

VKBP_DISABLE_WARNINGS()
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
VKBP_ENABLE_WARNINGS()

#include "filesystem/filesystem.hpp"

namespace vkb
{
namespace fs
{
namespace path
{
const std::unordered_map<Type, std::string> relative_paths = {
    {Type::Assets, "assets/"},
    {Type::Shaders, "shaders/"},
    {Type::Storage, "output/"},
    {Type::Screenshots, "output/images/"},
    {Type::Logs, "output/logs/"},
};

const std::string get(const Type type, const std::string &file)
{
	assert(relative_paths.size() == Type::TotalRelativePathTypes && "Not all paths are defined in filesystem, please check that each enum is specified");

	// Check for special cases first
	if (type == Type::Temp)
	{
		return vkb::filesystem::get()->temp_directory();
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

	auto fs   = vkb::filesystem::get();
	auto path = fs->external_storage_directory() + it->second;

	if (!is_directory(path))
	{
		create_path(fs->external_storage_directory(), it->second);
	}

	return path + file;
}
}        // namespace path

bool is_directory(const std::string &path)
{
	return vkb::filesystem::get()->is_directory(path);
}

bool is_file(const std::string &filename)
{
	return vkb::filesystem::get()->is_file(filename);
}

void create_directory(const std::string &path)
{
	vkb::filesystem::get()->create_directory(path);
}

void create_path(const std::string &root, const std::string &path)
{
	for (auto it = path.begin(); it != path.end(); ++it)
	{
		it = std::find(it, path.end(), '/');
		create_directory(root + std::string(path.begin(), it));
	}
}

std::vector<uint8_t> read_asset(const std::string &filename)
{
	return vkb::filesystem::get()->read_binary_file<uint8_t>(path::get(path::Type::Assets) + filename);
}

std::string read_shader(const std::string &filename)
{
	return vkb::filesystem::get()->read_file(path::get(path::Type::Shaders) + filename);
}

std::vector<uint8_t> read_shader_binary(const std::string &filename)
{
	return vkb::filesystem::get()->read_binary_file<uint8_t>(path::get(path::Type::Shaders) + filename);
}

std::vector<uint8_t> read_temp(const std::string &filename)
{
	return vkb::filesystem::get()->read_binary_file<uint8_t>(path::get(path::Type::Temp) + filename);
}

void write_temp(const std::vector<uint8_t> &data, const std::string &filename)
{
	vkb::filesystem::get()->write_file(path::get(path::Type::Temp) + filename, data);
}

void write_image(const uint8_t *data, const std::string &filename, const uint32_t width, const uint32_t height, const uint32_t components, const uint32_t row_stride)
{
	stbi_write_png((path::get(path::Type::Screenshots) + filename + ".png").c_str(), width, height, components, data, row_stride);
}

}        // namespace fs
}        // namespace vkb
