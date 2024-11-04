/* Copyright (c) 2023-2024, Thomas Atkinson
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

#include <catch2/catch_test_macros.hpp>

#include "filesystem/filesystem.hpp"

using namespace vkb::filesystem;

void create_test_file(FileSystemPtr fs, const Path &path, const std::string &data)
{
	REQUIRE(fs);
	REQUIRE_NOTHROW(fs->write_file(path, data));
	REQUIRE(fs->exists(path));
	REQUIRE(fs->is_file(path));

	const auto written_data = fs->read_file_string(path);
	REQUIRE(data == written_data);
}

void delete_test_file(FileSystemPtr fs, const Path &path)
{
	REQUIRE(fs);
	REQUIRE(fs->exists(path));
	REQUIRE(fs->is_file(path));
	REQUIRE_NOTHROW(fs->remove(path));
	REQUIRE_FALSE(fs->exists(path));
}

TEST_CASE("File read and write", "[filesystem]")
{
	vkb::filesystem::init();

	auto fs = vkb::filesystem::get();

	const auto        test_file = fs->temp_directory() / "vulkan_samples" / "test.txt";
	const std::string test_data = "Hello, World!";

	create_test_file(fs, test_file, test_data);
	delete_test_file(fs, test_file);
}

TEST_CASE("Read file chunk", "[filesystem]")
{
	vkb::filesystem::init();

	auto fs = vkb::filesystem::get();

	const auto        test_file = fs->temp_directory() / "vulkan_samples" / "chunk_test.txt";
	const std::string test_data = "Hello, World!";

	create_test_file(fs, test_file, test_data);

	const auto  chunk = fs->read_chunk(test_file, 0, 5);
	std::string chunk_str(chunk.begin(), chunk.end());
	REQUIRE(chunk_str == "Hello");

	delete_test_file(fs, test_file);
}

TEST_CASE("Read file chunk out of bounds", "[filesystem]")
{
	vkb::filesystem::init();

	auto fs = vkb::filesystem::get();

	const auto        test_file = fs->temp_directory() / "vulkan_samples" / "chunk_oob_test.txt";
	const std::string test_data = "Hello, World!";

	create_test_file(fs, test_file, test_data);

	const auto chunk = fs->read_chunk(test_file, 0, 100);
	REQUIRE(chunk.empty());

	delete_test_file(fs, test_file);
}

TEST_CASE("Read file chunk with offset", "[filesystem]")
{
	vkb::filesystem::init();

	auto fs = vkb::filesystem::get();

	const auto        test_file = fs->temp_directory() / "vulkan_samples" / "chunk_offset_test.txt";
	const std::string test_data = "Hello, World!";

	create_test_file(fs, test_file, test_data);

	const auto  chunk = fs->read_chunk(test_file, 7, 5);
	std::string chunk_str(chunk.begin(), chunk.end());
	REQUIRE(chunk_str == "World");

	delete_test_file(fs, test_file);
}

TEST_CASE("Read binary file", "[filesystem]")
{
	vkb::filesystem::init();

	auto fs = vkb::filesystem::get();

	const auto        test_file = fs->temp_directory() / "vulkan_samples" / "binary_test.txt";
	const std::string test_data = "Hello, World!";

	create_test_file(fs, test_file, test_data);

	const auto  binary = fs->read_file_binary(test_file);
	std::string binary_str(binary.begin(), binary.end());
	REQUIRE(binary_str == test_data);

	delete_test_file(fs, test_file);
}

TEST_CASE("Create Directory", "[filesystem]")
{
	vkb::filesystem::init();

	auto fs = vkb::filesystem::get();

	const auto test_dir = fs->temp_directory() / "vulkan_samples_directory_test" / "test_dir";

	REQUIRE(fs->create_directory(test_dir));
	REQUIRE(fs->exists(test_dir));
	REQUIRE(fs->is_directory(test_dir));

	std::vector<uint8_t> data = {0, 1, 2, 3, 4, 5};
	for (uint8_t i : data)
	{
		// Create sub directories
		const auto sub_dir = test_dir / fmt::format("sub_dir_{}", i);
		REQUIRE(fs->create_directory(sub_dir));
	}

	// Check in a separate pass to ensure create_directory called multiple times doesn't fail
	for (uint8_t i : data)
	{
		// Check sub directories
		const auto sub_dir = test_dir / fmt::format("sub_dir_{}", i);
		REQUIRE(fs->exists(sub_dir));
		REQUIRE(fs->is_directory(sub_dir));
	}

	// Remove both the directory and its parent
	fs->remove(fs->temp_directory() / "vulkan_samples_directory_test");

	REQUIRE_FALSE(fs->exists(test_dir));
}

TEST_CASE("Remove File Test", "[filesystem]")
{
	vkb::filesystem::init();

	auto fs = vkb::filesystem::get();

	const auto test_file = fs->temp_directory() / "vulkan_samples_remove_file_test" / "remove_test.txt";

	REQUIRE(fs->create_directory(test_file.parent_path()));

	fs->write_file(test_file, "Hello, World!");

	REQUIRE(fs->exists(test_file));
	REQUIRE(fs->is_file(test_file));

	REQUIRE_NOTHROW(fs->remove(test_file));

	REQUIRE_FALSE(fs->exists(test_file));

	fs->remove(test_file.parent_path());
}
