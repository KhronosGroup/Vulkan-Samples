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

#include <core/util/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <catch2/catch_test_macros.hpp>
VKBP_ENABLE_WARNINGS()

#include <core/util/strings.hpp>

using namespace vkb;

TEST_CASE("vkb::replace_all", "[core]")
{
	REQUIRE(replace_all("/././", "./", "/") == "///");
	REQUIRE(replace_all("vulkanvulkanvulkan", "vulkan", "kan") == "kankankan");
}

TEST_CASE("vkb::trim_right", "[core]")
{
	REQUIRE(trim_right("hello   ") == "hello");        // default case
	REQUIRE(trim_right("hello   ", " ") == "hello");
	REQUIRE(trim_right("hello   ignore", " ") == "hello   ignore");
	REQUIRE(trim_right("hellocomplex", "complex") == "h");        // remember we are trimming a set
}

TEST_CASE("vkb::trim_left", "[core]")
{
	REQUIRE(trim_left("   hello") == "hello");        // default case
	REQUIRE(trim_left("   hello", " ") == "hello");
	REQUIRE(trim_left("ignore   hello", " ") == "ignore   hello");
	REQUIRE(trim_left("complexhello", "complex") == "hello");        // remember we are trimming a set until the first non-match
}

TEST_CASE("vkb::split", "[core]")
{
	REQUIRE(split("hello world") == std::vector<std::string>{"hello", "world"});
	REQUIRE(split("hello world", " ") == std::vector<std::string>{"hello", "world"});
	REQUIRE(split("hello world", "world") == std::vector<std::string>{"hello ", ""});
	REQUIRE(split("hello_world", "_") == std::vector<std::string>{"hello", "world"});
}

TEST_CASE("vkb::to_snake_case", "[core]")
{
	REQUIRE(to_snake_case("HelloWorld") == "hello_world");

	// continuous upper case is not split into multiple words
	REQUIRE(to_snake_case("ABC") == "abc");
	REQUIRE(to_snake_case("ABCDef") == "abc_def");
}

TEST_CASE("vkb::to_upper_case", "[core]")
{
	REQUIRE(to_upper_case("ABC") == "ABC");
	REQUIRE(to_upper_case("ABCDef") == "ABCDEF");
	REQUIRE(to_upper_case("abc") == "ABC");
}

TEST_CASE("vkb::ends_with", "[core]")
{
	REQUIRE(ends_with("hello world", "world") == true);
	REQUIRE(ends_with("hello world", "WORLD") == false);
	REQUIRE(ends_with("hello world", "WORLD", false) == true);
}