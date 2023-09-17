/* Copyright (c) 2023, Thomas Atkinson
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

#include <core/platform/entrypoint.hpp>
#include <core/util/error.hpp>
#include <fs/filesystem.hpp>

#define EXPECT_TRUE(x, ...)  \
	if (!x)                  \
	{                        \
		ERRORF(__VA_ARGS__); \
	}

CUSTOM_MAIN(context)
{
	auto fs = vkb::fs::get_filesystem(context);
	EXPECT_TRUE(fs, "Filesystem is not null");

	std::string DIR = "./components/fs";

	EXPECT_TRUE(fs->exists(DIR), "Directory exists");
	EXPECT_TRUE(fs->is_directory(DIR), "Path is a directory");
	EXPECT_TRUE(!fs->is_file(DIR), "Path is not a file");

	{
		auto stat = fs->stat_file(DIR);
		EXPECT_TRUE(stat.is_directory, "Path is a directory");
		EXPECT_TRUE(!stat.is_file, "Path is not a file");
		EXPECT_TRUE(stat.size == 0, "Directory size is 0");
	}

	std::string FILE = DIR + "/tests/std_filesystem.test.cpp";
	auto        stat = fs->stat_file(FILE);
	EXPECT_TRUE(!stat.is_directory, "Path is not a directory");
	EXPECT_TRUE(stat.is_file, "Path is a file");
	EXPECT_TRUE(stat.size > 0, "File size is greater than 0");

	return 0;
}