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

#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <catch2/catch_test_macros.hpp>
VKBP_ENABLE_WARNINGS()

#include <string>
#include <vector>

#include <components/common/stack_error.hpp>

using namespace components;

TEST_CASE("what() exists and is as expected", "[common]")
{
	struct Test
	{
		std::string message;
		std::string file;
		uint32_t    line;
		std::string expected;
	};

	std::vector<Test> tests{
	    {
	        "this is an error",
	        "some/file.cpp",
	        24,
	        "[some/file.cpp:24] this is an error\n",
	    },
	    {
	        "this is another error",
	        "some/file.cpp",
	        0,
	        "[some/file.cpp:0] this is another error\n",
	    },
	    {
	        "this is another error",
	        "",
	        0,
	        "this is another error\n",
	    },
	};

	for (auto &test : tests)
	{
		auto error = StackError::unique(test.message, test.file.c_str(), test.line);
		REQUIRE(error->what() != nullptr);
		REQUIRE(std::string{error->what()} == test.expected);
	}
}

TEST_CASE("multiple stack errors", "[common]")
{
	auto error = StackError::unique("this is a test message", "file.cpp", 1);
	error->push("this is a another test message", "file.cpp", 2);
	error->push("this is a final test message");
	REQUIRE(error->size() == 3);
	REQUIRE(std::string{error->what()} == "[file.cpp:1] this is a test message\n[file.cpp:2] this is a another test message\nthis is a final test message\n");
}

TEST_CASE("combine stack errors", "[common]")
{
	auto error1 = StackError::unique("this is a test message", "file.cpp", 1);
	auto error2 = StackError::unique("this is a test message", "another_file.cpp", 2);
	auto error  = StackError::combine(std::move(error1), std::move(error2));
	REQUIRE(error->size() == 2);
	REQUIRE(std::string{error->what()} == "[file.cpp:1] this is a test message\n[another_file.cpp:2] this is a test message\n");
}

TEST_CASE("combine larger stack errors", "[common]")
{
	auto error1 = StackError::unique("this is a test message", "file.cpp", 1);
	error1->push("this is a test message", "file.cpp", 1);
	error1->push("this is a test message", "file.cpp", 1);
	error1->push("this is a test message", "file.cpp", 1);
	REQUIRE(error1->size() == 4);

	auto error2 = StackError::unique("this is a test message", "another_file.cpp", 2);
	error2->push("this is a test message", "another_file.cpp", 2);
	error2->push("this is a test message", "another_file.cpp", 2);
	REQUIRE(error2->size() == 3);

	auto error = StackError::combine(std::move(error1), std::move(error2));
	REQUIRE(error->size() == 7);

	REQUIRE(std::string{error->what()} == "[file.cpp:1] this is a test message\n[file.cpp:1] this is a test message\n[file.cpp:1] this is a test message\n[file.cpp:1] this is a test message\n[another_file.cpp:2] this is a test message\n[another_file.cpp:2] this is a test message\n[another_file.cpp:2] this is a test message\n");
}