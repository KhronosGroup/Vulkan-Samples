/* Copyright (c) 2023, Tom Atkinson. All rights reserved.
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

#include "core/containers/type_map.hpp"

using namespace vkb;

TEST_CASE("TypeMap", "[containers]")
{
	SECTION("find_or_insert")
	{
		vkb::TypeMap<int> map;

		int value = 0;

		auto it = map.find_or_insert<int>([&]() { return value++; });

		REQUIRE(it->second == 0);

		it = map.find_or_insert<int>([&]() { return value++; });

		REQUIRE(it->second == 0);

		it = map.find_or_insert<float>([&]() { return value++; });

		REQUIRE(it->second == 1);
	}

	SECTION("replace_emplace")
	{
		vkb::TypeMap<int> map;

		auto it = map.replace_emplace<int>(0);
		REQUIRE(it->second == 0);

		it = map.replace_emplace<int>(1);
		REQUIRE(it->second == 1);

		REQUIRE(map.find<int>()->second == 1);
	}

	SECTION("find")
	{
		vkb::TypeMap<int> map;

		int value = 0;

		map.replace_emplace<int>(value++);

		auto it = map.find<int>();

		REQUIRE(it->second == 0);

		it = map.find<float>();

		REQUIRE(it == map.end());
	}

	SECTION("find const")
	{
		vkb::TypeMap<int> map;

		int value = 0;

		map.replace_emplace<int>(value++);

		auto it = map.find<int>();

		REQUIRE(it->second == 0);

		it = map.find<float>();

		REQUIRE(it == map.end());
	}

	SECTION("contains", "[containers]")
	{
		vkb::TypeMap<int> map;

		int value = 0;

		map.replace_emplace<int>(value++);

		REQUIRE(map.contains<int>() == true);
		REQUIRE(map.contains<float>() == false);
	}
}