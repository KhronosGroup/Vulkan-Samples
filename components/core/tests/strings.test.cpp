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

#include <core/util/error.hpp>

#include <catch2/catch_test_macros.hpp>

#include <core/util/strings.hpp>

using namespace vkb;

TEST_CASE("vkb::replace_all", "[common]")
{
	REQUIRE(replace_all("/././", "./", "/") == "///");
	REQUIRE(replace_all("vulkanvulkanvulkan", "vulkan", "kan") == "kankankan");
}

TEST_CASE("vkb::trim_right", "[common]")
{
	REQUIRE(trim_right("hello   ") == "hello");        // default case
	REQUIRE(trim_right("hello   ", " ") == "hello");
	REQUIRE(trim_right("hello   ignore", " ") == "hello   ignore");
	REQUIRE(trim_right("hellocomplex", "complex") == "h");        // remember we are trimming a set
}

TEST_CASE("vkb::trim_left", "[common]")
{
	REQUIRE(trim_left("   hello") == "hello");        // default case
	REQUIRE(trim_left("   hello", " ") == "hello");
	REQUIRE(trim_left("ignore   hello", " ") == "ignore   hello");
	REQUIRE(trim_left("complexhello", "complex") == "hello");        // remember we are trimming a set until the first non-match
}